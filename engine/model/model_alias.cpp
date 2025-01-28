/******************************************************************************
	Copyright © 1997-2001 Id Software, Inc.
	Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
******************************************************************************/

#include <cfloat>

#include "qcommon/qcommon.h"
#include "qcommon/qfiles.h"

#include "renderer/renderer.h"
#include "renderer/ref_gl/gl_local.h"//TODO: eventually be rid of this, abstract it away!

#include "model_alias.h"

chr::AliasModel::AliasModel()  = default;
chr::AliasModel::~AliasModel() = default;

bool chr::AliasModel::LoadFromBuffer( const void *buffer )
{
	const dmdl_t *pinmodel = ( dmdl_t * ) buffer;
	int           version  = LittleLong( pinmodel->version );
	if ( version != 14 && version != 15 )
	{
		Com_Printf( "Alias model has wrong version number (%i should be 14/15)\n", version );
		return false;
	}

	int resolution = LittleLong( pinmodel->resolution );
	if ( resolution < 0 || resolution > 2 )
	{
		Com_Printf( "Invalid resolution, %d (expected 0 to 2)\n", resolution );
		return false;
	}

	numVertices_ = LittleLong( pinmodel->num_xyz );
	if ( numVertices_ <= 0 )
	{
		Com_Printf( "Model has no vertices\n" );
		return false;
	}

	numTriangles_ = LittleLong( pinmodel->num_tris );
	if ( numTriangles_ <= 0 )
	{
		Com_Printf( "Model has no triangles\n" );
		return false;
	}

	numFrames_ = LittleLong( pinmodel->num_frames );
	if ( numFrames_ <= 0 )
	{
		Com_Printf( "Model has no frames\n" );
		return false;
	}

	numGLCmds_ = LittleLong( pinmodel->num_glcmds );
	if ( numGLCmds_ <= 0 )
	{
		Com_Printf( "Model has empty command list\n" );
		return false;
	}

	int numSkins = LittleLong( pinmodel->num_skins );
	if ( numSkins <= 0 )
	{
		Com_Printf( "Model has no skins\n" );
		return false;
	}

	LoadSkins( pinmodel, numSkins );
	LoadTriangles( pinmodel );
	LoadTaggedTriangles( pinmodel );
	LoadCommands( pinmodel );
	LoadFrames( pinmodel, resolution );

	lerpedVertices_.resize( numVertices_ );

	return true;
}

void chr::AliasModel::LoadSkins( const dmdl_t *mdl, int numSkins )
{
	uint ofs = LittleLong( mdl->ofs_skins );
	for ( int i = 0; i < numSkins; ++i )
	{
		char name[ MAX_QPATH + 1 ];
		memcpy( name, ( byte * ) mdl + ofs + i * MAX_QPATH, MAX_QPATH );
		name[ strlen( name ) + 1 ] = '\0';
		skinNames_.emplace_back( name );
	}
}

void chr::AliasModel::LoadTriangles( const dmdl_t *mdl )
{
	struct dtriangle_t
	{
		int16_t index_xyz[ 3 ];
		int16_t index_st[ 3 ];
	};

	uint               ofs  = LittleLong( mdl->ofs_tris );
	const dtriangle_t *dtri = ( dtriangle_t * ) ( ( byte * ) mdl + ofs );
	for ( int i = 0; i < numTriangles_; ++i )
	{
		Triangle tri;
		for ( uint j = 0; j < 3; ++j )
		{
			tri.vertexIndices[ j ] = LittleShort( dtri[ i ].index_xyz[ j ] );
			tri.stIndices[ j ]     = LittleShort( dtri[ i ].index_st[ j ] );
		}

		triangles_.push_back( tri );
	}
}

void chr::AliasModel::LoadTaggedTriangles( const dmdl_t *mdl )
{
	struct MD2TaggedSurface
	{
		char     name[ 8 ];
		uint32_t triangleIndex;
	};

	int                     numTaggedTriangles = LittleLong( mdl->numTaggedTriangles );
	uint                    ofs                = LittleLong( mdl->taggedTrianglesOffset );
	const MD2TaggedSurface *taggedSurface      = ( MD2TaggedSurface * ) ( ( byte * ) mdl + ofs );
	for ( int i = 0; i < numTaggedTriangles; ++i )
	{
		taggedTriangles_.emplace( taggedSurface->name, LittleLong( taggedSurface->triangleIndex ) );
	}
}

