/*
Copyright (C) 1997-2001 Id Software, Inc.

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

*/
// r_main.c
#include <cctype>

#include "gl_local.h"

#include "model/model_alias.h"
#include "model/model_mda.h"

void R_Clear( void );

viddef_t vid;

model_t *r_worldmodel;

double gldepthmin, gldepthmax;

glconfig_t gl_config;
glstate_t  gl_state;

image_t *r_notexture;      // use for bad textures
image_t *r_particletexture;// little dot for particles

entity_t *currententity;
model_t  *currentmodel;

cplane_t frustum[ 4 ];

int r_visframecount;// bumped when going to a new PVS
int r_framecount;   // used for dlight push checking

int c_brush_polys, c_alias_polys;

float v_blend[ 4 ];// final blending color

void GL_Strings_f( void );

//
// view origin
//
vec3_t vup;
vec3_t vpn;
vec3_t vright;
vec3_t r_origin;

float r_world_matrix[ 16 ];
float r_base_world_matrix[ 16 ];

//
// screen size info
//
refdef_t r_newrefdef;

int r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_speeds;
cvar_t *r_fullbright;
cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_lerpmodels;
cvar_t *r_lefthand;

cvar_t *r_lightlevel;// FIXME: This is a HACK to get the client's light level

cvar_t *gl_nosubimage;
cvar_t *gl_allow_software;

cvar_t *gl_vertex_arrays;

cvar_t *gl_particle_min_size;
cvar_t *gl_particle_max_size;
cvar_t *gl_particle_size;
cvar_t *gl_particle_att_a;
cvar_t *gl_particle_att_b;
cvar_t *gl_particle_att_c;

cvar_t *gl_ext_swapinterval;
cvar_t *gl_ext_palettedtexture;
cvar_t *gl_ext_multitexture;
cvar_t *gl_ext_pointparameters;
cvar_t *gl_ext_compiled_vertex_array;

cvar_t *gl_log;
cvar_t *gl_bitdepth;
cvar_t *gl_drawbuffer;
cvar_t *gl_lightmap;
cvar_t *gl_shadows;
cvar_t *gl_mode;
cvar_t *gl_dynamic;
cvar_t *gl_monolightmap;
cvar_t *gl_modulate;
cvar_t *gl_nobind;
cvar_t *gl_round_down;
cvar_t *gl_picmip;
cvar_t *gl_skymip;
cvar_t *gl_showtris;
cvar_t *gl_ztrick;
cvar_t *gl_finish;
cvar_t *gl_clear;
cvar_t *gl_cull;
cvar_t *gl_polyblend;
cvar_t *gl_playermip;
cvar_t *gl_saturatelighting;
cvar_t *gl_swapinterval;
cvar_t *gl_texturemode;
cvar_t *gl_texturealphamode;
cvar_t *gl_texturesolidmode;
cvar_t *gl_lockpvs;

cvar_t *gl_3dlabs_broken;

cvar_t *vid_fullscreen;
cvar_t *vid_gamma;
cvar_t *vid_ref;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
bool R_CullBox( vec3_t mins, vec3_t maxs )
{
	int i;

	if ( r_nocull->value ) return false;

	for ( i = 0; i < 4; i++ )
		if ( BOX_ON_PLANE_SIDE( mins, maxs, &frustum[ i ] ) == 2 ) return true;
	return false;
}

