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

// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.
#include <cassert>

#include "../client/client.h"
#include "winquake.h"

#include "../client/ref_gl/gl_local.h"

cvar_t *win_noalttab;

#ifndef WM_MOUSEWHEEL
#	define WM_MOUSEWHEEL \
		( WM_MOUSELAST + 1 )// message that will be supported by the OS
#endif

static UINT MSH_MOUSEWHEEL;

// Console variables that we need to access from this module
extern cvar_t *vid_ref; // Name of Refresh DLL loaded
cvar_t        *vid_xpos;// X coordinate of window position
cvar_t        *vid_ypos;// Y coordinate of window position
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;

// Global variables used internally by this module
viddef_t viddef;// global video state; used by other modules
bool     reflib_active = 0;

HWND cl_hwnd;// Main window handle for life of program

#define VID_NUM_MODES ( sizeof( vid_modes ) / sizeof( vid_modes[ 0 ] ) )

LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static bool s_alttab_disabled;

extern unsigned sys_msg_time;

/*
** WIN32 helper functions
*/
extern bool s_win95;

static void WIN_DisableAltTab( void )
{
	if ( s_alttab_disabled ) return;

	if ( s_win95 )
	{
		BOOL old;

		SystemParametersInfo( SPI_SCREENSAVERRUNNING, 1, &old, 0 );
	}
	else
	{
		RegisterHotKey( 0, 0, MOD_ALT, VK_TAB );
		RegisterHotKey( 0, 1, MOD_ALT, VK_RETURN );
	}
	s_alttab_disabled = true;
}

static void WIN_EnableAltTab( void )
{
	if ( s_alttab_disabled )
	{
		if ( s_win95 )
		{
			BOOL old;

			SystemParametersInfo( SPI_SCREENSAVERRUNNING, 0, &old, 0 );
		}
		else
		{
			UnregisterHotKey( 0, 0 );
			UnregisterHotKey( 0, 1 );
		}

		s_alttab_disabled = false;
	}
}

/*
==========================================================================

DLL GLUE

==========================================================================
*/

//==========================================================================

byte scantokey[ 128 ] = {
        //  0           1       2       3       4       5       6       7
        //  8           9       A       B       C       D       E       F
        0, 27,
        '1', '2',
        '3', '4',
        '5', '6',
        '7', '8',
        '9', '0',
        '-', '=',
        K_BACKSPACE, 9,// 0
        'q', 'w',
        'e', 'r',
        't', 'y',
        'u', 'i',
        'o', 'p',
        '[', ']',
        13, K_CTRL,
        'a', 's',// 1
        'd', 'f',
        'g', 'h',
        'j', 'k',
        'l', ';',
        '\'', '`',
        K_SHIFT, '\\',
        'z', 'x',
        'c', 'v',// 2
        'b', 'n',
        'm', ',',
        '.', '/',
        K_SHIFT, '*',
        K_ALT, ' ',
        0, K_F1,
        K_F2, K_F3,
        K_F4, K_F5,// 3
        K_F6, K_F7,
        K_F8, K_F9,
        K_F10, K_PAUSE,
        0, K_HOME,
        K_UPARROW, K_PGUP,
        K_KP_MINUS, K_LEFTARROW,
        K_KP_5, K_RIGHTARROW,
        K_KP_PLUS, K_END,// 4
        K_DOWNARROW, K_PGDN,
        K_INS, K_DEL,
        0, 0,
        0, K_F11,
        K_F12, 0,
        0, 0,
        0, 0,
        0, 0,// 5
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,// 6
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0// 7
};

/*
=======
MapKey

Map from windows to quake keynums
=======
*/
int MapKey( int key )
{
	int  result;
	int  modified    = ( key >> 16 ) & 255;
	bool is_extended = false;

	if ( modified > 127 ) return 0;

	if ( key & ( 1 << 24 ) ) is_extended = true;

	result = scantokey[ modified ];

	if ( !is_extended )
	{
		switch ( result )
		{
			case K_HOME:
				return K_KP_HOME;
			case K_UPARROW:
				return K_KP_UPARROW;
			case K_PGUP:
				return K_KP_PGUP;
			case K_LEFTARROW:
				return K_KP_LEFTARROW;
			case K_RIGHTARROW:
				return K_KP_RIGHTARROW;
			case K_END:
				return K_KP_END;
			case K_DOWNARROW:
				return K_KP_DOWNARROW;
			case K_PGDN:
				return K_KP_PGDN;
			case K_INS:
				return K_KP_INS;
			case K_DEL:
				return K_KP_DEL;
			default:
				return result;
		}
	}
	else
	{
		switch ( result )
		{
			case 0x0D:
				return K_KP_ENTER;
			case 0x2F:
				return K_KP_SLASH;
			case 0xAF:
				return K_KP_PLUS;
		}
		return result;
	}
}

