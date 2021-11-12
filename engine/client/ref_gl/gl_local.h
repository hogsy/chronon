/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2020-2021 Mark E Sowden <markelswo@gmail.com>

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

#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdio>
#include <cmath>

#include "../engine/client/ref.h"
#include "../engine/client/vid.h"

extern viddef_t vid;

#include "qgl.h"

#define PITCH 0// up / down
#define YAW   1// left / right
#define ROLL  2// fall over

/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
} imagetype_t;

typedef struct image_s
{
	std::string        name;// game path, including extension
	imagetype_t        type;
	int                width, height;// source image
	int                originalWidth, originalHeight;
	int                registration_sequence;// 0 = free
	struct msurface_s *texturechain;         // for sort-by-texture world drawing
	int                texnum;               // gl texture binding
	float              sl, tl, sh, th;       // 0,0 - 1,1 unless part of the scrap
	qboolean           scrap;
	qboolean           has_alpha;

	qboolean paletted;
} image_t;

#define TEXNUM_LIGHTMAPS 1024
#define TEXNUM_SCRAPS    1152
#define TEXNUM_IMAGES    1153

#define MAX_GLTEXTURES 1024

//===================================================================

typedef enum
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "gl_model.h"

void GL_BeginRendering( int *x, int *y, int *width, int *height );
void GL_EndRendering( void );

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );

extern double gldepthmin, gldepthmax;

typedef struct
{
	float x, y, z;
	float s, t;
	float r, g, b;
} glvert_t;

#define BACKFACE_EPSILON 0.01

//====================================================

extern image_t gltextures[ MAX_GLTEXTURES ];
extern int     numgltextures;

extern image_t * r_notexture;
extern image_t * r_particletexture;
extern entity_t *currententity;
extern model_t * currentmodel;
extern int       r_visframecount;
extern int       r_framecount;
extern cplane_t  frustum[ 4 ];
extern int       c_brush_polys, c_alias_polys;

extern int gl_filter_min, gl_filter_max;

//
// view origin
//
extern vec3_t vup;
extern vec3_t vpn;
extern vec3_t vright;
extern vec3_t r_origin;

//
// screen size info
//
extern refdef_t r_newrefdef;
extern int      r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern cvar_t *r_norefresh;
extern cvar_t *r_lefthand;
extern cvar_t *r_drawentities;
extern cvar_t *r_drawworld;
extern cvar_t *r_speeds;
extern cvar_t *r_fullbright;
extern cvar_t *r_novis;
extern cvar_t *r_nocull;
extern cvar_t *r_lerpmodels;

extern cvar_t *
        r_lightlevel;// FIXME: This is a HACK to get the client's light level

extern cvar_t *gl_vertex_arrays;

extern cvar_t *gl_ext_swapinterval;
extern cvar_t *gl_ext_palettedtexture;
extern cvar_t *gl_ext_multitexture;
extern cvar_t *gl_ext_pointparameters;
extern cvar_t *gl_ext_compiled_vertex_array;

extern cvar_t *gl_particle_min_size;
extern cvar_t *gl_particle_max_size;
extern cvar_t *gl_particle_size;
extern cvar_t *gl_particle_att_a;
extern cvar_t *gl_particle_att_b;
extern cvar_t *gl_particle_att_c;

extern cvar_t *gl_nosubimage;
extern cvar_t *gl_bitdepth;
extern cvar_t *gl_mode;
extern cvar_t *gl_log;
extern cvar_t *gl_lightmap;
extern cvar_t *gl_shadows;
extern cvar_t *gl_dynamic;
extern cvar_t *gl_monolightmap;
extern cvar_t *gl_nobind;
extern cvar_t *gl_round_down;
extern cvar_t *gl_picmip;
extern cvar_t *gl_skymip;
extern cvar_t *gl_showtris;
extern cvar_t *gl_finish;
extern cvar_t *gl_ztrick;
extern cvar_t *gl_clear;
extern cvar_t *gl_cull;
extern cvar_t *gl_poly;
extern cvar_t *gl_texsort;
extern cvar_t *gl_polyblend;
extern cvar_t *gl_lightmaptype;
extern cvar_t *gl_modulate;
extern cvar_t *gl_playermip;
extern cvar_t *gl_drawbuffer;
extern cvar_t *gl_3dlabs_broken;
extern cvar_t *gl_swapinterval;
extern cvar_t *gl_texturemode;
extern cvar_t *gl_texturealphamode;
extern cvar_t *gl_texturesolidmode;
extern cvar_t *gl_saturatelighting;
extern cvar_t *gl_lockpvs;

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;

extern cvar_t *intensity;

extern int gl_lightmap_format;
extern int gl_solid_format;
extern int gl_alpha_format;
extern int gl_tex_solid_format;
extern int gl_tex_alpha_format;

