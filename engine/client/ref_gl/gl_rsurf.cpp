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

// GL_RSURF.C: surface-related refresh code

#include "gl_local.h"

static vec3_t	modelorg;		// relative to viewpoint

msurface_t *r_alpha_surfaces;

#define LIGHTMAP_BYTES 4

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#define	MAX_LIGHTMAPS	128

int		c_visible_lightmaps;
int		c_visible_textures;

#define GL_LIGHTMAP_FORMAT GL_RGBA

typedef struct {
	int internal_format;
	int	current_lightmap_texture;

	msurface_t *lightmap_surfaces[ MAX_LIGHTMAPS ];

	int			allocated[ BLOCK_WIDTH ];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmap_buffer[ 4 * BLOCK_WIDTH * BLOCK_HEIGHT ];
} gllightmapstate_t;
static gllightmapstate_t gl_lms;

static void LM_InitBlock();
static void LM_UploadBlock();
static bool LM_AllocBlock( int w, int h, int *x, int *y );

extern void R_SetCacheState( msurface_t *surf );
extern void R_BuildLightMap( msurface_t *surf, byte *dest, int stride );

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t *R_TextureAnimation( mtexinfo_t *tex ) {
	int		c;

	if( !tex->next )
		return tex->image;

	c = currententity->frame % tex->numframes;
	while( c ) {
		tex = tex->next;
		c--;
	}

	return tex->image;
}

/*
================
DrawGLPoly
================
*/
void DrawGLPoly( glpoly_t *p ) {
	int		i;
	float *v;

	glBegin( GL_POLYGON );
	v = p->verts[ 0 ];
	for( i = 0; i < p->numverts; i++, v += VERTEXSIZE ) {
		glTexCoord2f( v[ 3 ], v[ 4 ] );
		glVertex3fv( v );
	}
	glEnd();
}

