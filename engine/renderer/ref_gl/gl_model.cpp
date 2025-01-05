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
// models.c -- model loading and caching

#include <cfloat>

#include "gl_local.h"

model_t *loadmodel;
int      modfilelen;

void     Mod_LoadSpriteModel( model_t *mod, void *buffer );
void     Mod_LoadBrushModel( model_t *mod, void *buffer );
void     Mod_LoadMDAModel( model_t *mod, void *buffer );
void     Mod_LoadAliasModel( model_t *mod, void *buffer );
model_t *Mod_LoadModel( model_t *mod, bool crash );

byte mod_novis[ MAX_MAP_LEAFS / 8 ];

#define MAX_MOD_KNOWN 512
model_t mod_known[ MAX_MOD_KNOWN ];
int     mod_numknown;

// the inline * models from the current map are kept seperate
model_t mod_inline[ MAX_MOD_KNOWN ];

int registration_sequence;

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf( vec3_t p, model_t *model )
{
	mnode_t  *node;
	float     d;
	cplane_t *plane;

	if ( !model || !model->nodes )
		Com_Error( ERR_DROP, "Mod_PointInLeaf: bad model" );

	node = model->nodes;
	while ( 1 )
	{
		if ( node->contents != -1 ) return ( mleaf_t * ) node;
		plane = node->plane;
		d     = DotProduct( p, plane->normal ) - plane->dist;
		if ( d > 0 )
			node = node->children[ 0 ];
		else
			node = node->children[ 1 ];
	}

	return NULL;// never reached
}