void chr::AliasModel::LoadCommands( const dmdl_t *mdl )
{
	uint       ofs  = LittleLong( mdl->ofs_glcmds );
	const int *dcmd = ( int * ) ( ( byte * ) mdl + ofs );
	for ( int i = 0; i < numGLCmds_; ++i )
		glCmds_.push_back( LittleLong( dcmd[ i ] ) );
}

void chr::AliasModel::LoadFrames( const dmdl_t *mdl, int resolution )
{
	frames_.resize( numFrames_ );

	struct MD2FrameHeader
	{
		float scale[ 3 ];    // multiply byte verts by this
		float translate[ 3 ];// then add this
		char  name[ 16 ];    // frame name from grabbing
	};

	uint ofs       = LittleLong( mdl->ofs_frames );
	int  frameSize = LittleLong( mdl->framesize );
	for ( int i = 0; i < numFrames_; ++i )
	{
		frames_[ i ].vertices.resize( numVertices_ );

		// Get the position in the buffer where our cur frame is located
		const void *framePtr = ( ( byte * ) mdl + ofs + i * frameSize );

		// The initial body of the frame is always the same, regardless of resolution
		const MD2FrameHeader *frameHeader = ( MD2FrameHeader * ) framePtr;
		frames_[ i ].name                 = frameHeader->name;
		for ( uint j = 0; j < 3; ++j )
		{
			frames_[ i ].scale[ j ]     = LittleFloat( frameHeader->scale[ j ] );
			frames_[ i ].translate[ j ] = LittleFloat( frameHeader->translate[ j ] );
		}

		// Nox supports three different resolution types, we'll deal with those below and
		// translate them into our common structure
		switch ( resolution )
		{
			case 0:
			{
				const MD2VertexGroup *groups = ( MD2VertexGroup * ) ( ( byte * ) framePtr + sizeof( MD2FrameHeader ) );
				for ( int j = 0; j < numVertices_; ++j )
				{
					for ( uint k = 0; k < 3; ++k )
						frames_[ i ].vertices[ j ].vertex[ k ] = groups[ j ].vertices[ k ];

					frames_[ i ].vertices[ j ].normalIndex = ( uint16_t ) LittleShort( ( int16_t ) groups[ j ].normalIndex );
				}
				break;
			}
			case 1:
			{
				static const int shift[ 3 ] = { 0, 11, 21 };
				static const int mask[ 3 ]  = { 0x000007ff, 0x000003ff, 0x000007ff };

				const MD2VertexGroup4 *groups = ( MD2VertexGroup4 * ) ( ( byte * ) framePtr + sizeof( MD2FrameHeader ) );
				for ( int j = 0; j < numVertices_; ++j )
				{
					uint32_t vertices = ( uint32_t ) LittleLong( ( int32_t ) groups[ j ].vertices );
					for ( uint k = 0; k < 3; ++k )
						frames_[ i ].vertices[ j ].vertex[ k ] = ( ( vertices >> shift[ k ] ) & mask[ k ] );

					frames_[ i ].vertices[ j ].normalIndex = ( uint16_t ) LittleShort( ( int16_t ) groups[ j ].normalIndex );
				}
				break;
			}
			case 2:
			{
				const MD2VertexGroup6 *groups = ( MD2VertexGroup6 * ) ( ( byte * ) framePtr + sizeof( MD2FrameHeader ) );
				for ( int j = 0; j < numVertices_; ++j )
				{
					for ( uint k = 0; k < 3; ++k )
						frames_[ i ].vertices[ j ].vertex[ k ] = ( uint16_t ) LittleShort( ( int16_t ) groups[ j ].vertices[ k ] );

					frames_[ i ].vertices[ j ].normalIndex = ( uint16_t ) LittleShort( ( int16_t ) groups[ j ].normalIndex );
				}
				break;
			}
			default:
				Com_Error( ERR_FATAL, "Unhandled MD2 resolution %d\n", resolution );
				break;
		}

		// Now go ahead and precompute the bounds for the given frame

		vec3_t mins = { frames_[ i ].translate[ 0 ], frames_[ i ].translate[ 1 ], frames_[ i ].translate[ 2 ] };
		vec3_t maxs = { -FLT_MIN, -FLT_MIN, -FLT_MIN };

		vec3_t xmins, xmaxs;
		VectorCopy( mins, xmins );
		VectorCopy( maxs, xmaxs );

		for ( int j = 0; j < numVertices_; ++j )
		{
			for ( uint k = 0; k < 3; ++k )
			{
				xmins[ k ] = frames_[ i ].vertices[ j ].vertex[ k ] * frames_[ i ].scale[ k ];
				if ( xmins[ k ] < mins[ k ] )
					mins[ k ] = xmins[ k ];

				xmaxs[ k ] = frames_[ i ].vertices[ j ].vertex[ k ] * frames_[ i ].scale[ k ];
				if ( xmaxs[ k ] > maxs[ k ] )
					maxs[ k ] = xmaxs[ k ];
			}
		}

		VectorCopy( mins, frames_[ i ].bounds[ 0 ] );
		VectorCopy( maxs, frames_[ i ].bounds[ 1 ] );
	}
}

