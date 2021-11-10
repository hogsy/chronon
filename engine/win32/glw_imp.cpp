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
/*
** GLW_IMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/
#include <cassert>
#include <windows.h>

#include "../client/ref_gl/gl_local.h"

#include "glw_win.h"
#include "winquake.h"

static qboolean GLimp_SwitchFullscreen( int width, int height );
qboolean GLimp_InitGL( void );

glwstate_t glw_state;

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;

static qboolean VerifyDriver( void ) {
	char buffer[ 1024 ];

	strcpy( buffer, (const char *)glGetString( GL_RENDERER ) );
	strlwr( buffer );
	if( strcmp( buffer, "gdi generic" ) == 0 )
		if( !glw_state.mcd_accelerated )
			return false;
	return true;
}

/*
** VID_CreateWindow
*/
#define	WINDOW_CLASS_NAME "hosae"

qboolean VID_CreateWindow( int width, int height, qboolean fullscreen ) {
	WNDCLASS		wc;
	RECT			r;
	cvar_t *vid_xpos, *vid_ypos;
	int				stylebits;
	int				x, y, w, h;
	int				exstyle;

	/* Register the frame class */
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)glw_state.wndproc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = glw_state.hInstance;
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursor( nullptr, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = WINDOW_CLASS_NAME;

	if( !RegisterClass( &wc ) )
		Com_Error( ERR_FATAL, "Couldn't register window class" );

	if( fullscreen ) {
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE;
	} else {
		exstyle = 0;
		stylebits = WINDOW_STYLE;
	}

	r.left = 0;
	r.top = 0;
	r.right = width;
	r.bottom = height;

	AdjustWindowRect( &r, stylebits, FALSE );

	w = r.right - r.left;
	h = r.bottom - r.top;

	if( fullscreen ) {
		x = 0;
		y = 0;
	} else {
		vid_xpos = Cvar_Get( "vid_xpos", "0", 0 );
		vid_ypos = Cvar_Get( "vid_ypos", "0", 0 );
		x = (int)vid_xpos->value;
		y = (int)vid_ypos->value;
	}

	glw_state.hWnd = CreateWindowEx(
		exstyle,
		WINDOW_CLASS_NAME,
		WINDOW_CLASS_NAME,
		stylebits,
		x, y, w, h,
		nullptr,
		nullptr,
		glw_state.hInstance,
		nullptr );

	if( !glw_state.hWnd )
		Com_Error( ERR_FATAL, "Couldn't create window" );

	ShowWindow( glw_state.hWnd, SW_SHOW );
	UpdateWindow( glw_state.hWnd );

	// init all the gl stuff for the window
	if( !GLimp_InitGL() ) {
		Com_Printf( "VID_CreateWindow() - GLimp_InitGL failed\n" );
		return false;
	}

	SetForegroundWindow( glw_state.hWnd );
	SetFocus( glw_state.hWnd );

	// let the sound and input subsystems know about the new window
	VID_NewWindow( width, height );

	return true;
}