void AppActivate( BOOL fActive, BOOL minimize )
{
	Minimized = minimize;

	Key_ClearStates();

	// we don't want to act like we're active if we're minimized
	if ( fActive && !Minimized )
		ActiveApp = true;
	else
		ActiveApp = false;

	// minimize/restore mouse-capture on demand
	if ( !ActiveApp )
	{
		IN_Activate( false );
		CDAudio_Activate( false );
		S_Activate( false );

		if ( win_noalttab->value )
		{
			WIN_EnableAltTab();
		}
	}
	else
	{
		IN_Activate( true );
		CDAudio_Activate( true );
		S_Activate( true );
		if ( win_noalttab->value )
		{
			WIN_DisableAltTab();
		}
	}
}

/*
====================
MainWndProc

main window procedure
====================
*/
LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( uMsg == MSH_MOUSEWHEEL )
	{
		if ( ( ( int ) wParam ) > 0 )
		{
			Key_Event( K_MWHEELUP, true, sys_msg_time );
			Key_Event( K_MWHEELUP, false, sys_msg_time );
		}
		else
		{
			Key_Event( K_MWHEELDOWN, true, sys_msg_time );
			Key_Event( K_MWHEELDOWN, false, sys_msg_time );
		}
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

	switch ( uMsg )
	{
		case WM_MOUSEWHEEL:
			/*
		** this chunk of code theoretically only works under NT4 and Win98
		** since this message doesn't exist under Win95
		*/
			if ( ( short ) HIWORD( wParam ) > 0 )
			{
				Key_Event( K_MWHEELUP, true, sys_msg_time );
				Key_Event( K_MWHEELUP, false, sys_msg_time );
			}
			else
			{
				Key_Event( K_MWHEELDOWN, true, sys_msg_time );
				Key_Event( K_MWHEELDOWN, false, sys_msg_time );
			}
			break;

		case WM_HOTKEY:
			return 0;

		case WM_CREATE:
			cl_hwnd = hWnd;

			MSH_MOUSEWHEEL = RegisterWindowMessage( "MSWHEEL_ROLLMSG" );
			return DefWindowProc( hWnd, uMsg, wParam, lParam );

		case WM_PAINT:
			SCR_DirtyScreen();// force entire screen to update next frame
			return DefWindowProc( hWnd, uMsg, wParam, lParam );

		case WM_DESTROY:
			// let sound and input know about this?
			cl_hwnd = nullptr;
			return DefWindowProc( hWnd, uMsg, wParam, lParam );

		case WM_ACTIVATE:
		{
			int fActive, fMinimized;

			// KJB: Watch this for problems in fullscreen modes with Alt-tabbing.
			fActive    = LOWORD( wParam );
			fMinimized = ( BOOL ) HIWORD( wParam );

			AppActivate( fActive != WA_INACTIVE, fMinimized );

			GLimp_AppActivate( !( fActive == WA_INACTIVE ) );
		}
			return DefWindowProc( hWnd, uMsg, wParam, lParam );

		case WM_MOVE:
		{
			int  xPos, yPos;
			RECT r;
			int  style;

			if ( !vid_fullscreen->value )
			{
				xPos = ( short ) LOWORD( lParam );// horizontal position
				yPos = ( short ) HIWORD( lParam );// vertical position

				r.left   = 0;
				r.top    = 0;
				r.right  = 1;
				r.bottom = 1;

				style = GetWindowLong( hWnd, GWL_STYLE );
				AdjustWindowRect( &r, style, FALSE );

				Cvar_SetValue( "vid_xpos", xPos + r.left );
				Cvar_SetValue( "vid_ypos", yPos + r.top );
				vid_xpos->modified = false;
				vid_ypos->modified = false;
				if ( ActiveApp ) IN_Activate( true );
			}
		}
			return DefWindowProc( hWnd, uMsg, wParam, lParam );

		// this is complicated because Win32 seems to pack multiple mouse events
		// into one update sometimes, so we always check all states and look for
		// events
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
		{
			int temp;

			temp = 0;

			if ( wParam & MK_LBUTTON ) temp |= 1;

			if ( wParam & MK_RBUTTON ) temp |= 2;

			if ( wParam & MK_MBUTTON ) temp |= 4;

			IN_MouseEvent( temp );
		}
		break;

		case WM_SYSCOMMAND:
			if ( wParam == SC_SCREENSAVE ) return 0;
			return DefWindowProc( hWnd, uMsg, wParam, lParam );
		case WM_SYSKEYDOWN:
			if ( wParam == 13 )
			{
				if ( vid_fullscreen )
				{
					Cvar_SetValue( "vid_fullscreen", !vid_fullscreen->value );
				}
				return 0;
			}
			// fall through
		case WM_KEYDOWN:
			Key_Event( MapKey( lParam ), true, sys_msg_time );
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			Key_Event( MapKey( lParam ), false, sys_msg_time );
			break;

		case MM_MCINOTIFY:
		{
			LONG CDAudio_MessageHandler( HWND hWnd, UINT uMsg, WPARAM wParam,
			                             LPARAM lParam );
			CDAudio_MessageHandler( hWnd, uMsg, wParam, lParam );
		}
		break;

		default:// pass all unhandled messages to DefWindowProc
			return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

	/* return 0 if handled message, 1 if not */
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void VID_Restart_f() { vid_ref->modified = true; }

void VID_Front_f()
{
	SetWindowLong( cl_hwnd, GWL_EXSTYLE, WS_EX_TOPMOST );
	SetForegroundWindow( cl_hwnd );
}

/*
** VID_GetModeInfo
*/
typedef struct vidmode_s
{
	const char *description;
	int         width, height;
	int         mode;
} vidmode_t;

vidmode_t vid_modes[] = {
        { "Mode 0: 320x240", 320, 240, 0 },
        { "Mode 1: 400x300", 400, 300, 1 },
        { "Mode 2: 512x384", 512, 384, 2 },
        { "Mode 3: 640x480", 640, 480, 3 },
        { "Mode 4: 800x600", 800, 600, 4 },
        { "Mode 5: 960x720", 960, 720, 5 },
        { "Mode 6: 1024x768", 1024, 768, 6 },
        { "Mode 7: 1152x864", 1152, 864, 7 },
        { "Mode 8: 1280x960", 1280, 960, 8 },
        { "Mode 9: 1600x1200", 1600, 1200, 9 },
        { "Mode 10: 2048x1536", 2048, 1536, 10 } };

bool VID_GetModeInfo( int *width, int *height, int mode )
{
	if ( mode < 0 || mode >= VID_NUM_MODES ) return false;

	*width  = vid_modes[ mode ].width;
	*height = vid_modes[ mode ].height;

	return true;
}

/*
** VID_UpdateWindowPosAndSize
*/
void VID_UpdateWindowPosAndSize( int x, int y )
{
	RECT r;
	int  style;
	int  w, h;

	r.left   = 0;
	r.top    = 0;
	r.right  = viddef.width;
	r.bottom = viddef.height;

	style = GetWindowLong( cl_hwnd, GWL_STYLE );
	AdjustWindowRect( &r, style, FALSE );

	w = r.right - r.left;
	h = r.bottom - r.top;

	MoveWindow( cl_hwnd, x, y, w, h, TRUE );
}

/*
** VID_NewWindow
*/
void VID_NewWindow( int width, int height )
{
	viddef.width  = width;
	viddef.height = height;

	cl.force_refdef = true;// can't use a paused refdef
}

/*
==============
VID_LoadRefresh
==============
*/
bool VID_LoadRefresh()
{
	if ( R_Init( nullptr, ( void * ) MainWndProc ) == -1 )
	{
		R_Shutdown();
		return false;
	}

	Com_Printf( "------------------------------------\n" );

	vidref_val = VIDREF_GL;

	return true;
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole
purpose in life is to check to see if any of the video mode parameters have
changed, and if they have to update the rendering DLL and/or video mode to
match.
============
*/
void VID_CheckChanges( void )
{
	if ( win_noalttab->modified )
	{
		if ( win_noalttab->value )
		{
			WIN_DisableAltTab();
		}
		else
		{
			WIN_EnableAltTab();
		}
		win_noalttab->modified = false;
	}

	if ( vid_ref->modified )
	{
		cl.force_refdef = true;// can't use a paused refdef
		S_StopAllSounds();
	}
	while ( vid_ref->modified )
	{
		/*
		** refresh has changed
		*/
		vid_ref->modified        = false;
		vid_fullscreen->modified = true;
		cl.refresh_prepped       = false;
		cls.disable_screen       = true;

		if ( !VID_LoadRefresh() )
		{
			Com_Error( ERR_FATAL, "Failed to reload refresh!" );
		}
		cls.disable_screen = false;
	}

	/*
	** update our window position
	*/
	if ( vid_xpos->modified || vid_ypos->modified )
	{
		if ( !vid_fullscreen->value )
			VID_UpdateWindowPosAndSize( vid_xpos->value, vid_ypos->value );

		vid_xpos->modified = false;
		vid_ypos->modified = false;
	}
}

/*
============
VID_Init
============
*/
void VID_Init( void )
{
	/* Create the video variables so we know how to start the graphics drivers */
	vid_ref        = Cvar_Get( "vid_ref", "soft", CVAR_ARCHIVE );
	vid_xpos       = Cvar_Get( "vid_xpos", "3", CVAR_ARCHIVE );
	vid_ypos       = Cvar_Get( "vid_ypos", "22", CVAR_ARCHIVE );
	vid_fullscreen = Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	vid_gamma      = Cvar_Get( "vid_gamma", "1", CVAR_ARCHIVE );
	win_noalttab   = Cvar_Get( "win_noalttab", "0", CVAR_ARCHIVE );

	/* Add some console commands that we want to handle */
	Cmd_AddCommand( "vid_restart", VID_Restart_f );
	Cmd_AddCommand( "vid_front", VID_Front_f );

	/* Start the graphics mode and load refresh DLL */
	VID_CheckChanges();
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown( void )
{
	R_Shutdown();
}