void R_RotateForEntity( entity_t *e )
{
	glTranslatef( e->origin[ 0 ], e->origin[ 1 ], e->origin[ 2 ] );

	glRotatef( e->angles[ 1 ], 0, 0, 1 );
	glRotatef( -e->angles[ 0 ], 0, 1, 0 );
	glRotatef( -e->angles[ 2 ], 1, 0, 0 );
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel( entity_t *e )
{
	float        alpha = 1.0F;
	vec3_t       point;
	dsprframe_t *frame;
	float       *up, *right;
	dsprite_t   *psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = ( dsprite_t * ) currentmodel->extradata;

#if 0
	if( e->frame < 0 || e->frame >= psprite->numframes ) {
		Com_Printf( "no such sprite frame %i\n", e->frame );
		e->frame = 0;
	}
#endif
	e->frame %= psprite->numframes;

	frame = &psprite->frames[ e->frame ];

#if 0
	if( psprite->type == SPR_ORIENTED ) {	// bullet marks on walls
		vec3_t		v_forward, v_right, v_up;

		AngleVectors( currententity->angles, v_forward, v_right, v_up );
		up = v_up;
		right = v_right;
	} else
#endif
	{// normal sprite
		up    = vup;
		right = vright;
	}

	if ( e->flags & RF_TRANSLUCENT ) alpha = e->alpha;

	if ( alpha != 1.0F ) glEnable( GL_BLEND );

	glColor4f( 1, 1, 1, alpha );

	GL_Bind( currentmodel->skins[ e->frame ]->texnum );

	GL_TexEnv( GL_MODULATE );

	if ( alpha == 1.0 )
		glEnable( GL_ALPHA_TEST );
	else
		glDisable( GL_ALPHA_TEST );

	glBegin( GL_QUADS );

	glTexCoord2f( 0, 1 );
	VectorMA( e->origin, -frame->origin_y, up, point );
	VectorMA( point, -frame->origin_x, right, point );
	glVertex3fv( point );

	glTexCoord2f( 0, 0 );
	VectorMA( e->origin, frame->height - frame->origin_y, up, point );
	VectorMA( point, -frame->origin_x, right, point );
	glVertex3fv( point );

	glTexCoord2f( 1, 0 );
	VectorMA( e->origin, frame->height - frame->origin_y, up, point );
	VectorMA( point, frame->width - frame->origin_x, right, point );
	glVertex3fv( point );

	glTexCoord2f( 1, 1 );
	VectorMA( e->origin, -frame->origin_y, up, point );
	VectorMA( point, frame->width - frame->origin_x, right, point );
	glVertex3fv( point );

	glEnd();

	glDisable( GL_ALPHA_TEST );
	GL_TexEnv( GL_REPLACE );

	if ( alpha != 1.0F ) glDisable( GL_BLEND );

	glColor4f( 1, 1, 1, 1 );
}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel( void )
{
	vec3_t shadelight;

	if ( currententity->flags & RF_FULLBRIGHT )
		shadelight[ 0 ] = shadelight[ 1 ] = shadelight[ 2 ] = 1.0F;
	else
		R_LightPoint( currententity->origin, shadelight );

	glPushMatrix();
	R_RotateForEntity( currententity );

	glDisable( GL_TEXTURE_2D );
	glColor3fv( shadelight );

	glBegin( GL_TRIANGLE_FAN );
	glVertex3f( 0, 0, -16 );
	for ( int i = 0; i <= 4; i++ )
		glVertex3f( 16.0f * std::cos( ( float ) i * Q_PI / 2.0f ), 16.0f * std::sin( ( float ) i * Q_PI / 2.0f ), 0 );
	glEnd();

	glBegin( GL_TRIANGLE_FAN );
	glVertex3f( 0, 0, 16.0f );
	for ( int i = 4; i >= 0; i-- )
		glVertex3f( 16.0f * std::cos( ( float ) i * Q_PI / 2.0f ), 16.0f * std::sin( ( float ) i * Q_PI / 2.0f ), 0 );
	glEnd();

	glColor3f( 1, 1, 1 );
	glPopMatrix();
	glEnable( GL_TEXTURE_2D );
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList()
{
	if ( gl_showtris->value > 0.0f )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}

	if ( r_drawentities->value <= 0.0f ) return;

	// draw non-transparent first
	for ( int i = 0; i < r_newrefdef.num_entities; i++ )
	{
		currententity = &r_newrefdef.entities[ i ];
		if ( currententity->flags & RF_TRANSLUCENT ) continue;// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;
			if ( !currentmodel )
			{
				R_DrawNullModel();
				continue;
			}
			switch ( currentmodel->type )
			{
				case mod_alias:
					( ( chr::AliasModel * ) currentmodel->extradata )->Draw( currententity );
					break;
				case mod_mda:
					( ( chr::MDAModel * ) currentmodel->extradata )->Draw( currententity );
					break;
				case mod_brush:
					R_DrawBrushModel( currententity );
					break;
				case mod_sprite:
					R_DrawSpriteModel( currententity );
					break;
				default:
					R_DrawNullModel();
					break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	glDepthMask( 0 );// no z writes
	for ( int i = 0; i < r_newrefdef.num_entities; i++ )
	{
		currententity = &r_newrefdef.entities[ i ];
		if ( !( currententity->flags & RF_TRANSLUCENT ) ) continue;// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			if ( !currentmodel )
			{
				R_DrawNullModel();
				continue;
			}
			switch ( currentmodel->type )
			{
				case mod_alias:
					( ( chr::AliasModel * ) currentmodel->extradata )->Draw( currententity );
					break;
				case mod_mda:
					( ( chr::MDAModel * ) currentmodel->extradata )->Draw( currententity );
					break;
				case mod_brush:
					R_DrawBrushModel( currententity );
					break;
				case mod_sprite:
					R_DrawSpriteModel( currententity );
					break;
				default:
#if !defined( _DEBUG )
					Com_Error( ERR_DROP, "Bad modeltype" );
#endif
					break;
			}
		}
	}
	glDepthMask( 1 );// back to writing

	if ( gl_showtris->value > 0.0f )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}

void GL_DrawParticles( int num_particles, const particle_t particles[],
                       const unsigned colortable[ 768 ] )
{
	const particle_t *p;
	int               i;
	vec3_t            up, right;
	float             scale;
	byte              color[ 4 ];

	GL_Bind( r_particletexture->texnum );
	glDepthMask( GL_FALSE );// no z buffering
	glEnable( GL_BLEND );
	GL_TexEnv( GL_MODULATE );
	glBegin( GL_TRIANGLES );

	VectorScale( vup, 1.5, up );
	VectorScale( vright, 1.5, right );

	for ( p = particles, i = 0; i < num_particles; i++, p++ )
	{
		// hack a scale up to keep particles from disapearing
		scale = ( p->origin[ 0 ] - r_origin[ 0 ] ) * vpn[ 0 ] +
		        ( p->origin[ 1 ] - r_origin[ 1 ] ) * vpn[ 1 ] +
		        ( p->origin[ 2 ] - r_origin[ 2 ] ) * vpn[ 2 ];

		if ( scale < 20 )
			scale = 1;
		else
			scale = 1 + scale * 0.004;

		*( int * ) color = colortable[ p->color ];
		color[ 3 ]       = p->alpha * 255;

		glColor4ubv( color );

		glTexCoord2f( 0.0625, 0.0625 );
		glVertex3fv( p->origin );

		glTexCoord2f( 1.0625, 0.0625 );
		glVertex3f( p->origin[ 0 ] + up[ 0 ] * scale, p->origin[ 1 ] + up[ 1 ] * scale,
		            p->origin[ 2 ] + up[ 2 ] * scale );

		glTexCoord2f( 0.0625, 1.0625 );
		glVertex3f( p->origin[ 0 ] + right[ 0 ] * scale,
		            p->origin[ 1 ] + right[ 1 ] * scale,
		            p->origin[ 2 ] + right[ 2 ] * scale );
	}

	glEnd();
	glDisable( GL_BLEND );
	glColor4f( 1, 1, 1, 1 );
	glDepthMask( 1 );// back to normal Z buffering
	GL_TexEnv( GL_REPLACE );
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles( void )
{
	if ( gl_ext_pointparameters->value && glPointParameterfEXT )
	{
		int               i;
		unsigned char     color[ 4 ];
		const particle_t *p;

		glDepthMask( GL_FALSE );
		glEnable( GL_BLEND );
		glDisable( GL_TEXTURE_2D );

		glPointSize( gl_particle_size->value );

		glBegin( GL_POINTS );
		for ( i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles;
		      i++, p++ )
		{
			*( int * ) color = d_8to24table[ p->color ];
			color[ 3 ]       = p->alpha * 255;

			glColor4ubv( color );

			glVertex3fv( p->origin );
		}
		glEnd();

		glDisable( GL_BLEND );
		glColor4f( 1.0F, 1.0F, 1.0F, 1.0F );
		glDepthMask( GL_TRUE );
		glEnable( GL_TEXTURE_2D );
	}
	else
	{
		GL_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles,
		                  d_8to24table );
	}
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend( void )
{
	if ( !gl_polyblend->value ) return;
	if ( !v_blend[ 3 ] ) return;

	glDisable( GL_ALPHA_TEST );
	glEnable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_TEXTURE_2D );

	glLoadIdentity();

	// FIXME: get rid of these
	glRotatef( -90, 1, 0, 0 );// put Z going up
	glRotatef( 90, 0, 0, 1 ); // put Z going up

	glColor4fv( v_blend );

	glBegin( GL_QUADS );

	glVertex3f( 10, 100, 100 );
	glVertex3f( 10, -100, 100 );
	glVertex3f( 10, -100, -100 );
	glVertex3f( 10, 100, -100 );
	glEnd();

	glDisable( GL_BLEND );
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_ALPHA_TEST );

	glColor4f( 1, 1, 1, 1 );
}

//=======================================================================

int SignbitsForPlane( cplane_t *out )
{
	int bits, j;

	// for fast box on planeside test

	bits = 0;
	for ( j = 0; j < 3; j++ )
	{
		if ( out->normal[ j ] < 0 ) bits |= 1 << j;
	}
	return bits;
}

void R_SetFrustum( void )
{
	int i;

#if 0
	/*
	** this code is wrong, since it presume a 90 degree FOV both in the
	** horizontal and vertical plane
	*/
	// front side is visible
	VectorAdd( vpn, vright, frustum[ 0 ].normal );
	VectorSubtract( vpn, vright, frustum[ 1 ].normal );
	VectorAdd( vpn, vup, frustum[ 2 ].normal );
	VectorSubtract( vpn, vup, frustum[ 3 ].normal );

	// we theoretically don't need to normalize these vectors, but I do it
	// anyway so that debugging is a little easier
	VectorNormalize( frustum[ 0 ].normal );
	VectorNormalize( frustum[ 1 ].normal );
	VectorNormalize( frustum[ 2 ].normal );
	VectorNormalize( frustum[ 3 ].normal );
#else
	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[ 0 ].normal, vup, vpn,
	                         -( 90 - r_newrefdef.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[ 1 ].normal, vup, vpn,
	                         90 - r_newrefdef.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[ 2 ].normal, vright, vpn,
	                         90 - r_newrefdef.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[ 3 ].normal, vright, vpn,
	                         -( 90 - r_newrefdef.fov_y / 2 ) );
#endif

	for ( i = 0; i < 4; i++ )
	{
		frustum[ i ].type     = PLANE_ANYZ;
		frustum[ i ].dist     = DotProduct( r_origin, frustum[ i ].normal );
		frustum[ i ].signbits = SignbitsForPlane( &frustum[ i ] );
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame( void )
{
	int      i;
	mleaf_t *leaf;

	r_framecount++;

	// build the transformation matrix for the given view angles
	VectorCopy( r_newrefdef.vieworg, r_origin );

	AngleVectors( r_newrefdef.viewangles, vpn, vright, vup );

	// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster  = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf              = Mod_PointInLeaf( r_origin, r_worldmodel );
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if ( !leaf->contents )
		{// look down a bit
			vec3_t temp;

			VectorCopy( r_origin, temp );
			temp[ 2 ] -= 16;
			leaf = Mod_PointInLeaf( temp, r_worldmodel );
			if ( !( leaf->contents & CONTENTS_SOLID ) &&
			     ( leaf->cluster != r_viewcluster2 ) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{// look up a bit
			vec3_t temp;

			VectorCopy( r_origin, temp );
			temp[ 2 ] += 16;
			leaf = Mod_PointInLeaf( temp, r_worldmodel );
			if ( !( leaf->contents & CONTENTS_SOLID ) &&
			     ( leaf->cluster != r_viewcluster2 ) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	for ( i = 0; i < 4; i++ ) v_blend[ i ] = r_newrefdef.blend[ i ];

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
	{
		glEnable( GL_SCISSOR_TEST );
		glClearColor( 0.3, 0.3, 0.3, 1 );
		glScissor( r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y,
		           r_newrefdef.width, r_newrefdef.height );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glClearColor( 1, 0, 0.5, 0.5 );
		glDisable( GL_SCISSOR_TEST );
	}
}

void MYgluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear,
                       GLdouble zFar )
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL()
{
	float screenaspect;
	//	float	yfov;
	int x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x  = std::floor( r_newrefdef.x * vid.width / vid.width );
	x2 = std::ceil( ( r_newrefdef.x + r_newrefdef.width ) * vid.width / vid.width );
	y  = std::floor( vid.height - r_newrefdef.y * vid.height / vid.height );
	y2 = std::ceil( vid.height -
	                ( r_newrefdef.y + r_newrefdef.height ) * vid.height / vid.height );

	w = x2 - x;
	h = y - y2;

	glViewport( x, y2, w, h );

	//
	// set up projection matrix
	//
	screenaspect = ( float ) r_newrefdef.width / r_newrefdef.height;
	//	yfov = 2*atan((float)r_newrefdef.height/r_newrefdef.width)*180/M_PI;
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	MYgluPerspective( r_newrefdef.fov_y, screenaspect, 4, 9999 );

	glCullFace( GL_FRONT );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glRotatef( -90, 1, 0, 0 );// put Z going up
	glRotatef( 90, 0, 0, 1 ); // put Z going up
	glRotatef( -r_newrefdef.viewangles[ 2 ], 1, 0, 0 );
	glRotatef( -r_newrefdef.viewangles[ 0 ], 0, 1, 0 );
	glRotatef( -r_newrefdef.viewangles[ 1 ], 0, 0, 1 );
	glTranslatef( -r_newrefdef.vieworg[ 0 ], -r_newrefdef.vieworg[ 1 ],
	              -r_newrefdef.vieworg[ 2 ] );

	glGetFloatv( GL_MODELVIEW_MATRIX, r_world_matrix );

	//
	// set drawing parms
	//
	if ( gl_cull->value > 0.0f )
		glEnable( GL_CULL_FACE );
	else
		glDisable( GL_CULL_FACE );

	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );
	glEnable( GL_DEPTH_TEST );
}

/*
=============
R_Clear
=============
*/
void R_Clear()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glDepthFunc( GL_LEQUAL );
	glDepthRange( 0, 1 );
}

void R_Flash() { R_PolyBlend(); }

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView( refdef_t *fd )
{
	if ( r_norefresh->value > 0 ) return;

	r_newrefdef = *fd;

	if ( !r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		Com_Error( ERR_DROP, "R_RenderView: NULL worldmodel" );

	if ( r_speeds->value > 0 )
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights();

	if ( gl_finish->value > 0 ) glFinish();

	R_SetupFrame();

	R_SetFrustum();

	R_SetupGL();

	R_MarkLeaves();// done here so we know if we're in water

	Fog_SetState( true );

	R_DrawWorld();
	R_DrawEntitiesOnList();

	R_DrawParticles();
	R_DrawAlphaSurfaces();

	Fog_SetState( false );

	R_Flash();

	if ( r_speeds->value > 0 )
	{
		Com_Printf( "%4i wpoly %4i epoly %i tex %i lmaps\n",
		            c_brush_polys, c_alias_polys, c_visible_textures,
		            c_visible_lightmaps );
	}
}

void R_SetGL2D()
{
	// set 2D virtual screen size
	glViewport( 0, 0, vid.width, vid.height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, vid.width, vid.height, 0, -99999, 99999 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );
	glEnable( GL_ALPHA_TEST );
	glColor4f( 1, 1, 1, 1 );
}

/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel( void )
{
	vec3_t shadelight;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint( r_newrefdef.vieworg, shadelight );

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if ( shadelight[ 0 ] > shadelight[ 1 ] )
	{
		if ( shadelight[ 0 ] > shadelight[ 2 ] )
			r_lightlevel->value = 150 * shadelight[ 0 ];
		else
			r_lightlevel->value = 150 * shadelight[ 2 ];
	}
	else
	{
		if ( shadelight[ 1 ] > shadelight[ 2 ] )
			r_lightlevel->value = 150 * shadelight[ 1 ];
		else
			r_lightlevel->value = 150 * shadelight[ 2 ];
	}
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame( refdef_t *fd )
{
	R_RenderView( fd );
	R_SetLightLevel();
	R_SetGL2D();
}

void R_Register( void )
{
	r_lefthand     = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh    = Cvar_Get( "r_norefresh", "0", 0 );
	r_fullbright   = Cvar_Get( "r_fullbright", "0", 0 );
	r_drawentities = Cvar_Get( "r_drawentities", "1", 0 );
	r_drawworld    = Cvar_Get( "r_drawworld", "1", 0 );
	r_novis        = Cvar_Get( "r_novis", "0", 0 );
	r_nocull       = Cvar_Get( "r_nocull", "0", 0 );
	r_lerpmodels   = Cvar_Get( "r_lerpmodels", "1", 0 );
	r_speeds       = Cvar_Get( "r_speeds", "0", 0 );

	r_lightlevel = Cvar_Get( "r_lightlevel", "0", 0 );

	gl_nosubimage     = Cvar_Get( "gl_nosubimage", "0", 0 );
	gl_allow_software = Cvar_Get( "gl_allow_software", "0", 0 );

	gl_particle_min_size = Cvar_Get( "gl_particle_min_size", "2", CVAR_ARCHIVE );
	gl_particle_max_size =
	        Cvar_Get( "gl_particle_max_size", "40", CVAR_ARCHIVE );
	gl_particle_size  = Cvar_Get( "gl_particle_size", "40", CVAR_ARCHIVE );
	gl_particle_att_a = Cvar_Get( "gl_particle_att_a", "0.01", CVAR_ARCHIVE );
	gl_particle_att_b = Cvar_Get( "gl_particle_att_b", "0.0", CVAR_ARCHIVE );
	gl_particle_att_c = Cvar_Get( "gl_particle_att_c", "0.01", CVAR_ARCHIVE );

	gl_modulate     = Cvar_Get( "gl_modulate", "1", CVAR_ARCHIVE );
	gl_log          = Cvar_Get( "gl_log", "0", 0 );
	gl_bitdepth     = Cvar_Get( "gl_bitdepth", "0", 0 );
	gl_mode         = Cvar_Get( "gl_mode", "3", CVAR_ARCHIVE );
	gl_lightmap     = Cvar_Get( "gl_lightmap", "0", 0 );
	gl_shadows      = Cvar_Get( "gl_shadows", "0", CVAR_ARCHIVE );
	gl_dynamic      = Cvar_Get( "gl_dynamic", "1", 0 );
	gl_nobind       = Cvar_Get( "gl_nobind", "0", 0 );
	gl_round_down   = Cvar_Get( "gl_round_down", "1", 0 );
	gl_picmip       = Cvar_Get( "gl_picmip", "0", 0 );
	gl_skymip       = Cvar_Get( "gl_skymip", "0", 0 );
	gl_showtris     = Cvar_Get( "gl_showtris", "0", 0 );
	gl_ztrick       = Cvar_Get( "gl_ztrick", "0", 0 );
	gl_finish       = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE );
	gl_clear        = Cvar_Get( "gl_clear", "0", 0 );
	gl_cull         = Cvar_Get( "gl_cull", "1", 0 );
	gl_polyblend    = Cvar_Get( "gl_polyblend", "1", 0 );
	gl_playermip    = Cvar_Get( "gl_playermip", "0", 0 );
	gl_monolightmap = Cvar_Get( "gl_monolightmap", "0", 0 );
	gl_texturemode =
	        Cvar_Get( "gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE );
	gl_texturealphamode =
	        Cvar_Get( "gl_texturealphamode", "default", CVAR_ARCHIVE );
	gl_texturesolidmode =
	        Cvar_Get( "gl_texturesolidmode", "default", CVAR_ARCHIVE );
	gl_lockpvs = Cvar_Get( "gl_lockpvs", "0", 0 );

	gl_vertex_arrays = Cvar_Get( "gl_vertex_arrays", "0", CVAR_ARCHIVE );

	gl_ext_swapinterval = Cvar_Get( "gl_ext_swapinterval", "1", CVAR_ARCHIVE );
	gl_ext_palettedtexture =
	        Cvar_Get( "gl_ext_palettedtexture", "1", CVAR_ARCHIVE );
	gl_ext_multitexture = Cvar_Get( "gl_ext_multitexture", "1", CVAR_ARCHIVE );
	gl_ext_pointparameters =
	        Cvar_Get( "gl_ext_pointparameters", "1", CVAR_ARCHIVE );
	gl_ext_compiled_vertex_array =
	        Cvar_Get( "gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );

	gl_drawbuffer   = Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );
	gl_swapinterval = Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE );

	gl_saturatelighting = Cvar_Get( "gl_saturatelighting", "0", 0 );

	gl_3dlabs_broken = Cvar_Get( "gl_3dlabs_broken", "1", CVAR_ARCHIVE );

	vid_fullscreen = Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	vid_gamma      = Cvar_Get( "vid_gamma", "1.0", CVAR_ARCHIVE );
	vid_ref        = Cvar_Get( "vid_ref", "gl", CVAR_ARCHIVE );

	Cmd_AddCommand( "imagelist", GL_ImageList_f );
	Cmd_AddCommand( "screenshot", GL_ScreenShot_f );
	Cmd_AddCommand( "modellist", Mod_Modellist_f );
	Cmd_AddCommand( "gl_strings", GL_Strings_f );
}

/*
==================
R_SetMode
==================
*/
bool R_SetMode( void )
{
#if 1// simplified until we get SDL2 implemented
	rserr_t err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->value, false );

	vid_fullscreen->modified = false;
	gl_mode->modified        = false;

	return ( err == rserr_ok );
#else
	rserr_t err;
	bool    fullscreen;

	if ( vid_fullscreen->modified && !gl_config.allow_cds )
	{
		Com_Printf( "R_SetMode() - CDS not allowed with this driver\n" );
		ri.Cvar_SetValue( "vid_fullscreen", !vid_fullscreen->value );
		vid_fullscreen->modified = false;
	}

	fullscreen = vid_fullscreen->value;

	vid_fullscreen->modified = false;
	gl_mode->modified        = false;

	if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->value,
	                            fullscreen ) ) == rserr_ok )
	{
		gl_state.prev_mode = gl_mode->value;
	}
	else
	{
		if ( err == rserr_invalid_fullscreen )
		{
			ri.Cvar_SetValue( "vid_fullscreen", 0 );
			vid_fullscreen->modified = false;
			Com_Printf( "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->value,
			                            false ) ) == rserr_ok )
				return true;
		}
		else if ( err == rserr_invalid_mode )
		{
			ri.Cvar_SetValue( "gl_mode", gl_state.prev_mode );
			gl_mode->modified = false;
			Com_Printf( "ref_gl::R_SetMode() - invalid mode\n" );
		}

		// try setting it back to something safe
		if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_state.prev_mode,
		                            false ) ) != rserr_ok )
		{
			Com_Printf( "ref_gl::R_SetMode() - could not revert to safe mode\n" );
			return false;
		}
	}
	return true;
