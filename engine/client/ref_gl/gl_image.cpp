/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

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

#include "gl_local.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

image_t		gltextures[ MAX_GLTEXTURES ];
int			numgltextures;
int			base_textureid;		// gltextures[i] = base_textureid+i

static byte			 intensitytable[ 256 ];
static unsigned char gammatable[ 256 ];

cvar_t *intensity;

unsigned	d_8to24table[ 256 ];

static bool GL_Upload8( const byte *data, int width, int height, qboolean mipmap, qboolean is_sky );
static bool GL_Upload32( unsigned *data, int width, int height, qboolean mipmap );


int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

void GL_SetTexturePalette( unsigned palette[ 256 ] ) {
	Q_UNUSED( palette );
	// TODO: obsolete!
}

void GL_EnableMultitexture( bool enable )
{
	static bool state = false;
	if ( state == enable )
	{
		return;
	}

	if ( enable )
	{
		GL_SelectTexture( GL_TEXTURE1 );
		glEnable( GL_TEXTURE_2D );

		if ( gl_lightmap->value >= 1.0f )
		{
			GL_TexEnv( GL_REPLACE );
		}
		else
		{
			static cvar_t *overbrights = nullptr;
			if ( overbrights == nullptr )
			{
				overbrights = Cvar_Get( "r_overbrights", "1", CVAR_ARCHIVE );
			}

			// Setup lightmap channel for overbrights
			if ( overbrights->value > 1.0f )
			{
				GL_TexEnv( GL_COMBINE );
				glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
				glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );
				glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE );
				glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, overbrights->value );
			}
			else
			{
				GL_TexEnv( GL_MODULATE );
			}
		}
	}
	else
	{
		GL_SelectTexture( GL_TEXTURE1 );
		glDisable( GL_TEXTURE_2D );
	}

	GL_SelectTexture( GL_TEXTURE0 );
	GL_TexEnv( GL_REPLACE );

	state = enable;
}

void GL_SelectTexture( GLenum texture ) {
	int tmu;
	if( texture == GL_TEXTURE0 ) {
		tmu = 0;
	} else {
		tmu = 1;
	}

	if( tmu == gl_state.currenttmu ) {
		return;
	}

	gl_state.currenttmu = tmu;

	glActiveTexture( texture );
	glClientActiveTexture( texture );
}

void GL_TexEnv( GLenum mode ) {
	static int lastmodes[ 2 ] = { -1, -1 };
	if( mode != lastmodes[ gl_state.currenttmu ] ) {
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		lastmodes[ gl_state.currenttmu ] = mode;
	}
}

void GL_Bind( int texnum ) {
	extern	image_t *draw_chars;

	if( gl_nobind->value && draw_chars )		// performance evaluation option
		texnum = draw_chars->texnum;
	if( gl_state.currenttextures[ gl_state.currenttmu ] == texnum )
		return;
	gl_state.currenttextures[ gl_state.currenttmu ] = texnum;
	glBindTexture( GL_TEXTURE_2D, texnum );
}

void GL_MBind( GLenum target, int texnum ) {
	GL_SelectTexture( target );
	if( target == GL_TEXTURE0 ) {
		if( gl_state.currenttextures[ 0 ] == texnum )
			return;
	} else {
		if( gl_state.currenttextures[ 1 ] == texnum )
			return;
	}
	GL_Bind( texnum );
}