#define NUMVERTEXNORMALS 162
static float r_avertexnormals[ NUMVERTEXNORMALS ][ 3 ] = {
#include "../renderer/ref_gl/anorms.h"


};

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float r_avertexnormal_dots[ SHADEDOT_QUANT ][ 256 ] = {
#include "../renderer/ref_gl/anormtab.h"


};

void chr::AliasModel::LerpVertices( const VertexGroup *v, const VertexGroup *ov, const VertexGroup *verts, Vector3 *lerp, float move[ 3 ], float frontv[ 3 ], float backv[ 3 ] ) const
{
	// PMM -- added RF_SHELL_DOUBLE, RF_SHELL_HALF_DAM
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE |
	                              RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
	{
		for ( int i = 0; i < numVertices_; i++, v++, ov++, lerp++ )
		{
			const float *normal = r_avertexnormals[ verts[ i ].normalIndex ];
			lerp->x             = move[ 0 ] + ov->vertex[ 0 ] * backv[ 0 ] + v->vertex[ 0 ] * frontv[ 0 ] + normal[ 0 ] * POWERSUIT_SCALE;
			lerp->y             = move[ 1 ] + ov->vertex[ 1 ] * backv[ 1 ] + v->vertex[ 1 ] * frontv[ 1 ] + normal[ 1 ] * POWERSUIT_SCALE;
			lerp->z             = move[ 2 ] + ov->vertex[ 2 ] * backv[ 2 ] + v->vertex[ 2 ] * frontv[ 2 ] + normal[ 2 ] * POWERSUIT_SCALE;
		}
	}
	else
	{
		for ( int i = 0; i < numVertices_; i++, v++, ov++, lerp++ )
		{
			lerp->x = move[ 0 ] + ov->vertex[ 0 ] * backv[ 0 ] + v->vertex[ 0 ] * frontv[ 0 ];
			lerp->y = move[ 1 ] + ov->vertex[ 1 ] * backv[ 1 ] + v->vertex[ 1 ] * frontv[ 1 ];
			lerp->z = move[ 2 ] + ov->vertex[ 2 ] * backv[ 2 ] + v->vertex[ 2 ] * frontv[ 2 ];
		}
	}
}

void chr::AliasModel::ApplyLighting( const entity_t *e )
{
	if ( e->flags & RF_FULLBRIGHT )
	{
		for ( float &i : shadeLight_ )
		{
			i = 1.0f;
		}
	}
	else
	{
		R_LightPoint( e->origin, shadeLight_ );

		if ( *gl_monolightmap->string != '0' )
		{
			float s = shadeLight_[ 0 ];
			if ( s < shadeLight_[ 1 ] )
			{
				s = shadeLight_[ 1 ];
			}
			if ( s < shadeLight_[ 2 ] )
			{
				s = shadeLight_[ 2 ];
			}

			shadeLight_[ 0 ] = s;
			shadeLight_[ 1 ] = s;
			shadeLight_[ 2 ] = s;
		}
	}

	if ( e->flags & RF_MINLIGHT )
	{
		uint i;
		for ( i = 0; i < 3; ++i )
		{
			if ( shadeLight_[ i ] > 0.1f )
			{
				break;
			}
		}

		if ( i == 3 )
		{
			shadeLight_[ 0 ] = 0.1f;
			shadeLight_[ 1 ] = 0.1f;
			shadeLight_[ 2 ] = 0.1f;
		}
	}

	// bonus items will pulse with time
	if ( e->flags & RF_GLOW )
	{
		float scale = 0.1f * std::sin( r_newrefdef.time * 7.0f );
		for ( float &i : shadeLight_ )
		{
			float min = i * 0.8f;
			i += scale;
			if ( i < min )
				i = min;
		}
	}

	if ( r_newrefdef.rdflags & RDF_IRGOGGLES && e->flags & RF_IR_VISIBLE )
	{
		shadeLight_[ 0 ] = 1.0f;
		shadeLight_[ 1 ] = 0.0f;
		shadeLight_[ 2 ] = 0.0f;
	}

	shadeDots_ = r_avertexnormal_dots[ ( ( int ) ( e->angles[ 1 ] * ( SHADEDOT_QUANT / 360.0f ) ) ) &
	                                   ( SHADEDOT_QUANT - 1 ) ];

	float angle       = e->angles[ 1 ] / 180.0f * Q_PI;
	shadeVector_[ 0 ] = std::cos( -angle );
	shadeVector_[ 1 ] = std::sin( -angle );
	shadeVector_[ 2 ] = 1.0f;
	VectorNormalize( shadeVector_ );
}