#endif
}

/*
===============
R_Init
===============
*/
int R_Init()
{
	char         renderer_buffer[ 1000 ];
	char         vendor_buffer[ 1000 ];
	int          err;
	int          j;
	extern float r_turbsin[ 256 ];

	for ( j = 0; j < 256; j++ )
	{
		r_turbsin[ j ] *= 0.5;
	}

	Draw_GetPalette();

	R_Register();

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	if ( !R_SetMode() )
	{
		Com_Printf( "ref_gl::R_Init() - could not R_SetMode()\n" );
		return -1;
	}

	// initialize our QGL dynamic bindings
	if ( !QGL_Init() )
	{
		Com_Printf( "Failed to initialize GL interface!\n" );
		return -1;
	}

	VID_MenuInit();

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = ( const char * ) glGetString( GL_VENDOR );
	Com_DPrintf( "GL_VENDOR: %s\n", gl_config.vendor_string );
	gl_config.renderer_string = ( const char * ) glGetString( GL_RENDERER );
	Com_DPrintf( "GL_RENDERER: %s\n", gl_config.renderer_string );
	gl_config.version_string = ( const char * ) glGetString( GL_VERSION );
	Com_DPrintf( "GL_VERSION: %s\n", gl_config.version_string );
	gl_config.extensions_string = ( const char * ) glGetString( GL_EXTENSIONS );
	Com_DPrintf( "GL_EXTENSIONS: %s\n", gl_config.extensions_string );

	strcpy( renderer_buffer, gl_config.renderer_string );
	Q_strtolower( renderer_buffer );

	strcpy( vendor_buffer, gl_config.vendor_string );
	Q_strtolower( vendor_buffer );

	if ( strstr( renderer_buffer, "gdi" ) )
		gl_config.renderer = GL_RENDERER_MCD;
	else if ( strstr( renderer_buffer, "verite" ) )
		gl_config.renderer = GL_RENDERER_RENDITION;

	if ( toupper( gl_monolightmap->string[ 1 ] ) != 'F' )
	{
		Cvar_Set( "gl_monolightmap", "0" );
	}

	// power vr can't have anything stay in the framebuffer, so
	// the screen needs to redraw the tiled background every frame
	Cvar_Set( "scr_drawall", "0" );

	// MCD has buffering issues
	if ( gl_config.renderer == GL_RENDERER_MCD )
	{
		Cvar_SetValue( "gl_finish", 1 );
	}

	gl_config.allow_cds = true;

	if ( gl_config.allow_cds )
		Com_Printf( "...allowing CDS\n" );
	else
		Com_Printf( "...disabling CDS\n" );

	GL_SetDefaultState();

	/*
	** draw our stereo patterns
	*/

	GL_InitImages();
	Mod_Init();
	R_InitParticleTexture();
	Draw_InitLocal();

	err = glGetError();
	if ( err != GL_NO_ERROR )
		Com_Printf( "glGetError() = 0x%x\n", err );

	return rserr_ok;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown( void )
{
	Cmd_RemoveCommand( "modellist" );
	Cmd_RemoveCommand( "screenshot" );
	Cmd_RemoveCommand( "imagelist" );
	Cmd_RemoveCommand( "gl_strings" );

	Mod_FreeAll();

	// Make sure that we stop logging everything!
	GLimp_EnableLogging( false );

	GL_ShutdownImages();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();
}

void R_BeginFrame()
{
	/*
	** change modes if necessary
	*/
	if ( gl_mode->modified ||
	     vid_fullscreen->modified )
	{// FIXME: only restart if CDS is required
		cvar_t *ref;

		ref           = Cvar_Get( "vid_ref", "gl", 0 );
		ref->modified = true;
	}

	if ( gl_log->modified )
	{
		GLimp_EnableLogging( gl_log->value );
		gl_log->modified = false;
	}

	if ( gl_log->value )
	{
		GLimp_LogNewFrame();
	}

	glDrawBuffer( GL_BACK );

	/*
	** go into 2D mode
	*/
	glViewport( 0, 0, vid.width, vid.height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, vid.width, vid.height, 0, -99999, 99999 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );
	glEnable( GL_ALPHA_TEST );
	glColor4f( 1, 1, 1, 1 );

	/*
	** draw buffer stuff
	*/
	if ( gl_drawbuffer->modified )
	{
		gl_drawbuffer->modified = false;
		if ( Q_stricmp( gl_drawbuffer->string, "GL_FRONT" ) == 0 )
			glDrawBuffer( GL_FRONT );
		else
			glDrawBuffer( GL_BACK );
	}

	/*
	** texturemode stuff
	*/
	if ( gl_texturemode->modified )
	{
		GL_TextureMode( gl_texturemode->string );
		gl_texturemode->modified = false;
	}

	if ( gl_texturealphamode->modified )
	{
		GL_TextureAlphaMode( gl_texturealphamode->string );
		gl_texturealphamode->modified = false;
	}

	if ( gl_texturesolidmode->modified )
	{
		GL_TextureSolidMode( gl_texturesolidmode->string );
		gl_texturesolidmode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[ 256 ];

void R_SetPalette( const unsigned char *palette )
{
	int i;

	byte *rp = ( byte * ) r_rawpalette;

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[ i * 4 + 0 ] = palette[ i * 3 + 0 ];
			rp[ i * 4 + 1 ] = palette[ i * 3 + 1 ];
			rp[ i * 4 + 2 ] = palette[ i * 3 + 2 ];
			rp[ i * 4 + 3 ] = 0xff;
		}
	}
	else
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[ i * 4 + 0 ] = d_8to24table[ i ] & 0xff;
			rp[ i * 4 + 1 ] = ( d_8to24table[ i ] >> 8 ) & 0xff;
			rp[ i * 4 + 2 ] = ( d_8to24table[ i ] >> 16 ) & 0xff;
			rp[ i * 4 + 3 ] = 0xff;
		}
	}
	GL_SetTexturePalette( r_rawpalette );

	glClearColor( 0, 0, 0, 0 );
	glClear( GL_COLOR_BUFFER_BIT );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
}

