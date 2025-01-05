/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>

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

#include "../client/ref.h"

#include "model.h"

//===================================================================

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
	bool               scrap;
	bool               has_alpha;

	bool paletted;
} image_t;

#define TEXNUM_LIGHTMAPS 1024
#define TEXNUM_SCRAPS    1152
#define TEXNUM_IMAGES    1153

#define MAX_GLTEXTURES 1024

//===================================================================

extern image_t  *r_notexture;
extern image_t  *r_particletexture;
extern entity_t *currententity;
extern model_t  *currentmodel;
extern int       r_visframecount;
extern int       r_framecount;
extern cplane_t  frustum[ 4 ];
extern int       c_brush_polys, c_alias_polys;

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

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;

extern cvar_t *intensity;

extern cvar_t *gl_modulate;
extern cvar_t *gl_monolightmap;

//====================================================================

extern model_t *r_worldmodel;

extern unsigned d_8to24table[ 256 ];

extern int registration_sequence;