void chr::AliasModel::DrawFrameLerp( entity_t *e )
{
	// move should be the delta back to the previous frame * backlerp
	vec3_t delta;
	VectorSubtract( e->oldorigin, e->origin, delta );
	vec3_t vectors[ 3 ];
	AngleVectors( e->angles, vectors[ 0 ], vectors[ 1 ], vectors[ 2 ] );

	vec3_t move;
	move[ 0 ] = DotProduct( delta, vectors[ 0 ] ); // forward
	move[ 1 ] = -DotProduct( delta, vectors[ 1 ] );// left
	move[ 2 ] = DotProduct( delta, vectors[ 2 ] ); // up

	const Frame *frame    = &frames_[ e->frame ];
	const Frame *oldFrame = &frames_[ e->oldframe ];

	VectorAdd( move, oldFrame->translate, move );

	float frontlerp = 1.0f - e->backlerp;
	for ( uint i = 0; i < 3; i++ )
		move[ i ] = e->backlerp * move[ i ] + frontlerp * frame->translate[ i ];

	vec3_t frontv, backv;
	for ( uint i = 0; i < 3; i++ )
	{
		frontv[ i ] = frontlerp * frame->scale[ i ];
		backv[ i ]  = e->backlerp * oldFrame->scale[ i ];
	}

	const VertexGroup *v = &frame->vertices[ 0 ];
	LerpVertices( v, oldFrame->vertices.data(), frame->vertices.data(), lerpedVertices_.data(), move, frontv, backv );

	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
	{
		glDisable( GL_TEXTURE_2D );
	}
	else
	{
		image_t *texture;
		if ( e->model->skins[ 0 ] != nullptr )
		{
			texture = e->model->skins[ 0 ];
		}
		else
		{
			texture = r_notexture;
		}

		GL_Bind( texture->texnum );
	}

	GL_TexEnv( GL_MODULATE );

	float alpha = ( e->flags & RF_TRANSLUCENT ) ? e->alpha : 1.0f;

	int *order = &glCmds_[ 0 ];
	while ( true )
	{
		int vertIndex;

		// get the vertex count and primitive type
		int count = *order++;
		if ( !count ) break;// done
		if ( count < 0 )
		{
			count = -count;
			glBegin( GL_TRIANGLE_FAN );
		}
		else
		{
			glBegin( GL_TRIANGLE_STRIP );
		}

		if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE ) )
		{
			do {
				vertIndex = order[ 2 ];
				order += 3;

				glColor4f( shadeLight_[ 0 ], shadeLight_[ 1 ], shadeLight_[ 2 ], alpha );
				glVertex3f( lerpedVertices_[ vertIndex ].x, lerpedVertices_[ vertIndex ].y, lerpedVertices_[ vertIndex ].z );
			} while ( --count );
		}
		else
		{
			do {
				// texture coordinates come from the draw list
				glTexCoord2f( ( ( float * ) order )[ 0 ], ( ( float * ) order )[ 1 ] );
				vertIndex = order[ 2 ];
				order += 3;

				// normals and vertexes come from the frame list
				// I've botched this for now, as it doesn't appear to be correct anymore, plus we'll
				// be rewriting this anyway... ~hogsy
				float l = shadeDots_[ /*v[ vertIndex ].normalIndex*/ 0 ];

				glColor4f( l * shadeLight_[ 0 ], l * shadeLight_[ 1 ], l * shadeLight_[ 2 ], alpha );
				glVertex3f( lerpedVertices_[ vertIndex ].x, lerpedVertices_[ vertIndex ].y, lerpedVertices_[ vertIndex ].z );
			} while ( --count );
		}

		glEnd();
	}

	// Cleanup

	if ( e->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
	{
		glEnable( GL_TEXTURE_2D );
	}

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	GL_TexEnv( GL_REPLACE );

	c_alias_polys += numTriangles_;
}

