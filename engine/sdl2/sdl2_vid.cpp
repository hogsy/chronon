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

#include "../client/client.h"
#include "../client/ref_gl/gl_local.h"

#include <SDL2/SDL.h>

// Disgusting globals...
viddef_t viddef;
cvar_t *win_noalttab;
cvar_t *vid_xpos;// X coordinate of window position
cvar_t *vid_ypos;// Y coordinate of window position

static SDL_Window *sdlWindow = nullptr;
static SDL_GLContext glContext = nullptr;

bool VID_GetModeInfo( int *width, int *height, int mode )
{
	struct VideoMode
	{
		int width, height;
	};
	static std::vector< VideoMode > videoModes;

	// Determine what display modes are available
	static bool listInit = false;
	if ( !listInit )
	{
		Com_Printf( "Attempting to enumerate available display modes...\n" );

		int numSupportedModes = SDL_GetNumDisplayModes( 0 );
		if ( numSupportedModes <= 0 )
		{
			Com_Printf( "Failed to get display modes: %s\n", SDL_GetError() );
			return false;
		}

		int ow = 0, oh = 0;
		for ( int i = 0; i < numSupportedModes; ++i )
		{
			SDL_DisplayMode displayMode;
			if ( SDL_GetDisplayMode( 0, i, &displayMode ) != 0 )
			{
				Com_Printf( "Failed to get display mode %d: %s\n", SDL_GetError() );
				continue;
			}

			/* We only care about the resolution, and GetDisplayMode is going to
			 * give us results by their refresh rate too, so... */
			if ( displayMode.w == ow && displayMode.h == oh )
			{
				continue;
			}

			videoModes.push_back( { displayMode.w, displayMode.h } );

			Com_Printf( "Mode %d: %dx%d\n", i, displayMode.w, displayMode.h );

			ow = displayMode.w;
			oh = displayMode.h;
		}

		listInit = true;
	}

	if ( mode >= ( int ) videoModes.size() )
	{
		return false;
	}

	*width = videoModes[ mode ].width;
	*height = videoModes[ mode ].height;

	return true;
}

void VID_NewWindow( int width, int height )
{
	viddef.width = width;
	viddef.height = height;
}

/**
 * This function gets called once just before drawing each frame, and it's sole
 * purpose in life is to check to see if any of the video mode parameters have
 * changed, and if they have to update the rendering DLL and/or video mode to
 * match.
 */
void VID_CheckChanges()
{
}

void VID_Init()
{
	vid_xpos = Cvar_Get( "vid_xpos", "3", CVAR_ARCHIVE );
	vid_ypos = Cvar_Get( "vid_ypos", "22", CVAR_ARCHIVE );
	win_noalttab = Cvar_Get( "win_noalttab", "0", CVAR_ARCHIVE );
}

void VID_Shutdown()
{
	R_Shutdown();
}

/*
========================================================================
 GLimp_* Implementation
========================================================================
*/

rserr_t GLimp_SetMode( unsigned int *pwidth, unsigned int *pheight, int mode, bool fullscreen )
{
	Com_Printf( "Initializing OpenGL display\n" );

	int width, height;
	if ( !VID_GetModeInfo( &width, &height, mode ) )
	{
		Com_Printf( " invalid mode\n" );
		return rserr_invalid_mode;
	}

	static const char *win_fs[] = { "W", "FS" };
	Com_Printf( " %d %d %s\n", width, height, win_fs[ fullscreen ] );

	int winFlags = SDL_WINDOW_OPENGL;
	if ( fullscreen )
	{
		/* todo: does sdl handle internal buffer resize for us, or do
		 *  we need to do that ourselves? */
		winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	if ( sdlWindow == nullptr )
	{
		sdlWindow = SDL_CreateWindow( ENGINE_NAME,
		                              SDL_WINDOWPOS_CENTERED,
		                              SDL_WINDOWPOS_CENTERED,
		                              width, height,
		                              winFlags );
		if ( sdlWindow == nullptr )
		{
			Com_Error( ERR_FATAL, "Failed to create SDL window: %s\n", SDL_GetError() );
		}

		glContext = SDL_GL_CreateContext( sdlWindow );
		if ( glContext == nullptr )
		{
			Com_Error( ERR_FATAL, "Failed to create GL context: %s\n", SDL_GetError() );
		}

		SDL_GL_MakeCurrent( sdlWindow, glContext );

		return rserr_ok;
	}

	SDL_SetWindowSize( sdlWindow, width, height );
	SDL_SetWindowFullscreen( sdlWindow, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0 );
	SDL_SetWindowPosition( sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED );

	return rserr_ok;
}

void GLimp_Shutdown()
{
	if ( glContext != nullptr )
	{
		SDL_GL_MakeCurrent( sdlWindow, nullptr );
		SDL_GL_DeleteContext( glContext );
		glContext = nullptr;
	}

	if ( sdlWindow != nullptr )
	{
		SDL_DestroyWindow( sdlWindow );
		sdlWindow = nullptr;
	}
}

bool GLimp_Init( void *hinstance, void *wndproc )
{
	return true;
}

bool GLimp_InitGL()
{
	return true;
}

void GLimp_EndFrame()
{
	SDL_GL_SwapWindow( sdlWindow );
}

#if 0
namespace nox
{
	class IDisplayManager
	{
	public:
		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual rserr_t SetMode( uint *pwidth, uint *pheight, int mode, bool fullscreen ) = 0;

		virtual void AppActivate() = 0;
	};
}// namespace nox
#endif