typedef struct {
	const char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct {
	const char *name;
	int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
	{"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

void GL_TextureMode( char *string )
{
	nox::uint mode;
	for ( mode = 0; mode < NUM_GL_MODES; mode++ )
	{
		if ( !Q_stricmp( modes[ mode ].name, string ) )
			break;
	}

	if ( mode == NUM_GL_MODES )
	{
		Com_Printf( "bad filter name\n" );
		return;
	}

	gl_filter_min = modes[ mode ].minimize;
	gl_filter_max = modes[ mode ].maximize;

	// change all the existing mipmap texture objects
	image_t *glt;
	int      i;
	for ( i = 0, glt = gltextures; i < numgltextures; i++, glt++ )
	{
		if ( glt->type != it_pic && glt->type != it_sky )
		{
			GL_Bind( glt->texnum );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
		}
	}
}

void GL_TextureAlphaMode( char *string )
{
	unsigned int i;
	for ( i = 0; i < NUM_GL_ALPHA_MODES; i++ )
	{
		if ( !Q_stricmp( gl_alpha_modes[ i ].name, string ) )
			break;
	}

	if ( i == NUM_GL_ALPHA_MODES )
	{
		Com_Printf( "bad alpha texture mode name\n" );
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[ i ].mode;
}

void GL_TextureSolidMode( const char *string )
{
	unsigned int i;
	for ( i = 0; i < NUM_GL_SOLID_MODES; i++ )
	{
		if ( !Q_stricmp( gl_solid_modes[ i ].name, string ) )
			break;
	}

	if ( i == NUM_GL_SOLID_MODES )
	{
		Com_Printf( "bad solid texture mode name\n" );
		return;
	}

	gl_tex_solid_format = gl_solid_modes[ i ].mode;
}

void GL_ImageList_f()
{
	int         i;
	image_t    *image;
	int         texels;
	const char *palstrings[ 2 ] =
			{
					"RGB",
					"PAL" };

	Com_Printf( "------------------\n" );
	texels = 0;

	for ( i = 0, image = gltextures; i < numgltextures; i++, image++ )
	{
		if ( image->texnum <= 0 )
			continue;
		texels += image->width * image->height;
		switch ( image->type )
		{
			case it_skin:
				Com_Printf( "M" );
				break;
			case it_sprite:
				Com_Printf( "S" );
				break;
			case it_wall:
				Com_Printf( "W" );
				break;
			case it_pic:
				Com_Printf( "P" );
				break;
			default:
				Com_Printf( " " );
				break;
		}

		Com_Printf( " %3i %3i %s: %s\n",
		            image->width, image->height, palstrings[ image->paletted ], image->name.c_str() );
	}
	Com_Printf( "Total texel count (not counting mipmaps): %i\n", texels );
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

#define	MAX_SCRAPS		1
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int			scrap_allocated[ MAX_SCRAPS ][ BLOCK_WIDTH ];
byte		scrap_texels[ MAX_SCRAPS ][ BLOCK_WIDTH * BLOCK_HEIGHT ];
qboolean	scrap_dirty;

// returns a texture number and the position inside it
int Scrap_AllocBlock( int w, int h, int *x, int *y ) {
	int		i, j;
	int		best, best2;
	int		texnum;

	for( texnum = 0; texnum < MAX_SCRAPS; texnum++ ) {
		best = BLOCK_HEIGHT;

		for( i = 0; i < BLOCK_WIDTH - w; i++ ) {
			best2 = 0;

			for( j = 0; j < w; j++ ) {
				if( scrap_allocated[ texnum ][ i + j ] >= best )
					break;
				if( scrap_allocated[ texnum ][ i + j ] > best2 )
					best2 = scrap_allocated[ texnum ][ i + j ];
			}
			if( j == w ) {	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if( best + h > BLOCK_HEIGHT )
			continue;

		for( i = 0; i < w; i++ )
			scrap_allocated[ texnum ][ *x + i ] = best + h;

		return texnum;
	}

	return -1;
	//	Sys_Error ("Scrap_AllocBlock: full");
}

int	scrap_uploads;

void Scrap_Upload( void ) {
	scrap_uploads++;
	GL_Bind( TEXNUM_SCRAPS );
	GL_Upload8( scrap_texels[ 0 ], BLOCK_WIDTH, BLOCK_HEIGHT, false, false );
	scrap_dirty = false;
}

/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
static void LoadPCX( const char *filename, byte **pic, byte **palette, int *width, int *height ) {
	byte *raw;
	pcx_t *pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte *out, *pix;

	*pic = nullptr;
	*palette = nullptr;

	//
	// load the file
	//
	len = FS_LoadFile( filename, (void **)&raw );
	if( !raw ) {
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

	pcx->xmin = LittleShort( pcx->xmin );
	pcx->ymin = LittleShort( pcx->ymin );
	pcx->xmax = LittleShort( pcx->xmax );
	pcx->ymax = LittleShort( pcx->ymax );
	pcx->hres = LittleShort( pcx->hres );
	pcx->vres = LittleShort( pcx->vres );
	pcx->bytes_per_line = LittleShort( pcx->bytes_per_line );
	pcx->palette_type = LittleShort( pcx->palette_type );

	raw = &pcx->data;

	if( pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480 ) {
		return;
	}

	out = new byte[ ( pcx->ymax + 1 ) * ( pcx->xmax + 1 ) ];

	*pic = out;
	pix = out;

	if( palette ) {
		*palette = new byte[ 768 ];
		memcpy( *palette, (byte *)pcx + len - 768, 768 );
	}

	if( width )
		*width = pcx->xmax + 1;
	if( height )
		*height = pcx->ymax + 1;

	for( y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1 ) {
		for( x = 0; x <= pcx->xmax; ) {
			dataByte = *raw++;

			if( ( dataByte & 0xC0 ) == 0xC0 ) {
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			} else
				runLength = 1;

			while( runLength-- > 0 )
				pix[ x++ ] = dataByte;
		}

	}

	if( raw - (byte *)pcx > len ) {
		Com_DPrintf( "PCX file %s was malformed", filename );
		delete[] *pic;
		*pic = nullptr;
	}

	FS_FreeFile( pcx );
}

static void LoadImage32( const char *name, byte **pic, int *width, int *height )
{
	*pic = nullptr;

	byte *buffer;
	int   length = FS_LoadFile( name, ( void   **) &buffer );
	if ( buffer == nullptr )
	{
		return;
	}

	int   comp; /* this gets ignored for now */
	byte *rgbData = stbi_load_from_memory( buffer, length, width, height, &comp, 4 );
	if ( rgbData == nullptr )
	{
		Com_Error( ERR_DROP, "Failed to read %s!\n%s\n", name, stbi_failure_reason() );
	}

	*pic = rgbData;

	FS_FreeFile( buffer );
}

/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct {
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void R_FloodFillSkin( byte *skin, int skinwidth, int skinheight ) {
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[ FLOODFILL_FIFO_SIZE ];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if( filledcolor == -1 ) {
		filledcolor = 0;
		// attempt to find opaque black
		for( i = 0; i < 256; ++i )
			if( d_8to24table[ i ] == ( 255 << 0 ) ) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if( ( fillcolor == filledcolor ) || ( fillcolor == 255 ) ) {
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[ inpt ].x = 0, fifo[ inpt ].y = 0;
	inpt = ( inpt + 1 ) & FLOODFILL_FIFO_MASK;

	while( outpt != inpt ) {
		int			x = fifo[ outpt ].x, y = fifo[ outpt ].y;
		int			fdc = filledcolor;
		byte *pos = &skin[ x + skinwidth * y ];

		outpt = ( outpt + 1 ) & FLOODFILL_FIFO_MASK;

		if( x > 0 )				FLOODFILL_STEP( -1, -1, 0 );
		if( x < skinwidth - 1 )	FLOODFILL_STEP( 1, 1, 0 );
		if( y > 0 )				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if( y < skinheight - 1 )	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[ x + skinwidth * y ] = fdc;
	}
}

//=======================================================

void GL_ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight ) {
	int		i, j;
	unsigned *inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[ 1024 ], p2[ 1024 ];
	byte *pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for( i = 0; i < outwidth; i++ ) {
		p1[ i ] = 4 * ( frac >> 16 );
		frac += fracstep;
	}
	frac = 3 * ( fracstep >> 2 );
	for( i = 0; i < outwidth; i++ ) {
		p2[ i ] = 4 * ( frac >> 16 );
		frac += fracstep;
	}

	for( i = 0; i < outheight; i++, out += outwidth ) {
		inrow = in + inwidth * (int)( ( i + 0.25 ) * inheight / outheight );
		inrow2 = in + inwidth * (int)( ( i + 0.75 ) * inheight / outheight );
		frac = fracstep >> 1;
		for( j = 0; j < outwidth; j++ ) {
			pix1 = (byte *)inrow + p1[ j ];
			pix2 = (byte *)inrow + p2[ j ];
			pix3 = (byte *)inrow2 + p1[ j ];
			pix4 = (byte *)inrow2 + p2[ j ];
			( (byte *)( out + j ) )[ 0 ] = ( pix1[ 0 ] + pix2[ 0 ] + pix3[ 0 ] + pix4[ 0 ] ) >> 2;
			( (byte *)( out + j ) )[ 1 ] = ( pix1[ 1 ] + pix2[ 1 ] + pix3[ 1 ] + pix4[ 1 ] ) >> 2;
			( (byte *)( out + j ) )[ 2 ] = ( pix1[ 2 ] + pix2[ 2 ] + pix3[ 2 ] + pix4[ 2 ] ) >> 2;
			( (byte *)( out + j ) )[ 3 ] = ( pix1[ 3 ] + pix2[ 3 ] + pix3[ 3 ] + pix4[ 3 ] ) >> 2;
		}
	}
}

/*
** Operates in place, quartering the size of the texture
*/
void GL_MipMap( byte *in, int width, int height ) {
	int		i, j;
	byte *out;

	width <<= 2;
	height >>= 1;
	out = in;
	for( i = 0; i < height; i++, in += width ) {
		for( j = 0; j < width; j += 8, out += 4, in += 8 ) {
			out[ 0 ] = ( in[ 0 ] + in[ 4 ] + in[ width + 0 ] + in[ width + 4 ] ) >> 2;
			out[ 1 ] = ( in[ 1 ] + in[ 5 ] + in[ width + 1 ] + in[ width + 5 ] ) >> 2;
			out[ 2 ] = ( in[ 2 ] + in[ 6 ] + in[ width + 2 ] + in[ width + 6 ] ) >> 2;
			out[ 3 ] = ( in[ 3 ] + in[ 7 ] + in[ width + 3 ] + in[ width + 7 ] ) >> 2;
		}
	}
}

static bool GL_Upload32( unsigned *data, int width, int height, qboolean mipmap )
{
	int   samples;
	int   i, c;
	byte *scan;
	int   comp;

	// scan the texture for any non-255 alpha
	c = width * height;
	scan = ( ( byte * ) data ) + 3;
	samples = gl_solid_format;
	for ( i = 0; i < c; i++, scan += 4 )
	{
		if ( *scan != 255 )
		{
			samples = gl_alpha_format;
			break;
		}
	}

	if ( samples == gl_solid_format )
		comp = gl_tex_solid_format;
	else if ( samples == gl_alpha_format )
		comp = gl_tex_alpha_format;
	else
	{
		Com_Printf(
				"Unknown number of texture components %i\n",
				samples );
		comp = samples;
	}

	glTexImage2D( GL_TEXTURE_2D, 0, comp, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

	if ( mipmap )
	{
		glGenerateMipmap( GL_TEXTURE_2D );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
	}

	return ( samples == gl_alpha_format );
}

static bool GL_Upload8( const byte *data, int width, int height, qboolean mipmap, qboolean is_sky )
{
	unsigned trans[ 640 * 256 ];
	int      i, s;
	int      p;

	s = width * height;

	if ( s > sizeof( trans ) / 4 )
		Com_Error( ERR_DROP, "GL_Upload8: too large" );

	for ( i = 0; i < s; i++ )
	{
		p = data[ i ];
		trans[ i ] = d_8to24table[ p ];

		if ( p == 255 )
		{// transparent, so scan around for another color
			// to avoid alpha fringes
			// FIXME: do a full flood fill so mips work...
			if ( i > width && data[ i - width ] != 255 )
				p = data[ i - width ];
			else if ( i < s - width && data[ i + width ] != 255 )
				p = data[ i + width ];
			else if ( i > 0 && data[ i - 1 ] != 255 )
				p = data[ i - 1 ];
			else if ( i < s - 1 && data[ i + 1 ] != 255 )
				p = data[ i + 1 ];
			else
				p = 0;
			// copy rgb components
			( ( byte * ) &trans[ i ] )[ 0 ] = ( ( byte * ) &d_8to24table[ p ] )[ 0 ];
			( ( byte * ) &trans[ i ] )[ 1 ] = ( ( byte * ) &d_8to24table[ p ] )[ 1 ];
			( ( byte * ) &trans[ i ] )[ 2 ] = ( ( byte * ) &d_8to24table[ p ] )[ 2 ];
		}
	}

	return GL_Upload32( trans, width, height, mipmap );
}

/*
** This is also used as an entry point for the generated r_notexture
*/
image_t *GL_LoadPic( const std::string &name, byte *pic, int width, int height, imagetype_t type, int bits )
{
	image_t *image;
	int      i;

	// find a free image_t
	for ( i = 0, image = gltextures; i < numgltextures; i++, image++ )
	{
		if ( !image->texnum )
			break;
	}
	if ( i == numgltextures )
	{
		if ( numgltextures == MAX_GLTEXTURES )
			Com_Error( ERR_DROP, "MAX_GLTEXTURES" );
		numgltextures++;
	}
	image = &gltextures[ i ];

	image->name = name;
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	if ( type == it_skin && bits == 8 )
		R_FloodFillSkin( pic, width, height );

	// load little pics into the scrap
	if ( image->type == it_pic && bits == 8 && image->width < 64 && image->height < 64 )
	{
		int x, y;
		int j, k;
		int texnum;

		texnum = Scrap_AllocBlock( image->width, image->height, &x, &y );
		if ( texnum == -1 )
			goto nonscrap;
		scrap_dirty = true;

		// copy the texels into the scrap block
		k = 0;
		for ( i = 0; i < image->height; i++ )
			for ( j = 0; j < image->width; j++, k++ )
				scrap_texels[ texnum ][ ( y + i ) * BLOCK_WIDTH + x + j ] = pic[ k ];
		image->texnum = TEXNUM_SCRAPS + texnum;
		image->scrap = true;
		image->has_alpha = true;
		image->sl = ( x + 0.01f ) / ( float ) BLOCK_WIDTH;
		image->sh = ( x + image->width - 0.01f ) / ( float ) BLOCK_WIDTH;
		image->tl = ( y + 0.01f ) / ( float ) BLOCK_WIDTH;
		image->th = ( y + image->height - 0.01f ) / ( float ) BLOCK_WIDTH;
	}
	else
	{
	nonscrap:
		image->scrap = false;
		image->texnum = TEXNUM_IMAGES + ( image - gltextures );
		GL_Bind( image->texnum );
		if ( bits == 8 )
			image->has_alpha = GL_Upload8( pic, width, height, ( image->type != it_pic && image->type != it_sky ), image->type == it_sky );
		else
			image->has_alpha = GL_Upload32( ( unsigned * ) pic, width, height, ( image->type != it_pic && image->type != it_sky ) );
		image->paletted = false;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
	}

	return image;
}

/*
** Attempt to load an image in without an extension.
** Returns actual path after successful load, otherwise returns an empty string.
*/
std::string Image_LoadAbstract( const std::string &name, byte **pic, byte **palette, int *width, int *height, int *depth )
{
	struct ImageLoader
	{
		const char *extension;
		int         depth;
		void ( *Load8bpp )( const char *filename, byte **pic, byte **palette, int *width, int *height );
		void ( *Load32bpp )( const char *filename, byte **pic, int *width, int *height );
	};

	static const ImageLoader loaders[] = {
			{ "tga", 32, nullptr, LoadImage32 },
			{ "png", 32, nullptr, LoadImage32 },
			{ "bmp", 32, nullptr, LoadImage32 },
			{ "pcx", 8, LoadPCX, nullptr },
	};

	for ( auto &loader : loaders )
	{
		std::string nName = name + "." + loader.extension;

		*pic = nullptr;
		*palette = nullptr;
		if ( loader.depth == 8 )
		{
			loader.Load8bpp( nName.c_str(), pic, palette, width, height );
		}
		else
		{
			loader.Load32bpp( nName.c_str(), pic, width, height );
		}

		if ( *pic == nullptr )
		{
			continue;
		}

		*depth = loader.depth;
		return nName;
	}

	return "";
}

/*
** Finds or loads the given image
*/
image_t *GL_FindImage( const std::string &name, imagetype_t type )
{
	// look for it
	int      i;
	image_t *image;
	for ( i = 0, image = gltextures; i < numgltextures; i++, image++ )
	{
		if ( name != image->name )
		{
			continue;
		}

		image->registration_sequence = registration_sequence;
		return image;
	}

	image = nullptr;

	//
	// load the pic from disk
	//

	byte *pic, *palette;
	int width, height, depth;

	// Right, this is kinda gross but Anachronox seems to strip the extension
	// from the filename and then load whatever standard formats it supports
	// todo: rather than stripping here, try with the extension first, if that doesn't work, try everything else?

	std::string loadName;
	size_t l = name.rfind( '.', name.length() );
	if ( l != std::string::npos )
	{
		loadName = name.substr( 0, l );
	}
	else
	{
		loadName = name;
	}

	loadName = Image_LoadAbstract( loadName, &pic, &palette, &width, &height, &depth );

	if ( pic != nullptr )
	{
		image = GL_LoadPic( loadName, pic, width, height, type, depth );
		// HACK: store the original name for comparing later!
		image->name = name;
	}

	if ( image == nullptr )
	{
		Com_Printf( "WARNING: Failed to find \"%s\"!\n", name.c_str() );
		return nullptr;
	}

	delete[] pic;
	delete[] palette;

	return image;
}

struct image_s *R_RegisterSkin( const char *name ) {
	return GL_FindImage( name, it_skin );
}

/*
** Any image that was not touched on this registration sequence
** will be freed.
*/
void GL_FreeUnusedImages() {
	int		i;
	image_t *image;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for( i = 0, image = gltextures; i < numgltextures; i++, image++ ) {
		if( image->registration_sequence == registration_sequence )
			continue;		// used this sequence
		if( !image->registration_sequence )
			continue;		// free image_t slot
		if( image->type == it_pic )
			continue;		// don't free pics
		// free it
		glDeleteTextures( 1, (GLuint *)&image->texnum );
		memset( image, 0, sizeof( *image ) );
	}
}

int Draw_GetPalette() {
	int		i;
	int		r, g, b;
	unsigned	v;
	byte *pic, *pal;
	int		width, height;

	// get the palette

	LoadPCX( "graphics/colormap.pcx", &pic, &pal, &width, &height );
	if( !pal )
		Com_Error( ERR_FATAL, "Couldn't load graphics/colormap.pcx" );

	for( i = 0; i < 256; i++ ) {
		r = pal[ i * 3 + 0 ];
		g = pal[ i * 3 + 1 ];
		b = pal[ i * 3 + 2 ];

		v = ( 255 << 24 ) + ( r << 0 ) + ( g << 8 ) + ( b << 16 );
		d_8to24table[ i ] = LittleLong( v );
	}

	d_8to24table[ 255 ] &= LittleLong( 0xffffff );	// 255 is transparent

	free( pic );
	free( pal );

	return 0;
}

static void Image_LoadTextureInfo();

/*
===============
GL_InitImages
===============
*/
void	GL_InitImages() {
	int		i, j;
	float	g = vid_gamma->value;

	registration_sequence = 1;

	// init intensity conversions
	intensity = Cvar_Get( "intensity", "2", 0 );

	if( intensity->value <= 1 )
		Cvar_Set( "intensity", "1" );

	gl_state.inverse_intensity = 1 / intensity->value;

	Draw_GetPalette();

	for( i = 0; i < 256; i++ ) {
		if( g == 1 ) {
			gammatable[ i ] = i;
		} else {
			float inf;

			inf = 255 * pow( ( i + 0.5 ) / 255.5, g ) + 0.5;
			if( inf < 0 )
				inf = 0;
			if( inf > 255 )
				inf = 255;
			gammatable[ i ] = inf;
		}
	}

	for( i = 0; i < 256; i++ ) {
		j = i * intensity->value;
		if( j > 255 )
			j = 255;
		intensitytable[ i ] = j;
	}

	Image_LoadTextureInfo(); // Load and cache the textureinfo.dat
}

/*
===============
GL_ShutdownImages
===============
*/
void	GL_ShutdownImages( void ) {
	int		i;
	image_t *image;

	for( i = 0, image = gltextures; i < numgltextures; i++, image++ ) {
		if( !image->registration_sequence )
			continue;		// free image_t slot
		// free it
		glDeleteTextures( 1, (GLuint *)&image->texnum );
		memset( image, 0, sizeof( *image ) );
	}
}

/*==============================================
 * Texture Surface Flags
 * ---------------------------------------------
 * These are a collection of flags per texture
 * that determine how each texture should be handled,
 * either in-terms of rendering or just general game
 * behaviour.
 *
 * These are all indexed via a textureinfo.dat file
 * provided by the game.
 ===============================================*/

static std::map< std::string, int > textureSurfaceFlags;

static void Image_LoadTextureInfo()
{
	char *script;
	// Allocate and load it into a buffer to ensure null-termination
	{
		char *buf;
		int length = FS_LoadFile( "textures/textureinfo.dat", ( void ** ) &buf );
		if ( length == -1 )
		{
			Com_Error( ERR_FATAL, "Invalid or missing textureinfo.dat file!\n" );
			return;
		}

		script = new char[ length + 1 ];
		memcpy( script, buf, length );
		script[ length ] = '\0';

		FS_FreeFile( buf );
	}

	int surfaceFlag = 0;

	const char *p = script;
	while ( p != nullptr && *p != '\0' )
	{
		const char *token = COM_Parse( &p );
		if ( *token == '#' )
		{
			token++;
			if ( Q_strcasecmp( "carpet", token ) == 0 )
				surfaceFlag = SURF_SND_CARPET;
			else if ( Q_strcasecmp( "grass", token ) == 0 )
				surfaceFlag = SURF_SND_GRASS;
			else if ( Q_strcasecmp( "gravel", token ) == 0 )
				surfaceFlag = SURF_SND_GRAVEL;
			else if ( Q_strcasecmp( "hollow", token ) == 0 )
				surfaceFlag = SURF_SND_HOLLOW;
			else if ( Q_strcasecmp( "ice", token ) == 0 )
				surfaceFlag = SURF_SND_ICE;
			else if ( Q_strcasecmp( "leaves", token ) == 0 )
				surfaceFlag = SURF_SND_LEAVES;
			else if ( Q_strcasecmp( "metal", token ) == 0 )
				surfaceFlag = SURF_SND_METAL;
			else if ( Q_strcasecmp( "puddle", token ) == 0 )
				surfaceFlag = SURF_SND_PUDDLE;
			else if ( Q_strcasecmp( "sand", token ) == 0 )
				surfaceFlag = SURF_SND_SAND;
			else if ( Q_strcasecmp( "snow", token ) == 0 )
				surfaceFlag = SURF_SND_SNOW;
			else if ( Q_strcasecmp( "stone", token ) == 0 )
				surfaceFlag = SURF_SND_STONE;
			else if ( Q_strcasecmp( "water", token ) == 0 )
				surfaceFlag = SURF_SND_WATER;
			else if ( Q_strcasecmp( "wood", token ) == 0 )
				surfaceFlag = SURF_SND_WOOD;
			else if ( Q_strcasecmp( "alphatest", token ) == 0 )
				surfaceFlag = SURF_ALPHA_TEST;
			else if ( Q_strcasecmp( "alphabanner", token ) == 0 )
				surfaceFlag = SURF_ALPHA_BANNER;
			else
			{
				Com_Printf( "WARNING: Unknown surface type \"%s\"!\n", token );
				surfaceFlag = 0;
			}
			continue;
		}

		if ( surfaceFlag == 0 )
		{
			Com_Printf( "WARNING: No valid surface type, ignoring \"%s\"!\n", token );
			continue;
		}

		// Copy the string into a temporary buffer without the extension (if there is one)
		char tmp[ MAX_QPATH ];
		for ( nox::uint i = 0; i < MAX_QPATH - 1; ++i )
		{
			if ( *token == '\0' || *token == '.' )
			{
				tmp[ i ] = '\0';
				break;
			}

			tmp[ i ] = ( char ) tolower( *token++ );
		}

		auto i = textureSurfaceFlags.find( tmp );
		if ( i == textureSurfaceFlags.end() )
		{
			textureSurfaceFlags.emplace( tmp, surfaceFlag );
			continue;
		}

		i->second |= surfaceFlag;
	}

	delete[] script;
}

/**
 * Returns the surface flags associated with the given texture.
 */
int Image_GetSurfaceFlagsForName( const std::string &path )
{
	auto i = textureSurfaceFlags.find( nox::StringToLower( path ) );
	if ( i == textureSurfaceFlags.end() )
	{
		return 0;
	}

	return i->second;
}
