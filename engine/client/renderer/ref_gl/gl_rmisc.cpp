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
// r_misc.c

#include "gl_local.h"

/*==============================================
 * FOG
 ===============================================*/

struct FogState
{
	float  density{ 0.0f };
	vec3_t colour{ 0.0f, 0.0f, 0.0f };
	bool   isDirty{ false };
};

static FogState fogState;

void Fog_Setup( const vec3_t colour, float density )
{
	VectorCopy( colour, fogState.colour );
	fogState.density = density;
	fogState.isDirty = true;
}

void Fog_Reset()
{
	Fog_Setup( vec4_origin, 1.0f );
}

void Fog_SetState( bool enable )
{
	if ( enable )
	{
		if ( fogState.isDirty )
		{
			glFogf( GL_FOG_DENSITY, fogState.density );
			//glFogf( GL_FOG_START, 100.0f );
			//glFogf( GL_FOG_END, 1.0f );
			vec4_t colour = {
					fogState.colour[ 0 ],
					fogState.colour[ 1 ],
					fogState.colour[ 2 ],
					1.0f };
			glFogfv( GL_FOG_COLOR, colour );
			glFogi( GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH );

			fogState.isDirty = false;
		}

		glEnable( GL_FOG );
		return;
	}

	glDisable( GL_FOG );
}

/*==============================================
 ===============================================*/

/*
==================
R_InitParticleTexture
==================
*/
byte dottexture[ 8 ][ 8 ] =
		{
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 1, 1, 0, 0, 0, 0 },
				{ 0, 1, 1, 1, 1, 0, 0, 0 },
				{ 0, 1, 1, 1, 1, 0, 0, 0 },
				{ 0, 0, 1, 1, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
};

void R_InitParticleTexture( void )
{
	int     x, y;
	uint8_t data[ 8 ][ 8 ][ 4 ];

	//
	// particle texture
	//
	for ( x = 0; x < 8; x++ )
	{
		for ( y = 0; y < 8; y++ )
		{
			data[ y ][ x ][ 0 ] = 255;
			data[ y ][ x ][ 1 ] = 255;
			data[ y ][ x ][ 2 ] = 255;
			data[ y ][ x ][ 3 ] = dottexture[ x ][ y ] * 255;
		}
	}
	r_particletexture = GL_LoadPic( "***particle***", ( byte * ) data, 8, 8, it_sprite, 32 );

	uint8_t missingData[] = {
			255, 0, 255, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			255, 0, 255, 255 };

	r_notexture = GL_LoadPic( "***r_notexture***", missingData, 2, 2, it_wall, 32 );
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

void GL_ScreenShot_f()
{
	// create the screenshots directory if it doesn't exist
	char checkname[ MAX_OSPATH ];
	Com_sprintf( checkname, sizeof( checkname ), "%s/screenshots", FS_Gamedir() );
	Sys_Mkdir( checkname );

	// Updated the below so that we're no longer limited to 99 screenshots ~hogsy

	std::string outDir = std::string( FS_Gamedir() ) + "/screenshots/";
	if ( r_worldmodel != nullptr )
	{
		const char *p = strrchr( r_worldmodel->name, '/' );
		if ( p == nullptr )
			p = r_worldmodel->name;

		if ( *p == '/' )
			p++;

		char tmp[ MAX_QPATH ];
		COM_StripExtension( p, tmp );
		outDir += tmp;
	}

	std::string outPath;
	for ( int i = 0;; ++i )
	{
		outPath = outDir + "_" + std::to_string( i ) + ".png";
		if ( !FS_LocalFileExists( outPath.c_str() ) )
			break;
	}

	byte *buffer = ( byte * ) Z_Malloc( vid.width * vid.height * 4 );
	glReadPixels( 0, 0, vid.width, vid.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer );

	Image_WritePNG( outPath, buffer, vid.width, vid.height, 4, true );

	Z_Free( buffer );
}

/*
** GL_Strings_f
*/
void GL_Strings_f( void )
{
	Com_DPrintf( "GL_VENDOR: %s\n", gl_config.vendor_string );
	Com_DPrintf( "GL_RENDERER: %s\n", gl_config.renderer_string );
	Com_DPrintf( "GL_VERSION: %s\n", gl_config.version_string );
	Com_DPrintf( "GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glCullFace( GL_FRONT );
	glEnable( GL_TEXTURE_2D );

	glEnable( GL_ALPHA_TEST );
	glAlphaFunc( GL_GREATER, 0.45f );

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );

	glColor4f( 1, 1, 1, 1 );

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glShadeModel( GL_FLAT );

	GL_TextureMode( gl_texturemode->string );
	GL_TextureAlphaMode( gl_texturealphamode->string );
	GL_TextureSolidMode( gl_texturesolidmode->string );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	GL_TexEnv( GL_REPLACE );

	if ( glPointParameterfEXT )
	{
		float attenuations[ 3 ];

		attenuations[ 0 ] = gl_particle_att_a->value;
		attenuations[ 1 ] = gl_particle_att_b->value;
		attenuations[ 2 ] = gl_particle_att_c->value;

		glEnable( GL_POINT_SMOOTH );
		glPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value );
		glPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value );
		glPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}

	GL_UpdateSwapInterval();
}

void GL_UpdateSwapInterval( void )
{
	if ( gl_swapinterval->modified )
	{
		gl_swapinterval->modified = false;

#if 0// TODO: replace this!
#	ifdef _WIN32
		if( qwglSwapIntervalEXT )
			qwglSwapIntervalEXT( (int)gl_swapinterval->value );
#	endif
#endif
	}
}