/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis( byte *in, model_t *model )
{
	static byte decompressed[ MAX_MAP_LEAFS / 8 ];
	int         c;
	byte       *out;
	int         row;

	row = ( model->vis->numclusters + 7 ) >> 3;
	out = decompressed;

	if ( !in )
	{// no vis info, so make all visible
		while ( row )
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do {
		if ( *in )
		{
			*out++ = *in++;
			continue;
		}

		c = in[ 1 ];
		in += 2;
		while ( c )
		{
			*out++ = 0;
			c--;
		}
	} while ( out - decompressed < row );

	return decompressed;
}

/*
==============
Mod_ClusterPVS
==============
*/
byte *Mod_ClusterPVS( int cluster, model_t *model )
{
	if ( cluster == -1 || !model->vis ) return mod_novis;
	return Mod_DecompressVis(
	        ( byte * ) model->vis + model->vis->bitofs[ cluster ][ DVIS_PVS ], model );
}

//===============================================================================

/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f( void )
{
	int      i;
	model_t *mod;
	int      total;

	total = 0;
	Com_Printf( "Loaded models:\n" );
	for ( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
	{
		if ( !mod->name[ 0 ] ) continue;
		Com_Printf( "%8i : %s\n", mod->extradatasize, mod->name );
		total += mod->extradatasize;
	}
	Com_Printf( "Total resident: %i\n", total );
}

/*
===============
Mod_Init
===============
*/
void Mod_Init( void ) { memset( mod_novis, 0xff, sizeof( mod_novis ) ); }

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName( const char *name, bool crash )
{
	model_t  *mod;
	unsigned *buf;
	int       i;

	if ( !name[ 0 ] ) Com_Error( ERR_DROP, "Mod_ForName: NULL name" );

	//
	// inline models are grabbed only from worldmodel
	//
	if ( name[ 0 ] == '*' )
	{
		i = atoi( name + 1 );
		if ( i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels )
			Com_Error( ERR_DROP, "bad inline model number" );
		return &mod_inline[ i ];
	}

	//
	// search the currently loaded models
	//
	for ( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
	{
		if ( !mod->name[ 0 ] ) continue;
		if ( !strcmp( mod->name, name ) ) return mod;
	}

	//
	// find a free model slot spot
	//
	for ( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
	{
		if ( !mod->name[ 0 ] ) break;// free spot
	}
	if ( i == mod_numknown )
	{
		if ( mod_numknown == MAX_MOD_KNOWN )
			Com_Error( ERR_DROP, "mod_numknown == MAX_MOD_KNOWN" );
		mod_numknown++;
	}
	strcpy( mod->name, name );

	//
	// load the file
	//
	modfilelen = FS_LoadFile( mod->name, ( void ** ) &buf );
	if ( !buf )
	{
		if ( crash )
			Com_Error( ERR_DROP, "Mod_NumForName: %s not found", mod->name );
		memset( mod->name, 0, sizeof( mod->name ) );
		return NULL;
	}

	loadmodel = mod;

	//
	// fill it in
	//

	// call the apropriate loader

	switch ( LittleLong( *( unsigned * ) buf ) )
	{
		case IDMDAHEADER:
			Mod_LoadMDAModel( mod, buf );
			break;

		case IDALIASHEADER:
			Mod_LoadAliasModel( mod, buf );
			break;

		case IDSPRITEHEADER:
			loadmodel->extradata = Hunk_Begin( 0x10000 );
			Mod_LoadSpriteModel( mod, buf );
			loadmodel->extradatasize = Hunk_End();
			break;

		case IDBSPHEADER:
			loadmodel->extradata = Hunk_Begin( 0x3D09000 );
			Mod_LoadBrushModel( mod, buf );
			loadmodel->extradatasize = Hunk_End();
			break;

		default:
			Com_Printf( "Mod_NumForName: unknown fileid for %s", mod->name );
			break;
	}

	FS_FreeFile( buf );

	return mod;
}

/*
===============================================================================

										BRUSHMODEL LOADING

===============================================================================
*/

byte *mod_base;

/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting( lump_t *l )
{
	if ( !l->filelen )
	{
		loadmodel->lightdata = NULL;
		return;
	}
	loadmodel->lightdata = static_cast< byte * >( Hunk_Alloc( l->filelen ) );
	memcpy( loadmodel->lightdata, mod_base + l->fileofs, l->filelen );
}

/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility( lump_t *l )
{
	int i;

	if ( !l->filelen )
	{
		loadmodel->vis = NULL;
		return;
	}
	loadmodel->vis = static_cast< dvis_t * >( Hunk_Alloc( l->filelen ) );
	memcpy( loadmodel->vis, mod_base + l->fileofs, l->filelen );

	loadmodel->vis->numclusters = LittleLong( loadmodel->vis->numclusters );
	for ( i = 0; i < loadmodel->vis->numclusters; i++ )
	{
		loadmodel->vis->bitofs[ i ][ 0 ] = LittleLong( loadmodel->vis->bitofs[ i ][ 0 ] );
		loadmodel->vis->bitofs[ i ][ 1 ] = LittleLong( loadmodel->vis->bitofs[ i ][ 1 ] );
	}
}

/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes( lump_t *l )
{
	dvertex_t *in;
	mvertex_t *out;
	int        i, count;

	in = ( dvertex_t * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	out   = static_cast< mvertex_t   *>( Hunk_Alloc( count * sizeof( *out ) ) );

	loadmodel->vertexes    = out;
	loadmodel->numvertexes = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out->position[ 0 ] = LittleFloat( in->point[ 0 ] );
		out->position[ 1 ] = LittleFloat( in->point[ 1 ] );
		out->position[ 2 ] = LittleFloat( in->point[ 2 ] );
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( vec3_t mins, vec3_t maxs )
{
	int    i;
	vec3_t corner;

	for ( i = 0; i < 3; i++ )
	{
		corner[ i ] = fabs( mins[ i ] ) > fabs( maxs[ i ] ) ? fabs( mins[ i ] ) : fabs( maxs[ i ] );
	}

	return VectorLength( corner );
}

/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels( lump_t *l )
{
	dmodel_t *in;
	mmodel_t *out;
	int       i, j, count;

	in = ( dmodel_t * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	out   = static_cast< mmodel_t   *>( Hunk_Alloc( count * sizeof( *out ) ) );

	loadmodel->submodels    = out;
	loadmodel->numsubmodels = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		for ( j = 0; j < 3; j++ )
		{// spread the mins / maxs by a pixel
			out->mins[ j ]   = LittleFloat( in->mins[ j ] ) - 1;
			out->maxs[ j ]   = LittleFloat( in->maxs[ j ] ) + 1;
			out->origin[ j ] = LittleFloat( in->origin[ j ] );
		}
		out->radius    = RadiusFromBounds( out->mins, out->maxs );
		out->headnode  = LittleLong( in->headnode );
		out->firstface = LittleLong( in->firstface );
		out->numfaces  = LittleLong( in->numfaces );
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges( lump_t *l )
{
	dedge_t *in;
	medge_t *out;
	int      i, count;

	in = ( dedge_t * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	out   = static_cast< medge_t   *>( Hunk_Alloc( ( count + 1 ) * sizeof( *out ) ) );

	loadmodel->edges    = out;
	loadmodel->numedges = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out->v[ 0 ] = ( unsigned short ) LittleShort( in->v[ 0 ] );
		out->v[ 1 ] = ( unsigned short ) LittleShort( in->v[ 1 ] );
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo( lump_t *l )
{
	texinfo_t  *in;
	mtexinfo_t *out, *step;
	int         i, j, count;
	char        name[ MAX_QPATH ];
	int         next;

	in = ( texinfo_t * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	out   = static_cast< mtexinfo_t   *>( Hunk_Alloc( count * sizeof( *out ) ) );

	loadmodel->texinfo    = out;
	loadmodel->numtexinfo = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		for ( j = 0; j < 8; j++ ) out->vecs[ 0 ][ j ] = LittleFloat( in->vecs[ 0 ][ j ] );

		out->flags = LittleLong( in->flags );
		next       = LittleLong( in->nexttexinfo );
		if ( next > 0 )
			out->next = loadmodel->texinfo + next;
		else
			out->next = nullptr;

		Com_sprintf( name, sizeof( name ), "textures/%s.tga", in->texture );
		out->image = GL_FindImage( name, it_wall );
		if ( !out->image )
		{
			Com_Printf( "Couldn't load %s\n", name );
			out->image = r_notexture;
		}

		out->flags |= Image_GetSurfaceFlagsForName( in->texture );
	}

	// count animation frames
	for ( i = 0; i < count; i++ )
	{
		out            = &loadmodel->texinfo[ i ];
		out->numframes = 1;
		for ( step = out->next; step && step != out; step = step->next )
			out->numframes++;
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents( msurface_t *s )
{
	float       mins[ 2 ], maxs[ 2 ], val;
	int         i, j, e;
	mvertex_t  *v;
	mtexinfo_t *tex;
	int         bmins[ 2 ], bmaxs[ 2 ];

	mins[ 0 ] = mins[ 1 ] = 999999;
	maxs[ 0 ] = maxs[ 1 ] = -99999;

	tex = s->texinfo;

	for ( i = 0; i < s->numedges; i++ )
	{
		e = loadmodel->surfedges[ s->firstedge + i ];
		if ( e >= 0 )
			v = &loadmodel->vertexes[ loadmodel->edges[ e ].v[ 0 ] ];
		else
			v = &loadmodel->vertexes[ loadmodel->edges[ -e ].v[ 1 ] ];

		for ( j = 0; j < 2; j++ )
		{
			val = v->position[ 0 ] * tex->vecs[ j ][ 0 ] +
			      v->position[ 1 ] * tex->vecs[ j ][ 1 ] +
			      v->position[ 2 ] * tex->vecs[ j ][ 2 ] + tex->vecs[ j ][ 3 ];
			if ( val < mins[ j ] ) mins[ j ] = val;
			if ( val > maxs[ j ] ) maxs[ j ] = val;
		}
	}

	for ( i = 0; i < 2; i++ )
	{
		bmins[ i ] = floor( mins[ i ] / 16 );
		bmaxs[ i ] = ceil( maxs[ i ] / 16 );

		s->texturemins[ i ] = bmins[ i ] * 16;
		s->extents[ i ]     = ( bmaxs[ i ] - bmins[ i ] ) * 16;

		//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 /* 256
		//*/ ) 			VID_Error (ERR_DROP, "Bad surface extents");
	}
}

void GL_BuildPolygonFromSurface( msurface_t *fa );
void GL_CreateSurfaceLightmap( msurface_t *surf );
void GL_EndBuildingLightmaps( void );
void GL_BeginBuildingLightmaps();

/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces( lump_t *l )
{
	dface_t    *in;
	msurface_t *out;
	int         i, count, surfnum;
	int         planenum, side;
	int         ti;

	in = ( dface_t * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	out   = static_cast< msurface_t   *>( Hunk_Alloc( count * sizeof( *out ) ) );

	loadmodel->surfaces    = out;
	loadmodel->numsurfaces = count;

	currentmodel = loadmodel;

	GL_BeginBuildingLightmaps();

	for ( surfnum = 0; surfnum < count; surfnum++, in++, out++ )
	{
		out->firstedge = LittleLong( in->firstedge );
		out->numedges  = LittleShort( in->numedges );
		out->flags     = 0;
		out->polys     = NULL;

		planenum = LittleShort( in->planenum );
		side     = LittleShort( in->side );
		if ( side ) out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		ti = LittleShort( in->texinfo );
		if ( ti < 0 || ti >= loadmodel->numtexinfo )
			Com_Error( ERR_DROP, "MOD_LoadBmodel: bad texinfo number" );
		out->texinfo = loadmodel->texinfo + ti;

		CalcSurfaceExtents( out );

		// lighting info

		for ( i = 0; i < MAXLIGHTMAPS; i++ ) out->styles[ i ] = in->styles[ i ];
		i = LittleLong( in->lightofs );
		if ( i == -1 )
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i;

		// set the drawing flags

		if ( out->texinfo->flags & SURF_WARP )
		{
			out->flags |= SURF_DRAWTURB;
			for ( i = 0; i < 2; i++ )
			{
				out->extents[ i ]     = 16384;
				out->texturemins[ i ] = -8192;
			}
			GL_SubdivideSurface( out );// cut up polygon for warps
		}

		// create lightmaps and polygons
		if ( !( out->texinfo->flags &
		        ( SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP ) ) )
			GL_CreateSurfaceLightmap( out );

		if ( !( out->texinfo->flags & SURF_WARP ) ) GL_BuildPolygonFromSurface( out );
	}

	GL_EndBuildingLightmaps();
}

/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent( mnode_t *node, mnode_t *parent )
{
	node->parent = parent;
	if ( node->contents != -1 ) return;
	Mod_SetParent( node->children[ 0 ], node );
	Mod_SetParent( node->children[ 1 ], node );
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes( lump_t *l )
{
	int      i, j, count, p;
	dnode_t *in;
	mnode_t *out;

	in = ( dnode_t * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	out   = static_cast< mnode_t   *>( Hunk_Alloc( count * sizeof( *out ) ) );

	loadmodel->nodes    = out;
	loadmodel->numnodes = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			out->minmaxs[ j ]     = LittleShort( in->mins[ j ] );
			out->minmaxs[ 3 + j ] = LittleShort( in->maxs[ j ] );
		}

		p          = LittleLong( in->planenum );
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleShort( in->firstface );
		out->numsurfaces  = LittleShort( in->numfaces );
		out->contents     = -1;// differentiate from leafs

		for ( j = 0; j < 2; j++ )
		{
			p = LittleLong( in->children[ j ] );
			if ( p >= 0 )
				out->children[ j ] = loadmodel->nodes + p;
			else
				out->children[ j ] = ( mnode_t * ) ( loadmodel->leafs + ( -1 - p ) );
		}
	}

	Mod_SetParent( loadmodel->nodes, NULL );// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs( lump_t *l )
{
	dleaf_t *in;
	mleaf_t *out;
	int      i, j, count, p;
	//	glpoly_t	*poly;

	in = ( dleaf_t * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	out   = static_cast< mleaf_t   *>( Hunk_Alloc( count * sizeof( *out ) ) );

	loadmodel->leafs    = out;
	loadmodel->numleafs = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			out->minmaxs[ j ]     = LittleShort( in->mins[ j ] );
			out->minmaxs[ 3 + j ] = LittleShort( in->maxs[ j ] );
		}

		p             = LittleLong( in->contents );
		out->contents = p;

		out->cluster = LittleShort( in->cluster );
		out->area    = LittleShort( in->area );

		out->firstmarksurface =
		        loadmodel->marksurfaces + LittleShort( in->firstleafface );
		out->nummarksurfaces = LittleShort( in->numleaffaces );

		// gl underwater warp
#if 0
		if( out->contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_THINWATER ) ) {
			for( j = 0; j < out->nummarksurfaces; j++ ) {
				out->firstmarksurface[ j ]->flags |= SURF_UNDERWATER;
				for( poly = out->firstmarksurface[ j ]->polys; poly; poly = poly->next )
					poly->flags |= SURF_UNDERWATER;
			}
		}
#endif
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces( lump_t *l )
{
	int          i, j, count;
	short       *in;
	msurface_t **out;

	in = ( short * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	out   = static_cast< msurface_t   **>( Hunk_Alloc( count * sizeof( *out ) ) );

	loadmodel->marksurfaces    = out;
	loadmodel->nummarksurfaces = count;

	for ( i = 0; i < count; i++ )
	{
		j = LittleShort( in[ i ] );
		if ( j < 0 || j >= loadmodel->numsurfaces )
			Com_Error( ERR_DROP, "Mod_ParseMarksurfaces: bad surface number" );
		out[ i ] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges( lump_t *l )
{
	int  i, count;
	int *in, *out;

	in = ( int * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	if ( count < 1 || count >= MAX_MAP_SURFEDGES )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: bad surfedges count in %s: %i",
		           loadmodel->name, count );

	out = static_cast< int * >( Hunk_Alloc( count * sizeof( *out ) ) );

	loadmodel->surfedges    = out;
	loadmodel->numsurfedges = count;

	for ( i = 0; i < count; i++ ) out[ i ] = LittleLong( in[ i ] );
}

/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes( lump_t *l )
{
	int       i, j;
	cplane_t *out;
	dplane_t *in;
	int       count;
	int       bits;

	in = ( dplane_t * ) ( mod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",
		           loadmodel->name );
	count = l->filelen / sizeof( *in );
	out   = static_cast< cplane_t   *>( Hunk_Alloc( count * 2 * sizeof( *out ) ) );

	loadmodel->planes    = out;
	loadmodel->numplanes = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		bits = 0;
		for ( j = 0; j < 3; j++ )
		{
			out->normal[ j ] = LittleFloat( in->normal[ j ] );
			if ( out->normal[ j ] < 0 ) bits |= 1 << j;
		}

		out->dist     = LittleFloat( in->dist );
		out->type     = LittleLong( in->type );
		out->signbits = bits;
	}
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel( model_t *mod, void *buffer )
{
	dheader_t *header;
	mmodel_t  *bm;

	loadmodel->type = mod_brush;
	if ( loadmodel != mod_known )
		Com_Error( ERR_DROP, "Loaded a brush model after the world" );

	header = ( dheader_t * ) buffer;

	unsigned int version = LittleLong( header->version );
	if ( version != BSPVERSION )
		Com_Error(
		        ERR_DROP,
		        "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)",
		        mod->name, version

		        ,
		        BSPVERSION );

	// swap all the lumps
	mod_base = ( byte * ) header;

	for ( unsigned int i = 0; i < sizeof( dheader_t ) / 4; i++ )
		( ( int * ) header )[ i ] = LittleLong( ( ( int * ) header )[ i ] );

	// load into heap

	Mod_LoadVertexes( &header->lumps[ LUMP_VERTEXES ] );
	Mod_LoadEdges( &header->lumps[ LUMP_EDGES ] );
	Mod_LoadSurfedges( &header->lumps[ LUMP_SURFEDGES ] );
	Mod_LoadLighting( &header->lumps[ LUMP_LIGHTING ] );
	Mod_LoadPlanes( &header->lumps[ LUMP_PLANES ] );
	Mod_LoadTexinfo( &header->lumps[ LUMP_TEXINFO ] );
	Mod_LoadFaces( &header->lumps[ LUMP_FACES ] );
	Mod_LoadMarksurfaces( &header->lumps[ LUMP_LEAFFACES ] );
	Mod_LoadVisibility( &header->lumps[ LUMP_VISIBILITY ] );
	Mod_LoadLeafs( &header->lumps[ LUMP_LEAFS ] );
	Mod_LoadNodes( &header->lumps[ LUMP_NODES ] );
	Mod_LoadSubmodels( &header->lumps[ LUMP_MODELS ] );
	mod->numframes = 2;// regular and alternate animation

	//
	// set up the submodels
	//
	for ( int i = 0; i < mod->numsubmodels; i++ )
	{
		model_t *starmod;

		bm      = &mod->submodels[ i ];
		starmod = &mod_inline[ i ];

		*starmod = *loadmodel;

		starmod->firstmodelsurface = bm->firstface;
		starmod->nummodelsurfaces  = bm->numfaces;
		starmod->firstnode         = bm->headnode;
		if ( starmod->firstnode >= loadmodel->numnodes )
			Com_Error( ERR_DROP, "Inline model %i has bad firstnode", i );

		VectorCopy( bm->maxs, starmod->maxs );
		VectorCopy( bm->mins, starmod->mins );
		starmod->radius = bm->radius;

		if ( i == 0 ) *loadmodel = *starmod;

		starmod->numleafs = bm->visleafs;
	}
}

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

/**
 * Parse in an MDA file, which outlines how the model
 * should be rendered in the scene along with some
 * additional data we need.
 */
void Mod_LoadMDAModel( model_t *mod, void *buffer )
{
	// + 4 as we're skipping the magic id
	const char *pos = ( const char * ) ( ( byte * ) buffer + 4 );
	while ( *pos != '\0' )
	{
		const char *token = COM_Parse( &pos );
		if ( *token == '#' )
		{
			pos = Script_SkipLine( pos );
			continue;
		}

		if ( strcmp( token, "basemodel" ) == 0 )
		{
			token = COM_Parse( &pos );

			uint8_t *buf;
			FS_LoadFile( token, ( void ** ) &buf );
			if ( !buf )
			{
				Com_Printf( "WARNING: %s not found for MDA %s!\n", token, mod->name );
				return;
			}

			// Ensure magic is valid
			if ( LittleLong( *( int32_t * ) ( buf ) ) != IDALIASHEADER )
			{
				Com_Printf( "WARNING: %s is not a valid basemodel for MDA %s!\n", token, mod->name );
				return;
			}

			Mod_LoadAliasModel( mod, buf );

			FS_FreeFile( buf );

			// Only go so far as fetching the basemodel for now...
			break;
		}
	}
}

void Mod_LoadAliasModel( model_t *mod, void *buffer )
{
	auto *aliasModel = new chr::AliasModel();
	if ( !aliasModel->LoadFromBuffer( buffer ) )
	{
		Com_Printf( "Failed to load model, \"%s\", from buffer!\n", mod->name );
		delete aliasModel;
		return;
	}

	mod->type = mod_alias;

	mod->mins[ 0 ] = -32.0f;
	mod->mins[ 1 ] = -32.0f;
	mod->mins[ 2 ] = -32.0f;
	mod->maxs[ 0 ] = 32.0f;
	mod->maxs[ 1 ] = 32.0f;
	mod->maxs[ 2 ] = 32.0f;

	mod->extradata     = aliasModel;
	mod->extradatasize = sizeof( chr::AliasModel );

	mod->numframes = aliasModel->GetNumFrames();

	// Load in all the skins we need
	const auto &skins = aliasModel->GetSkins();
	for ( size_t i = 0; i < skins.size(); ++i )
	{
		char skinPath[ MAX_QPATH ];
		snprintf( skinPath, sizeof( skinPath ), "%s", mod->name );
		strcpy( strrchr( skinPath, '/' ) + 1, skins[ i ].c_str() );
		mod->skins[ i ] = GL_FindImage( skinPath, it_skin );
	}
}

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel( model_t *mod, void *buffer )
{
	dsprite_t *sprin, *sprout;
	int        i;

	sprin  = ( dsprite_t  *) buffer;
	sprout = static_cast< dsprite_t * >( Hunk_Alloc( modfilelen ) );

	sprout->ident     = LittleLong( sprin->ident );
	sprout->version   = LittleLong( sprin->version );
	sprout->numframes = LittleLong( sprin->numframes );

	if ( sprout->version != SPRITE_VERSION )
		Com_Error( ERR_DROP, "%s has wrong version number (%i should be %i)",
		           mod->name, sprout->version, SPRITE_VERSION );

	if ( sprout->numframes > MAX_MD2SKINS )
		Com_Error( ERR_DROP, "%s has too many frames (%i > %i)", mod->name,
		           sprout->numframes, MAX_MD2SKINS );

	// byte swap everything
	for ( i = 0; i < sprout->numframes; i++ )
	{
		sprout->frames[ i ].width    = LittleLong( sprin->frames[ i ].width );
		sprout->frames[ i ].height   = LittleLong( sprin->frames[ i ].height );
		sprout->frames[ i ].origin_x = LittleLong( sprin->frames[ i ].origin_x );
		sprout->frames[ i ].origin_y = LittleLong( sprin->frames[ i ].origin_y );
		memcpy( sprout->frames[ i ].name, sprin->frames[ i ].name, MAX_SKINNAME );
		mod->skins[ i ] = GL_FindImage( sprout->frames[ i ].name, it_sprite );
	}

	mod->type = mod_sprite;
}

//=============================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void Mod_BeginRegistration( const char *model )
{
	char    fullname[ MAX_QPATH ];
	cvar_t *flushmap;

	registration_sequence++;
	r_oldviewcluster = -1;// force markleafs

	Com_sprintf( fullname, sizeof( fullname ), "maps/%s.bsp", model );

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	flushmap = Cvar_Get( "flushmap", "0", 0 );
	if ( strcmp( mod_known[ 0 ].name, fullname ) || flushmap->value )
		Mod_Free( &mod_known[ 0 ] );
	r_worldmodel = Mod_ForName( fullname, true );

	r_viewcluster = -1;
}

struct model_s *Mod_RegisterModel( const char *name )
{
	model_t *mod = Mod_ForName( name, false );
	if ( mod )
	{
		mod->registration_sequence = registration_sequence;

		// register any images used by the models
		if ( mod->type == mod_sprite )
		{
			dsprite_t *sprout = ( dsprite_t * ) mod->extradata;
			for ( int i = 0; i < sprout->numframes; i++ )
				mod->skins[ i ] = GL_FindImage( sprout->frames[ i ].name, it_sprite );
		}
		else if ( mod->type == mod_brush )
		{
			for ( int i = 0; i < mod->numtexinfo; i++ )
				mod->texinfo[ i ].image->registration_sequence = registration_sequence;
		}
	}
	return mod;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void Mod_EndRegistration( void )
{
	int      i;
	model_t *mod;

	for ( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
	{
		if ( !mod->name[ 0 ] ) continue;
		if ( mod->registration_sequence !=
		     registration_sequence )
		{// don't need this model
			Mod_Free( mod );
		}
	}

	GL_FreeUnusedImages();
}

//=============================================================================

void Mod_Free( model_t *mod )
{
	if ( mod->type == mod_alias )
	{
		auto *aliasModel = static_cast< chr::AliasModel * >( mod->extradata );
		delete aliasModel;
	}
	else
	{
		Hunk_Free( mod->extradata );
	}

	memset( mod, 0, sizeof( *mod ) );
}

/*
================
Mod_FreeAll
================
*/
void Mod_FreeAll( void )
{
	for ( int i = 0; i < mod_numknown; i++ )
	{
		if ( mod_known[ i ].extradatasize <= 0 )
			continue;

		Mod_Free( &mod_known[ i ] );
	}
}

//=============================================================================

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
		memcpy( name, ( char * ) ( ( byte * ) mdl + ofs + i * MAX_QPATH ), MAX_QPATH );
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
	const MD2TaggedSurface *taggedSurface      = ( MD2TaggedSurface      *) ( ( byte      *) mdl + ofs );
	for ( int i = 0; i < numTaggedTriangles; ++i )
		taggedTriangles_.emplace( taggedSurface->name, LittleLong( taggedSurface->triangleIndex ) );
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
		if ( resolution == 1 )
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
		}
		else if ( resolution == 2 )
		{
			const MD2VertexGroup6 *groups = ( MD2VertexGroup6 * ) ( ( byte * ) framePtr + sizeof( MD2FrameHeader ) );
			for ( int j = 0; j < numVertices_; ++j )
			{
				for ( uint k = 0; k < 3; ++k )
					frames_[ i ].vertices[ j ].vertex[ k ] = ( uint16_t ) LittleShort( ( int16_t ) groups[ j ].vertices[ k ] );

				frames_[ i ].vertices[ j ].normalIndex = ( uint16_t ) LittleShort( ( int16_t ) groups[ j ].normalIndex );
			}
		}
		else
		{
			const MD2VertexGroup *groups = ( MD2VertexGroup * ) ( ( byte * ) framePtr + sizeof( MD2FrameHeader ) );
			for ( int j = 0; j < numVertices_; ++j )
			{
				for ( uint k = 0; k < 3; ++k )
					frames_[ i ].vertices[ j ].vertex[ k ] = groups[ j ].vertices[ k ];

				frames_[ i ].vertices[ j ].normalIndex = ( uint16_t ) LittleShort( ( int16_t ) groups[ j ].normalIndex );
			}
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
#include "anorms.h"
};

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float r_avertexnormal_dots[ SHADEDOT_QUANT ][ 256 ] = {
#include "anormtab.h"
};

void chr::AliasModel::LerpVertices( const VertexGroup *v, const VertexGroup *ov, const VertexGroup *verts, Vector3 *lerp, float move[ 3 ], float frontv[ 3 ], float backv[ 3 ] ) const
{
	// PMM -- added RF_SHELL_DOUBLE, RF_SHELL_HALF_DAM
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE |
	                              RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
	{
		for ( int i = 0; i < numVertices_; i++, v++, ov++, lerp++ )
		{
			float *normal = r_avertexnormals[ verts[ i ].normalIndex ];
			lerp->x       = move[ 0 ] + ov->vertex[ 0 ] * backv[ 0 ] + v->vertex[ 0 ] * frontv[ 0 ] + normal[ 0 ] * POWERSUIT_SCALE;
			lerp->y       = move[ 1 ] + ov->vertex[ 1 ] * backv[ 1 ] + v->vertex[ 1 ] * frontv[ 1 ] + normal[ 1 ] * POWERSUIT_SCALE;
			lerp->z       = move[ 2 ] + ov->vertex[ 2 ] * backv[ 2 ] + v->vertex[ 2 ] * frontv[ 2 ] + normal[ 2 ] * POWERSUIT_SCALE;
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
			i = 1.0f;
	}
	else
	{
		R_LightPoint( e->origin, shadeLight_ );

		if ( *gl_monolightmap->string != '0' )
		{
			float s = shadeLight_[ 0 ];
			if ( s < shadeLight_[ 1 ] )
				s = shadeLight_[ 1 ];
			if ( s < shadeLight_[ 2 ] )
				s = shadeLight_[ 2 ];

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
				break;
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
		glDisable( GL_TEXTURE_2D );
	else
	{
		image_t *texture;
		if ( e->model->skins[ 0 ] != nullptr )
			texture = e->model->skins[ 0 ];
		else
			texture = r_notexture;

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
			glBegin( GL_TRIANGLE_STRIP );

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
		glEnable( GL_TEXTURE_2D );

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