/*
** R_DrawBeam
*/
void R_DrawBeam( entity_t *e )
{
#define NUM_BEAM_SEGS 6

	int   i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t start_points[ NUM_BEAM_SEGS ], end_points[ NUM_BEAM_SEGS ];
	vec3_t oldorigin, origin;

	oldorigin[ 0 ] = e->oldorigin[ 0 ];
	oldorigin[ 1 ] = e->oldorigin[ 1 ];
	oldorigin[ 2 ] = e->oldorigin[ 2 ];

	origin[ 0 ] = e->origin[ 0 ];
	origin[ 1 ] = e->origin[ 1 ];
	origin[ 2 ] = e->origin[ 2 ];

	normalized_direction[ 0 ] = direction[ 0 ] = oldorigin[ 0 ] - origin[ 0 ];
	normalized_direction[ 1 ] = direction[ 1 ] = oldorigin[ 1 ] - origin[ 1 ];
	normalized_direction[ 2 ] = direction[ 2 ] = oldorigin[ 2 ] - origin[ 2 ];

	if ( VectorNormalize( normalized_direction ) == 0 ) return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[ i ], normalized_direction, perpvec,
		                         ( 360.0 / NUM_BEAM_SEGS ) * i );
		VectorAdd( start_points[ i ], origin, start_points[ i ] );
		VectorAdd( start_points[ i ], direction, end_points[ i ] );
	}

	glDisable( GL_TEXTURE_2D );
	glEnable( GL_BLEND );
	glDepthMask( GL_FALSE );

	r = ( d_8to24table[ e->skinnum & 0xFF ] ) & 0xFF;
	g = ( d_8to24table[ e->skinnum & 0xFF ] >> 8 ) & 0xFF;
	b = ( d_8to24table[ e->skinnum & 0xFF ] >> 16 ) & 0xFF;

	r *= 1 / 255.0F;
	g *= 1 / 255.0F;
	b *= 1 / 255.0F;

	glColor4f( r, g, b, e->alpha );

	glBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		glVertex3fv( start_points[ i ] );
		glVertex3fv( end_points[ i ] );
		glVertex3fv( start_points[ ( i + 1 ) % NUM_BEAM_SEGS ] );
		glVertex3fv( end_points[ ( i + 1 ) % NUM_BEAM_SEGS ] );
	}
	glEnd();

	glEnable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );
	glDepthMask( GL_TRUE );
}