void chr::AliasModel::Draw( entity_t *e )
{
	if ( e->frame >= numFrames_ || e->frame < 0 )
	{
		Com_Printf( "%s: no such frame %d\n", e->model->name, e->frame );
		e->frame = 0;
	}
	if ( ( e->oldframe >= numFrames_ ) || ( e->oldframe < 0 ) )
	{
		Com_Printf( "%s: no such oldframe %d\n", e->model->name, e->oldframe );
		e->oldframe = 0;
	}

	if ( !( e->flags & RF_WEAPONMODEL ) )
	{
		vec3_t bbox[ 8 ];
		if ( Cull( bbox, e ) )
			return;

#if 0
		glDisable( GL_CULL_FACE );
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glDisable( GL_TEXTURE_2D );
		glBegin( GL_TRIANGLE_STRIP );
		for ( uint i = 0; i < 8; i++ )
		{
			glVertex3fv( bbox[ i ] );
		}
		glEnd();
		glEnable( GL_TEXTURE_2D );
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glEnable( GL_CULL_FACE );
#endif
	}

	ApplyLighting( e );

	glPushMatrix();

	e->angles[ PITCH ] = -e->angles[ PITCH ];
	R_RotateForEntity( e );
	e->angles[ PITCH ] = -e->angles[ PITCH ];

	// hack the depth range to prevent
	// view model from poking into walls
	if ( e->flags & RF_DEPTHHACK ) glDepthRange( gldepthmin, gldepthmin + 0.3 * ( gldepthmax - gldepthmin ) );
	if ( e->flags & RF_TRANSLUCENT ) glEnable( GL_BLEND );

	if ( r_lerpmodels->value <= 0.0f )
	{
		e->backlerp = 0;
	}

	DrawFrameLerp( e );

	glPopMatrix();

	if ( e->flags & RF_TRANSLUCENT ) glDisable( GL_BLEND );
	if ( e->flags & RF_DEPTHHACK ) glDepthRange( gldepthmin, gldepthmax );
}

bool chr::AliasModel::Cull( vec3_t bbox[ 8 ], entity_t *e )
{
	Frame *frame    = &frames_[ e->frame ];
	Frame *oldFrame = &frames_[ e->oldframe ];

	vec3_t mins, maxs;
	VectorCopy( frame->bounds[ 0 ], mins );
	VectorCopy( frame->bounds[ 1 ], maxs );

	if ( frame != oldFrame )
	{
		for ( uint i = 0; i < 3; ++i )
		{
			if ( oldFrame->bounds[ 0 ][ i ] < mins[ i ] )
				mins[ i ] = oldFrame->bounds[ 0 ][ i ];
			if ( oldFrame->bounds[ 1 ][ i ] > maxs[ i ] )
				maxs[ i ] = oldFrame->bounds[ 1 ][ i ];
		}
	}

	// Compute a full bounding box
	for ( uint i = 0; i < 8; ++i )
	{
		vec3_t tmp;
		if ( i & 1 )
			tmp[ 0 ] = mins[ 0 ];
		else
			tmp[ 0 ] = maxs[ 0 ];

		if ( i & 2 )
			tmp[ 1 ] = mins[ 1 ];
		else
			tmp[ 1 ] = maxs[ 1 ];

		if ( i & 4 )
			tmp[ 2 ] = mins[ 2 ];
		else
			tmp[ 2 ] = maxs[ 2 ];

		VectorCopy( tmp, bbox[ i ] );
	}

	// Rotate the bounding box
	vec3_t angles;
	VectorCopy( e->angles, angles );
	angles[ YAW ] = -angles[ YAW ];
	vec3_t vectors[ 3 ];
	AngleVectors( angles, vectors[ 0 ], vectors[ 1 ], vectors[ 2 ] );

	for ( uint i = 0; i < 8; ++i )
	{
		vec3_t tmp;

		VectorCopy( bbox[ i ], tmp );

		bbox[ i ][ 0 ] = DotProduct( vectors[ 0 ], tmp );
		bbox[ i ][ 1 ] = -DotProduct( vectors[ 1 ], tmp );
		bbox[ i ][ 2 ] = DotProduct( vectors[ 2 ], tmp );

		VectorAdd( e->origin, bbox[ i ], bbox[ i ] );
	}

	int aggregatemask = ~0;
	for ( uint p = 0; p < 8; p++ )
	{
		int mask = 0;

		for ( uint f = 0; f < 4; f++ )
		{
			float dp = DotProduct( frustum[ f ].normal, bbox[ p ] );
			if ( ( dp - frustum[ f ].dist ) < 0 )
				mask |= ( 1 << f );
		}

		aggregatemask &= mask;
	}

	return ( bool ) aggregatemask;
}