/*
** GLimp_SetMode
*/
rserr_t GLimp_SetMode( unsigned int *pwidth, unsigned int *pheight, int mode, qboolean fullscreen ) {
	int width, height;
	const char *win_fs[] = { "W", "FS" };

	Com_Printf( "Initializing OpenGL display\n" );

	Com_Printf( "...setting mode %d:", mode );

	if( !VID_GetModeInfo( &width, &height, mode ) ) {
		Com_Printf( " invalid mode\n" );
		return rserr_invalid_mode;
	}

	Com_Printf( " %d %d %s\n", width, height, win_fs[ fullscreen ] );

	// destroy the existing window
	if( glw_state.hWnd ) {
		GLimp_Shutdown();
	}

	// do a CDS if needed
	if( fullscreen ) {
		DEVMODE dm;

		Com_Printf( "...attempting fullscreen\n" );

		memset( &dm, 0, sizeof( dm ) );

		dm.dmSize = sizeof( dm );

		dm.dmPelsWidth = width;
		dm.dmPelsHeight = height;
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

		if( gl_bitdepth->value != 0 ) {
			dm.dmBitsPerPel = (DWORD)gl_bitdepth->value;
			dm.dmFields |= DM_BITSPERPEL;
			Com_Printf( "...using gl_bitdepth of %d\n", (int)gl_bitdepth->value );
		} else {
			HDC hdc = GetDC( NULL );
			int bitspixel = GetDeviceCaps( hdc, BITSPIXEL );

			Com_Printf( "...using desktop display depth of %d\n", bitspixel );

			ReleaseDC( 0, hdc );
		}

		Com_Printf( "...calling CDS: " );
		if( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL ) {
			*pwidth = width;
			*pheight = height;

			gl_state.fullscreen = true;

			Com_Printf( "ok\n" );

			if( !VID_CreateWindow( width, height, true ) )
				return rserr_invalid_mode;

			return rserr_ok;
		} else {
			*pwidth = width;
			*pheight = height;

			Com_Printf( "failed\n" );

			Com_Printf( "...calling CDS assuming dual monitors:" );

			dm.dmPelsWidth = width * 2;
			dm.dmPelsHeight = height;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			if( gl_bitdepth->value != 0 ) {
				dm.dmBitsPerPel = (DWORD)gl_bitdepth->value;
				dm.dmFields |= DM_BITSPERPEL;
			}

			/*
			** our first CDS failed, so maybe we're running on some weird dual monitor
			** system
			*/
			if( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL ) {
				Com_Printf( " failed\n" );

				Com_Printf( "...setting windowed mode\n" );

				ChangeDisplaySettings( 0, 0 );

				*pwidth = width;
				*pheight = height;
				gl_state.fullscreen = false;
				if( !VID_CreateWindow( width, height, false ) )
					return rserr_invalid_mode;
				return rserr_invalid_fullscreen;
			} else {
				Com_Printf( " ok\n" );
				if( !VID_CreateWindow( width, height, true ) )
					return rserr_invalid_mode;

				gl_state.fullscreen = true;
				return rserr_ok;
			}
		}
	} else {
		Com_Printf( "...setting windowed mode\n" );

		ChangeDisplaySettings( 0, 0 );

		*pwidth = width;
		*pheight = height;
		gl_state.fullscreen = false;
		if( !VID_CreateWindow( width, height, false ) )
			return rserr_invalid_mode;
	}

	return rserr_ok;
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( void ) {
	if( !wglMakeCurrent( NULL, NULL ) )
		Com_Printf( "ref_gl::R_Shutdown() - wglMakeCurrent failed\n" );
	if( glw_state.hGLRC ) {
		if( !wglDeleteContext( glw_state.hGLRC ) )
			Com_Printf( "ref_gl::R_Shutdown() - wglDeleteContext failed\n" );
		glw_state.hGLRC = NULL;
	}
	if( glw_state.hDC ) {
		if( !ReleaseDC( glw_state.hWnd, glw_state.hDC ) )
			Com_Printf( "ref_gl::R_Shutdown() - ReleaseDC failed\n" );
		glw_state.hDC = NULL;
	}
	if( glw_state.hWnd ) {
		DestroyWindow( glw_state.hWnd );
		glw_state.hWnd = NULL;
	}

	if( glw_state.log_fp ) {
		fclose( glw_state.log_fp );
		glw_state.log_fp = 0;
	}

	UnregisterClass( WINDOW_CLASS_NAME, glw_state.hInstance );

	if( gl_state.fullscreen ) {
		ChangeDisplaySettings( 0, 0 );
		gl_state.fullscreen = false;
	}
}


/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  Under Win32 this means dealing with the pixelformats and
** doing the wgl interface stuff.
*/
qboolean GLimp_Init( void *hinstance, void *wndproc ) {
#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof( vinfo );

	glw_state.allowdisplaydepthchange = false;

	if( GetVersionEx( &vinfo ) ) {
		if( vinfo.dwMajorVersion > 4 ) {
			glw_state.allowdisplaydepthchange = true;
		} else if( vinfo.dwMajorVersion == 4 ) {
			if( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) {
				glw_state.allowdisplaydepthchange = true;
			} else if( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
				if( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER ) {
					glw_state.allowdisplaydepthchange = true;
				}
			}
		}
	} else {
		Com_Printf( "GLimp_Init() - GetVersionEx failed\n" );
		return false;
	}

	glw_state.hInstance = (HINSTANCE)hinstance;
	glw_state.wndproc = wndproc;

	return true;
}

qboolean GLimp_InitGL( void ) {
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof( PIXELFORMATDESCRIPTOR ),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		24,								// 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		32,								// 32-bit z-buffer	
		0,								// no stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
	};
	int  pixelformat;
	cvar_t *stereo;

	stereo = Cvar_Get( "cl_stereo", "0", 0 );

	/*
	** set PFD_STEREO if necessary
	*/
	if( stereo->value != 0 ) {
		Com_Printf( "...attempting to use stereo\n" );
		pfd.dwFlags |= PFD_STEREO;
		gl_state.stereo_enabled = true;
	} else {
		gl_state.stereo_enabled = false;
	}

	/*
	** figure out if we're running on a minidriver or not
	*/
	glw_state.minidriver = false;

	/*
	** Get a DC for the specified window
	*/
	if( glw_state.hDC != NULL )
		Com_Printf( "GLimp_Init() - non-NULL DC exists\n" );

	if( ( glw_state.hDC = GetDC( glw_state.hWnd ) ) == NULL ) {
		Com_Printf( "GLimp_Init() - GetDC failed\n" );
		return false;
	}

	if( ( pixelformat = ChoosePixelFormat( glw_state.hDC, &pfd ) ) == 0 ) {
		Com_Printf( "GLimp_Init() - ChoosePixelFormat failed\n" );
		return false;
	}
	if( SetPixelFormat( glw_state.hDC, pixelformat, &pfd ) == FALSE ) {
		Com_Printf( "GLimp_Init() - SetPixelFormat failed\n" );
		return false;
	}
	DescribePixelFormat( glw_state.hDC, pixelformat, sizeof( pfd ), &pfd );

	if( !( pfd.dwFlags & PFD_GENERIC_ACCELERATED ) ) {
		extern cvar_t *gl_allow_software;

		if( gl_allow_software->value )
			glw_state.mcd_accelerated = true;
		else
			glw_state.mcd_accelerated = false;
	} else {
		glw_state.mcd_accelerated = true;
	}

	/*
	** report if stereo is desired but unavailable
	*/
	if( !( pfd.dwFlags & PFD_STEREO ) && ( stereo->value != 0 ) ) {
		Com_Printf( "...failed to select stereo pixel format\n" );
		Cvar_SetValue( "cl_stereo", 0 );
		gl_state.stereo_enabled = false;
	}

	/*
	** startup the OpenGL subsystem by creating a context and making
	** it current
	*/
	if( ( glw_state.hGLRC = wglCreateContext( glw_state.hDC ) ) == 0 ) {
		Com_Printf( "GLimp_Init() - qwglCreateContext failed\n" );

		goto fail;
	}

	if( !wglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) ) {
		Com_Printf( "GLimp_Init() - qwglMakeCurrent failed\n" );

		goto fail;
	}

	if( !VerifyDriver() ) {
		Com_Printf( "GLimp_Init() - no hardware acceleration detected\n" );
		goto fail;
	}

	/*
	** print out PFD specifics
	*/
	Com_Printf( "GL PFD: color(%d-bits) Z(%d-bit)\n", (int)pfd.cColorBits, (int)pfd.cDepthBits );

	return true;

fail:
	if( glw_state.hGLRC ) {
		wglDeleteContext( glw_state.hGLRC );
		glw_state.hGLRC = NULL;
	}

	if( glw_state.hDC ) {
		ReleaseDC( glw_state.hWnd, glw_state.hDC );
		glw_state.hDC = NULL;
	}
	return false;
}

/*
** GLimp_EndFrame
**
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame( void ) {
#if 0
#if defined( _DEBUG )
	GLenum err = glGetError();
	assert( err == GL_NO_ERROR );
#endif
#endif

	if( stricmp( gl_drawbuffer->string, "GL_BACK" ) == 0 ) {
		if( !SwapBuffers( glw_state.hDC ) )
			Com_Error( ERR_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n" );
	}
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active ) {
	if( active ) {
		SetForegroundWindow( glw_state.hWnd );
		ShowWindow( glw_state.hWnd, SW_RESTORE );
	} else {
		if( vid_fullscreen->value )
			ShowWindow( glw_state.hWnd, SW_MINIMIZE );
	}
}