extern int c_visible_lightmaps;
extern int c_visible_textures;

extern float r_world_matrix[ 16 ];

void R_TranslatePlayerSkin( int playernum );
void GL_Bind( int texnum );
void GL_MBind( GLenum target, int texnum );
void GL_TexEnv( GLenum value );
void GL_EnableMultitexture( bool enable );
void GL_SelectTexture( GLenum );

void R_LightPoint( const vec3_t p, vec3_t color );
void R_PushDlights( void );

//====================================================================

extern model_t *r_worldmodel;

extern unsigned d_8to24table[ 256 ];

extern int registration_sequence;

void V_AddBlend( float r, float g, float b, float a, float *v_blend );

void R_RenderFrame( refdef_t *fd );

int  R_Init();
void R_Shutdown( void );

void     R_RenderView( refdef_t *fd );
void     GL_ScreenShot_f( void );
void     R_DrawAliasModel( entity_t *e );
void     R_DrawBrushModel( entity_t *e );
void     R_DrawSpriteModel( entity_t *e );
void     R_DrawBeam( entity_t *e );
void     R_DrawWorld( void );
void     R_DrawAlphaSurfaces( void );
void     R_RenderBrushPoly( msurface_t *fa );
void     R_InitParticleTexture( void );
void     Draw_InitLocal( void );
void     GL_SubdivideSurface( msurface_t *fa );
qboolean R_CullBox( vec3_t mins, vec3_t maxs );
void     R_RotateForEntity( entity_t *e );
void     R_MarkLeaves( void );

void R_SetSky( const char *name, float rotate, vec3_t axis );

glpoly_t *WaterWarpPolyVerts( glpoly_t *p );
void      EmitWaterPolys( msurface_t *fa );
void      R_AddSkySurface( msurface_t *fa );
void      R_ClearSkyBox( void );
void      R_DrawSkyBox( void );
void      R_MarkLights( dlight_t *light, int bit, mnode_t *node );

void COM_StripExtension( char *in, char *out );

struct image_s *Draw_FindPic( const char *name );

void Draw_GetPicSize( int *w, int *h, const char *name );
void Draw_Pic( int x, int y, const char *name );
void Draw_StretchPic( int x, int y, int w, int h, const char *name );
void Draw_Char( int x, int y, int c );
void Draw_TileClear( int x, int y, int w, int h, const char *name );
void Draw_Fill( int x, int y, int w, int h, int c );
void Draw_FadeScreen( void );
void Draw_StretchRaw( int x, int y, int w, int h, int cols, int rows,
                      byte *data );

void R_BeginFrame();
void R_SwapBuffers( int );
void R_SetPalette( const unsigned char *palette );

int Draw_GetPalette( void );

void GL_ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out,
                         int outwidth, int outheight );

struct image_s *R_RegisterSkin( const char *name );

image_t *GL_LoadPic( const std::string &name, byte *pic, int width, int height,
                     imagetype_t type, int bits );
image_t *GL_FindImage( const std::string &name, imagetype_t type );
void     GL_TextureMode( char *string );
void     GL_ImageList_f( void );
int Image_GetSurfaceFlagsForName( const std::string &path );

void GL_SetTexturePalette( unsigned palette[ 256 ] );

void GL_InitImages( void );
void GL_ShutdownImages( void );

void GL_FreeUnusedImages( void );

void GL_TextureAlphaMode( char *string );
void GL_TextureSolidMode( const char *string );

void Fog_Setup( const vec3_t colour, float density );
void Fog_Reset();
void Fog_SetState( bool enable );

/*
** GL extension emulation functions
*/
void GL_DrawParticles( int n, const particle_t particles[],
                       const unsigned colortable[ 768 ] );

/*
** GL config stuff
*/
#define GL_RENDERER_RENDITION  0x001C0000
#define GL_RENDERER_MCD        0x01000000

typedef struct
{
	int         renderer;
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;

	qboolean allow_cds;
} glconfig_t;

typedef struct
{
	float    inverse_intensity;
	qboolean fullscreen;

	int prev_mode;

	int lightmap_textures;

	int currenttextures[ 2 ];
	int currenttmu;

	unsigned char originalRedGammaTable[ 256 ];
	unsigned char originalGreenGammaTable[ 256 ];
	unsigned char originalBlueGammaTable[ 256 ];
} glstate_t;

extern glconfig_t gl_config;
extern glstate_t  gl_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void     GLimp_EndFrame( void );
void     GLimp_Shutdown( void );
rserr_t  GLimp_SetMode( unsigned int *pwidth, unsigned int *pheight, int mode, qboolean fullscreen );
void     GLimp_AppActivate( qboolean active );
void     GLimp_EnableLogging( qboolean enable );
void     GLimp_LogNewFrame( void );