//============
//PGM
/*
================
DrawGLFlowingPoly -- version of DrawGLPoly that handles scrolling texture
================
*/
void DrawGLFlowingPoly( msurface_t *fa ) {
	int		i;
	float *v;
	glpoly_t *p;
	float	scroll;

	p = fa->polys;

	scroll = -64.0f * ( ( r_newrefdef.time / 40.0f ) - (int)( r_newrefdef.time / 40.0f ) );
	if( scroll == 0.0f )
		scroll = -64.0f;

	glBegin( GL_POLYGON );
	v = p->verts[ 0 ];
	for( i = 0; i < p->numverts; i++, v += VERTEXSIZE ) {
		glTexCoord2f( ( v[ 3 ] + scroll ), v[ 4 ] );
		glVertex3fv( v );
	}
	glEnd();
}
//PGM
//============

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly( msurface_t *fa ) {
	int			maps;
	image_t *image;
	qboolean is_dynamic = false;

	c_brush_polys++;

	image = R_TextureAnimation( fa->texinfo );

	if( fa->flags & SURF_DRAWTURB ) {
		GL_Bind( image->texnum );

		// warp texture, no lightmaps
		GL_TexEnv( GL_MODULATE );
		glColor4f( gl_state.inverse_intensity,
			gl_state.inverse_intensity,
			gl_state.inverse_intensity,
			1.0F );
		EmitWaterPolys( fa );
		GL_TexEnv( GL_REPLACE );

		return;
	} else {
		GL_Bind( image->texnum );

		GL_TexEnv( GL_REPLACE );
	}

	//======
	//PGM
	if( fa->texinfo->flags & SURF_FLOWING )
		DrawGLFlowingPoly( fa );
	else
		DrawGLPoly( fa->polys );
	//PGM
	//======

		/*
		** check for lightmap modification
		*/
	for( maps = 0; maps < MAXLIGHTMAPS && fa->styles[ maps ] != 255; maps++ ) {
		if( r_newrefdef.lightstyles[ fa->styles[ maps ] ].white != fa->cached_light[ maps ] )
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if( ( fa->dlightframe == r_framecount ) ) {
	dynamic:
		if( gl_dynamic->value ) {
			if( !( fa->texinfo->flags & ( SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP ) ) ) {
				is_dynamic = true;
			}
		}
	}

	if( is_dynamic ) {
		if( ( fa->styles[ maps ] >= 32 || fa->styles[ maps ] == 0 ) && ( fa->dlightframe != r_framecount ) ) {
			unsigned	temp[ 34 * 34 ];
			int			smax, tmax;

			smax = ( fa->extents[ 0 ] >> 4 ) + 1;
			tmax = ( fa->extents[ 1 ] >> 4 ) + 1;

			R_BuildLightMap( fa, (byte *)temp, smax * 4 );
			R_SetCacheState( fa );

			GL_Bind( gl_state.lightmap_textures + fa->lightmaptexturenum );

			glTexSubImage2D( GL_TEXTURE_2D, 0,
				fa->light_s, fa->light_t,
				smax, tmax,
				GL_LIGHTMAP_FORMAT,
				GL_UNSIGNED_BYTE, temp );

			fa->lightmapchain = gl_lms.lightmap_surfaces[ fa->lightmaptexturenum ];
			gl_lms.lightmap_surfaces[ fa->lightmaptexturenum ] = fa;
		} else {
			fa->lightmapchain = gl_lms.lightmap_surfaces[ 0 ];
			gl_lms.lightmap_surfaces[ 0 ] = fa;
		}
	} else {
		fa->lightmapchain = gl_lms.lightmap_surfaces[ fa->lightmaptexturenum ];
		gl_lms.lightmap_surfaces[ fa->lightmaptexturenum ] = fa;
	}
}

static void GL_RenderLightmappedPoly( msurface_t *surf );

/*
Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
*/
void R_DrawAlphaSurfaces()
{
	// go back to the world matrix
	glLoadMatrixf( r_world_matrix );

	glEnable( GL_BLEND );

	GL_EnableMultitexture( true );

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	float intens = gl_state.inverse_intensity;

	// todo: consider moving water onto their own list?

	for ( msurface_t *s = r_alpha_surfaces; s; s = s->texturechain )
	{
		c_brush_polys++;
		if ( s->texinfo->flags & SURF_TRANS33 )
			glColor4f( intens, intens, intens, 0.33f );
		else if ( s->texinfo->flags & SURF_TRANS66 )
			glColor4f( intens, intens, intens, 0.66f );
		else
			glColor4f( intens, intens, intens, 1.0f );

		if ( s->flags & SURF_DRAWTURB )
		{
			GL_EnableMultitexture( false );
			EmitWaterPolys( s );
			GL_EnableMultitexture( true );
			continue;
		}

		GL_RenderLightmappedPoly( s );
	}

	GL_EnableMultitexture( false );

	GL_TexEnv( GL_REPLACE );
	glColor4f( 1, 1, 1, 1 );
	glDisable( GL_BLEND );

	r_alpha_surfaces = nullptr;
}

static void GL_RenderLightmappedPoly( msurface_t *surf )
{
	bool is_dynamic = false;

	int map;
	for ( map = 0; map < MAXLIGHTMAPS && surf->styles[ map ] != 255; map++ )
	{
		if ( r_newrefdef.lightstyles[ surf->styles[ map ] ].white != surf->cached_light[ map ] )
		{
			goto dynamic;
		}
	}

	// dynamic this frame or dynamic previously
	if ( surf->dlightframe == r_framecount  )
	{
	dynamic:
		if ( gl_dynamic->value > 0.0f )
		{
			if ( !( surf->texinfo->flags & ( SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP ) ) )
			{
				is_dynamic = true;
			}
		}
	}

	int lmtex = surf->lightmaptexturenum;
	if ( is_dynamic )
	{
		unsigned temp[ 128 * 128 ];
		int smax, tmax;

		if ( ( surf->styles[ map ] >= 32 || surf->styles[ map ] == 0 ) && ( surf->dlightframe != r_framecount ) )
		{
			smax = ( surf->extents[ 0 ] >> 4 ) + 1;
			tmax = ( surf->extents[ 1 ] >> 4 ) + 1;

			R_BuildLightMap( surf, ( byte * ) temp, smax * 4 );
			R_SetCacheState( surf );

			GL_MBind( GL_TEXTURE1, gl_state.lightmap_textures + surf->lightmaptexturenum );

			lmtex = surf->lightmaptexturenum;
		}
		else
		{
			smax = ( surf->extents[ 0 ] >> 4 ) + 1;
			tmax = ( surf->extents[ 1 ] >> 4 ) + 1;

			R_BuildLightMap( surf, ( byte * ) temp, smax * 4 );

			GL_MBind( GL_TEXTURE1, gl_state.lightmap_textures + 0 );

			lmtex = 0;
		}

		glTexSubImage2D( GL_TEXTURE_2D, 0,
		                 surf->light_s, surf->light_t,
		                 smax, tmax,
		                 GL_LIGHTMAP_FORMAT,
		                 GL_UNSIGNED_BYTE, temp );
	}

	c_brush_polys++;

	image_t *image = R_TextureAnimation( surf->texinfo );
	GL_MBind( GL_TEXTURE0, image->texnum );
	GL_MBind( GL_TEXTURE1, gl_state.lightmap_textures + lmtex );

	float scroll = 0.0f;
	if ( surf->texinfo->flags & SURF_FLOWING )
	{
		scroll = -64.0f * ( ( r_newrefdef.time / 40.0f ) - ( int ) ( r_newrefdef.time / 40.0f ) );
		if ( scroll == 0.0f )
		{
			scroll = -64.0f;
		}
	}

	for ( glpoly_t *p = surf->polys; p; p = p->chain )
	{
		float *v = p->verts[ 0 ];
		glBegin( GL_POLYGON );
		for ( int i = 0; i < surf->polys->numverts; i++, v += VERTEXSIZE )
		{
			glMultiTexCoord2f( GL_TEXTURE0, ( v[ 3 ] + scroll ), v[ 4 ] );
			glMultiTexCoord2f( GL_TEXTURE1, v[ 5 ], v[ 6 ] );
			glVertex3fv( v );
		}
		glEnd();
	}
}

/*
=================
R_DrawInlineBModel
=================
*/
void R_DrawInlineBModel() {
	cplane_t *pplane;
	float		dot;
	dlight_t *lt;

	// calculate dynamic lighting for bmodel
	if( gl_flashblend->value <= 0.0f ) {
		lt = r_newrefdef.dlights;
		for( int k = 0; k < r_newrefdef.num_dlights; k++, lt++ ) {
			R_MarkLights( lt, 1 << k, currentmodel->nodes + currentmodel->firstnode );
		}
	}

	if( currententity->flags & RF_TRANSLUCENT ) {
		glEnable( GL_BLEND );
		glColor4f( 1, 1, 1, 0.25 );
		GL_TexEnv( GL_MODULATE );
	}

	//
	// draw texture
	//
	msurface_t *psurf = &currentmodel->surfaces[ currentmodel->firstmodelsurface ];
	for( int i = 0; i < currentmodel->nummodelsurfaces; i++, psurf++ ) {
		// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct( modelorg, pplane->normal ) - pplane->dist;

		// draw the polygon
		if ( ( ( psurf->flags & SURF_PLANEBACK ) && ( dot < -BACKFACE_EPSILON ) ) ||
		     ( !( psurf->flags & SURF_PLANEBACK ) && ( dot > BACKFACE_EPSILON ) ) )
		{
			if ( psurf->texinfo->flags & ( SURF_TRANS33 |
			                               SURF_TRANS66 |
			                               SURF_ALPHA_BANNER |
			                               SURF_ALPHA_TEST ) ||
			     psurf->texinfo->image->has_alpha )
			{
				// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
				continue;
			}

			if ( !( psurf->flags & SURF_DRAWTURB ) )
			{
				GL_RenderLightmappedPoly( psurf );
				continue;
			}

			GL_EnableMultitexture( false );
			R_RenderBrushPoly( psurf );
			GL_EnableMultitexture( true );
		}
	}

	if( ( currententity->flags & RF_TRANSLUCENT ) ) {
		glDisable( GL_BLEND );
		glColor4f( 1, 1, 1, 1 );
		GL_TexEnv( GL_REPLACE );
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel( entity_t *e ) {
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if( currentmodel->nummodelsurfaces == 0 )
		return;

	currententity = e;
	gl_state.currenttextures[ 0 ] = gl_state.currenttextures[ 1 ] = -1;

	if( e->angles[ 0 ] > 0.0f || e->angles[ 1 ] > 0.0f || e->angles[ 2 ] > 0.0f ) {
		rotated = true;
		for( i = 0; i < 3; i++ ) {
			mins[ i ] = e->origin[ i ] - currentmodel->radius;
			maxs[ i ] = e->origin[ i ] + currentmodel->radius;
		}
	} else {
		rotated = false;
		VectorAdd( e->origin, currentmodel->mins, mins );
		VectorAdd( e->origin, currentmodel->maxs, maxs );
	}

	if( R_CullBox( mins, maxs ) )
		return;

	glColor3f( 1, 1, 1 );
	memset( gl_lms.lightmap_surfaces, 0, sizeof( gl_lms.lightmap_surfaces ) );

	VectorSubtract( r_newrefdef.vieworg, e->origin, modelorg );
	if( rotated ) {
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy( modelorg, temp );
		AngleVectors( e->angles, forward, right, up );
		modelorg[ 0 ] = DotProduct( temp, forward );
		modelorg[ 1 ] = -DotProduct( temp, right );
		modelorg[ 2 ] = DotProduct( temp, up );
	}

	glPushMatrix();
	e->angles[ 0 ] = -e->angles[ 0 ];	// stupid quake bug
	e->angles[ 2 ] = -e->angles[ 2 ];	// stupid quake bug
	R_RotateForEntity( e );
	e->angles[ 0 ] = -e->angles[ 0 ];	// stupid quake bug
	e->angles[ 2 ] = -e->angles[ 2 ];	// stupid quake bug

	GL_EnableMultitexture( true );

	R_DrawInlineBModel();

	GL_EnableMultitexture( false );

	glPopMatrix();
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

void R_RecursiveWorldNode( mnode_t *node ) {
	int			c, side, sidebit;
	cplane_t *plane;
	msurface_t *surf, **mark;
	mleaf_t *pleaf;
	float		dot;
	image_t *image;

	if( node->contents == CONTENTS_SOLID )
		return;		// solid

	if( node->visframe != r_visframecount )
		return;
	if( R_CullBox( node->minmaxs, node->minmaxs + 3 ) )
		return;

	// if a leaf node, draw stuff
	if( node->contents != -1 ) {
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if( r_newrefdef.areabits ) {
			if( !( r_newrefdef.areabits[ pleaf->area >> 3 ] & ( 1 << ( pleaf->area & 7 ) ) ) )
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if( c ) {
			do {
				( *mark )->visframe = r_framecount;
				mark++;
			} while( --c );
		}

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	plane = node->plane;

	switch( plane->type ) {
	case PLANE_X:
		dot = modelorg[ 0 ] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[ 1 ] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[ 2 ] - plane->dist;
		break;
	default:
		dot = DotProduct( modelorg, plane->normal ) - plane->dist;
		break;
	}

	if( dot >= 0 ) {
		side = 0;
		sidebit = 0;
	} else {
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveWorldNode( node->children[ side ] );

	// draw stuff
	for( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c; c--, surf++ ) {
		if( surf->visframe != r_framecount )
			continue;

		if( ( surf->flags & SURF_PLANEBACK ) != sidebit )
			continue;		// wrong side

		if ( surf->texinfo->flags & SURF_SKY )
		{
			// just adds to visible sky bounds
			R_AddSkySurface( surf );
		}
		else if ( surf->texinfo->flags & ( SURF_TRANS33 |
		                                   SURF_TRANS66 |
		                                   SURF_ALPHA_BANNER |
		                                   SURF_ALPHA_TEST ) ||
		          surf->texinfo->image->has_alpha )
		{
			// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else
		{
			if ( !( surf->flags & SURF_DRAWTURB ) )
			{
				GL_RenderLightmappedPoly( surf );
			}
			else
			{
				// the polygon is visible, so add it to the texture
				// sorted chain
				// FIXME: this is a hack for animation
				image = R_TextureAnimation( surf->texinfo );
				surf->texturechain = image->texturechain;
				image->texturechain = surf;
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode( node->children[ !side ] );
}

void R_DrawWorld()
{
	if ( r_drawworld->value <= 0.0f || ( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		return;

	currentmodel = r_worldmodel;

	VectorCopy( r_newrefdef.vieworg, modelorg );

	// auto cycle the world frame for texture animation
	entity_t ent;
	memset( &ent, 0, sizeof( ent ) );
	ent.frame = ( int ) ( r_newrefdef.time * 2 );
	currententity = &ent;

	gl_state.currenttextures[ 0 ] = gl_state.currenttextures[ 1 ] = -1;

	glColor3f( 1, 1, 1 );
	memset( gl_lms.lightmap_surfaces, 0, sizeof( gl_lms.lightmap_surfaces ) );
	R_ClearSkyBox();

	if ( gl_showtris->value > 0.0f )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}

	GL_EnableMultitexture( true );

	R_RecursiveWorldNode( r_worldmodel->nodes );

	GL_EnableMultitexture( false );

	R_DrawSkyBox();

	if ( gl_showtris->value > 0.0f )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}

/*
** Mark the leaves and nodes that are in the PVS for the current
** cluster
*/
void R_MarkLeaves() {
	byte *vis;
	byte	fatvis[ MAX_MAP_LEAFS / 8 ];
	mnode_t *node;
	int		i, c;
	mleaf_t *leaf;
	int		cluster;

	if( r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1 )
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if( gl_lockpvs->value > 0.0f )
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if( r_novis->value > 0.0f || r_viewcluster == -1 || !r_worldmodel->vis ) {
		// mark everything
		for( i = 0; i < r_worldmodel->numleafs; i++ )
			r_worldmodel->leafs[ i ].visframe = r_visframecount;
		for( i = 0; i < r_worldmodel->numnodes; i++ )
			r_worldmodel->nodes[ i ].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS( r_viewcluster, r_worldmodel );
	// may have to combine two clusters because of solid water boundaries
	if( r_viewcluster2 != r_viewcluster ) {
		memcpy( fatvis, vis, ( r_worldmodel->numleafs + 7 ) / 8 );
		vis = Mod_ClusterPVS( r_viewcluster2, r_worldmodel );
		c = ( r_worldmodel->numleafs + 31 ) / 32;
		for( i = 0; i < c; i++ )
			( (int *)fatvis )[ i ] |= ( (int *)vis )[ i ];
		vis = fatvis;
	}

	for( i = 0, leaf = r_worldmodel->leafs; i < r_worldmodel->numleafs; i++, leaf++ ) {
		cluster = leaf->cluster;
		if( cluster == -1 )
			continue;
		if( vis[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) {
			node = (mnode_t *)leaf;
			do {
				if( node->visframe == r_visframecount )
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while( node );
		}
	}
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

static void LM_InitBlock() {
	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );
}

static void LM_UploadBlock()
{
	int texture = gl_lms.current_lightmap_texture;

	GL_Bind( gl_state.lightmap_textures + texture );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexImage2D( GL_TEXTURE_2D,
	              0,
	              gl_lms.internal_format,
	              BLOCK_WIDTH, BLOCK_HEIGHT,
	              0,
	              GL_LIGHTMAP_FORMAT,
	              GL_UNSIGNED_BYTE,
	              gl_lms.lightmap_buffer );
	if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
		Com_Error( ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
}

// returns a texture number and the position inside it
static qboolean LM_AllocBlock( int w, int h, int *x, int *y ) {
	int		i, j;
	int		best, best2;

	best = BLOCK_HEIGHT;

	for( i = 0; i < BLOCK_WIDTH - w; i++ ) {
		best2 = 0;

		for( j = 0; j < w; j++ ) {
			if( gl_lms.allocated[ i + j ] >= best )
				break;
			if( gl_lms.allocated[ i + j ] > best2 )
				best2 = gl_lms.allocated[ i + j ];
		}
		if( j == w ) {	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if( best + h > BLOCK_HEIGHT )
		return false;

	for( i = 0; i < w; i++ )
		gl_lms.allocated[ *x + i ] = best + h;

	return true;
}

/*
================
GL_BuildPolygonFromSurface
================
*/
void GL_BuildPolygonFromSurface( msurface_t *fa ) {
	int			i, lindex, lnumverts;
	medge_t *pedges, *r_pedge;
	float *vec;
	float		s, t;
	glpoly_t *poly;
	vec3_t		total;

	// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;

	VectorClear( total );
	//
	// draw texture
	//
	poly = (glpoly_t *)Hunk_Alloc( sizeof( glpoly_t ) + ( lnumverts - 4 ) * VERTEXSIZE * sizeof( float ) );
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for( i = 0; i < lnumverts; i++ ) {
		lindex = currentmodel->surfedges[ fa->firstedge + i ];

		if( lindex > 0 ) {
			r_pedge = &pedges[ lindex ];
			vec = currentmodel->vertexes[ r_pedge->v[ 0 ] ].position;
		} else {
			r_pedge = &pedges[ -lindex ];
			vec = currentmodel->vertexes[ r_pedge->v[ 1 ] ].position;
		}
		s = DotProduct( vec, fa->texinfo->vecs[ 0 ] ) + fa->texinfo->vecs[ 0 ][ 3 ];
		s /= fa->texinfo->image->originalWidth;

		t = DotProduct( vec, fa->texinfo->vecs[ 1 ] ) + fa->texinfo->vecs[ 1 ][ 3 ];
		t /= fa->texinfo->image->originalHeight;

		VectorAdd( total, vec, total );
		VectorCopy( vec, poly->verts[ i ] );
		poly->verts[ i ][ 3 ] = s;
		poly->verts[ i ][ 4 ] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct( vec, fa->texinfo->vecs[ 0 ] ) + fa->texinfo->vecs[ 0 ][ 3 ];
		s -= fa->texturemins[ 0 ];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16; //fa->texinfo->texture->width;

		t = DotProduct( vec, fa->texinfo->vecs[ 1 ] ) + fa->texinfo->vecs[ 1 ][ 3 ];
		t -= fa->texturemins[ 1 ];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16; //fa->texinfo->texture->height;

		poly->verts[ i ][ 5 ] = s;
		poly->verts[ i ][ 6 ] = t;
	}

	poly->numverts = lnumverts;

}

void GL_CreateSurfaceLightmap( msurface_t *surf ) {
	int		smax, tmax;
	byte *base;

	if( surf->flags & ( SURF_DRAWSKY | SURF_DRAWTURB ) )
		return;

	smax = ( surf->extents[ 0 ] >> 4 ) + 1;
	tmax = ( surf->extents[ 1 ] >> 4 ) + 1;

	if( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) ) {
		LM_UploadBlock();
		LM_InitBlock();
		if( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) ) {
			Com_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	base = gl_lms.lightmap_buffer;
	base += ( surf->light_t * BLOCK_WIDTH + surf->light_s ) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMap( surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES );
}

void GL_BeginBuildingLightmaps()
{
	static lightstyle_t	lightstyles[ MAX_LIGHTSTYLES ];
	int				i;
	unsigned		dummy[ 128 * 128 ];

	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );

	r_framecount = 1;		// no dlightcache

	GL_EnableMultitexture( true );
	GL_SelectTexture( GL_TEXTURE1 );

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for( i = 0; i < MAX_LIGHTSTYLES; i++ ) {
		lightstyles[ i ].rgb[ 0 ] = 1;
		lightstyles[ i ].rgb[ 1 ] = 1;
		lightstyles[ i ].rgb[ 2 ] = 1;
		lightstyles[ i ].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;

	if( !gl_state.lightmap_textures ) {
		gl_state.lightmap_textures = TEXNUM_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 1;

	/*
	** if mono lightmaps are enabled and we want to use alpha
	** blending (a,1-a) then we're likely running on a 3DLabs
	** Permedia2.  In a perfect world we'd use a GL_ALPHA lightmap
	** in order to conserve space and maximize bandwidth, however
	** this isn't a perfect world.
	**
	** So we have to use alpha lightmaps, but stored in GL_RGBA format,
	** which means we only get 1/16th the color resolution we should when
	** using alpha lightmaps.  If we find another board that supports
	** only alpha lightmaps but that can at least support the GL_ALPHA
	** format then we should change this code to use real alpha maps.
	*/
	if( toupper( gl_monolightmap->string[ 0 ] ) == 'A' ) {
		gl_lms.internal_format = gl_tex_alpha_format;
	}
	/*
	** try to do hacked colored lighting with a blended texture
	*/
	else if( toupper( gl_monolightmap->string[ 0 ] ) == 'C' ) {
		gl_lms.internal_format = gl_tex_alpha_format;
	} else if( toupper( gl_monolightmap->string[ 0 ] ) == 'I' ) {
		gl_lms.internal_format = GL_INTENSITY8;
	} else if( toupper( gl_monolightmap->string[ 0 ] ) == 'L' ) {
		gl_lms.internal_format = GL_LUMINANCE8;
	} else {
		gl_lms.internal_format = gl_tex_solid_format;
	}

	/*
	** initialize the dynamic lightmap texture
	*/
	GL_Bind( gl_state.lightmap_textures + 0 );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D,
		0,
		gl_lms.internal_format,
		BLOCK_WIDTH, BLOCK_HEIGHT,
		0,
		GL_LIGHTMAP_FORMAT,
		GL_UNSIGNED_BYTE,
		dummy );
}

void GL_EndBuildingLightmaps() {
	LM_UploadBlock();
	GL_EnableMultitexture( false );
}
