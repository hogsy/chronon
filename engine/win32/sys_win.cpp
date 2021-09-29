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
// sys_win.h

#include <direct.h>
#include <cstdio>

#include "../qcommon/qcommon.h"
#include "resource.h"
#include "winquake.h"

qboolean s_win95;

int starttime;
qboolean ActiveApp;
qboolean Minimized;

static HANDLE hinput, houtput;

unsigned sys_msg_time;
unsigned sys_frame_time;

static HANDLE qwclsemaphore;

#define MAX_NUM_ARGVS 128
int argc;
char *argv[ MAX_NUM_ARGVS ];

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error( const char *error, ... ) {
	CL_Shutdown();
	Qcommon_Shutdown();

	va_list argptr;
	va_start( argptr, error );
	int len = Q_vscprintf( error, argptr ) + 1;
	char *text = static_cast<char*>( Z_Malloc( len ) );
	vsprintf( text, error, argptr );
	va_end( argptr );

	MessageBox( nullptr, text, "Error", 0 /* MB_OK */ );

	Z_Free( text );

	if( qwclsemaphore ) CloseHandle( qwclsemaphore );

	exit( EXIT_FAILURE );
}

void Sys_Quit( void ) {
	timeEndPeriod( 1 );

	CL_Shutdown();
	Qcommon_Shutdown();
	CloseHandle( qwclsemaphore );

	exit( 0 );
}

void WinError( void ) {
	LPTSTR lpMsgBuf;

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, GetLastError(),
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
		lpMsgBuf, 0, nullptr );

	// Display the string.
	MessageBox( nullptr, lpMsgBuf, "GetLastError", MB_OK | MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
}

//================================================================

/*
================
Sys_ScanForCD

================
*/
char *Sys_ScanForCD( void ) {
	static char cddir[ MAX_OSPATH ];
	static qboolean done;
#ifndef DEMO
	char drive[ 4 ];
	FILE *f;
	char test[ MAX_QPATH ];

	if( done )  // don't re-check
		return cddir;

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	drive[ 0 ] = 'c';
	drive[ 1 ] = ':';
	drive[ 2 ] = '\\';
	drive[ 3 ] = 0;

	done = true;

	// scan the drives
	for( drive[ 0 ] = 'c'; drive[ 0 ] <= 'z'; drive[ 0 ]++ ) {
		// where activision put the stuff...
		sprintf( cddir, "%sinstall\\data", drive );
		sprintf( test, "%sinstall\\data\\quake2.exe", drive );
		f = fopen( test, "r" );
		if( f ) {
			fclose( f );
			if( GetDriveType( drive ) == DRIVE_CDROM ) return cddir;
		}
	}
#endif

	cddir[ 0 ] = 0;

	return nullptr;
}

/*
================
Sys_CopyProtect

================
*/
void Sys_CopyProtect( void ) {
#ifndef DEMO
	char *cddir;

	cddir = Sys_ScanForCD();
	if( !cddir[ 0 ] )
		Com_Error( ERR_FATAL, "You must have the Quake2 CD in the drive to play." );
#endif
}

//================================================================

/*
================
Sys_Init
================
*/
void Sys_Init( void ) {
	OSVERSIONINFO vinfo;

	timeBeginPeriod( 1 );

	vinfo.dwOSVersionInfoSize = sizeof( vinfo );

	if( !GetVersionEx( &vinfo ) ) Sys_Error( "Couldn't get OS info" );

	if( vinfo.dwMajorVersion < 4 )
		Sys_Error( ENGINE_NAME " requires windows version 4 or greater" );
	if( vinfo.dwPlatformId == VER_PLATFORM_WIN32s )
		Sys_Error( ENGINE_NAME " doesn't run on Win32s" );
	else if( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
		s_win95 = true;
}

static char console_text[ 256 ];
static int console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput( void ) {
	INPUT_RECORD recs[ 1024 ];
	DWORD dummy;
	int ch;
	DWORD numread, numevents;

	if( !dedicated || dedicated->value <= 0 ) return nullptr;

	for( ;;) {
		if( !GetNumberOfConsoleInputEvents( hinput, &numevents ) )
			Sys_Error( "Error getting # of console events" );

		if( numevents <= 0 ) break;

		if( !ReadConsoleInput( hinput, recs, 1, &numread ) )
			Sys_Error( "Error reading console input" );

		if( numread != 1 ) Sys_Error( "Couldn't read console input" );

		if( recs[ 0 ].EventType == KEY_EVENT ) {
			if( !recs[ 0 ].Event.KeyEvent.bKeyDown ) {
				ch = recs[ 0 ].Event.KeyEvent.uChar.AsciiChar;

				switch( ch ) {
				case '\r':
					WriteFile( houtput, "\r\n", 2, &dummy, nullptr );

					if( console_textlen ) {
						console_text[ console_textlen ] = 0;
						console_textlen = 0;
						return console_text;
					}
					break;

				case '\b':
					if( console_textlen ) {
						console_textlen--;
						WriteFile( houtput, "\b \b", 3, &dummy, nullptr );
					}
					break;

				default:
					if( ch >= ' ' ) {
						if( console_textlen < sizeof( console_text ) - 2 ) {
							WriteFile( houtput, &ch, 1, &dummy, nullptr );
							console_text[ console_textlen ] = ch;
							console_textlen++;
						}
					}

					break;
				}
			}
		}
	}

	return nullptr;
}

/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput( char *string ) {
	DWORD dummy;
	char text[ 256 ];

#if defined( WIN32 )
	OutputDebugString( string );
#endif

	if( !dedicated || dedicated->value <= 0 ) return;

	if( console_textlen ) {
		text[ 0 ] = '\r';
		memset( &text[ 1 ], ' ', console_textlen );
		text[ console_textlen + 1 ] = '\r';
		text[ console_textlen + 2 ] = 0;
		WriteFile( houtput, text, console_textlen + 2, &dummy, nullptr );
	}

	WriteFile( houtput, string, strlen( string ), &dummy, nullptr );

	if( console_textlen )
		WriteFile( houtput, console_text, console_textlen, &dummy, nullptr );
}

/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents( ) {
	MSG msg;

	while( PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE ) ) {
		if( !GetMessage( &msg, nullptr, 0, 0 ) ) Sys_Quit();
		sys_msg_time = msg.time;
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// grab frame time
	sys_frame_time = timeGetTime();  // FIXME: should this be at start?
}

/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( ) {
	char *data = nullptr;
	char *cliptext;

	if( OpenClipboard( nullptr ) != 0 ) {
		HANDLE hClipboardData;

		if( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != nullptr ) {
			if( ( cliptext = static_cast<char*>( GlobalLock( hClipboardData ) ) ) != nullptr ) {
				data = static_cast<char*>( malloc( GlobalSize( hClipboardData ) + 1 ) );
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate( ) {
	ShowWindow( cl_hwnd, SW_RESTORE );
	SetForegroundWindow( cl_hwnd );
}

/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame( ) {
	if( !FreeLibrary( game_library ) )
		Com_Error( ERR_FATAL, "FreeLibrary failed for game library" );
	game_library = nullptr;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI( void *parms ) {
	void *( *GetGameAPI )( void * );
	char name[ MAX_OSPATH ];
	char *path;
	char cwd[ MAX_OSPATH ];

#if defined( _WIN32 )
	const char *gamename = "game.dll";
#else
	const char *gamename = "game.so";
#endif

#if !defined( NDEBUG )
	const char *debugdir = "debug";
#else
	const char *debugdir = "release";
#endif

	if( game_library )
		Com_Error( ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame" );

	// check the current debug directory first for development purposes
	if( _getcwd( cwd, sizeof( cwd ) ) == nullptr ) {
		Com_Error( ERR_FATAL, "Failed to get current working directory!\n" );
	}
	Com_sprintf( name, sizeof( name ), "%s/%s/%s", cwd, debugdir, gamename );
	game_library = LoadLibrary( name );
	if( game_library ) {
		Com_DPrintf( "LoadLibrary (%s)\n", name );
	} else {
#if !defined( NDEBUG )
		// check the current directory for other development purposes
		Com_sprintf( name, sizeof( name ), "%s/%s", cwd, gamename );
		game_library = LoadLibrary( name );
		if( game_library ) {
			Com_DPrintf( "LoadLibrary (%s)\n", name );
		} else
#endif
		{
			// now run through the search paths
			path = nullptr;
			while( true ) {
				path = FS_NextPath( path );
				if( !path ) return nullptr;  // couldn't find one anywhere
				Com_sprintf( name, sizeof( name ), "%s/%s", path, gamename );
				game_library = LoadLibrary( name );
				if( game_library ) {
					Com_DPrintf( "LoadLibrary (%s)\n", name );
					break;
				}
			}
		}
	}

	GetGameAPI = (void*(*)(void*))GetProcAddress( game_library, "GetGameAPI" );
	if( !GetGameAPI ) {
		Sys_UnloadGame();
		return nullptr;
	}

	return GetGameAPI( parms );
}

//=======================================================================

/*
==================
ParseCommandLine

==================
*/
void ParseCommandLine( LPSTR lpCmdLine ) {
	argc = 1;
	argv[ 0 ] = "exe";

	while( *lpCmdLine && ( argc < MAX_NUM_ARGVS ) ) {
		while( *lpCmdLine && ( ( *lpCmdLine <= 32 ) || ( *lpCmdLine > 126 ) ) )
			lpCmdLine++;

		if( *lpCmdLine ) {
			argv[ argc ] = lpCmdLine;
			argc++;

			while( *lpCmdLine && ( ( *lpCmdLine > 32 ) && ( *lpCmdLine <= 126 ) ) )
				lpCmdLine++;

			if( *lpCmdLine ) {
				*lpCmdLine = 0;
				lpCmdLine++;
			}
		}
	}
}

/*
==================
WinMain

==================
*/
HINSTANCE global_hInstance;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow ) {
	Q_UNUSED( nCmdShow );

	MSG msg;
	int time, oldtime, newtime;
	char *cddir;

	/* previous instances do not exist in Win32 */
	if( hPrevInstance ) return 0;

	global_hInstance = hInstance;

	ParseCommandLine( lpCmdLine );

	// if we find the CD, add a +set cddir xxx command line
	cddir = Sys_ScanForCD();
	if( cddir && argc < MAX_NUM_ARGVS - 3 ) {
		int i;

		// don't override a cddir on the command line
		for( i = 0; i < argc; i++ )
			if( !strcmp( argv[ i ], "cddir" ) ) break;
		if( i == argc ) {
			argv[ argc++ ] = "+set";
			argv[ argc++ ] = "cddir";
			argv[ argc++ ] = cddir;
		}
	}

	Qcommon_Init( argc, argv );
	oldtime = Sys_Milliseconds();

	/* main window message loop */
	while( true ) {
		// if at a full screen console, don't update unless needed
		if( Minimized || ( dedicated && ( dedicated->value > 0 ) ) ) {
			Sleep( 1 );
		}

		while( PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE ) ) {
			if( !GetMessage( &msg, nullptr, 0, 0 ) ) Com_Quit();
			sys_msg_time = msg.time;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		do {
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} while( time < 1 );
		//			Con_Printf ("time:%5.2f - %5.2f = %5.2f\n", newtime,
		//oldtime, time);

#if defined( Q_PLATFORM_X86 )
		_controlfp( _PC_24, _MCW_PC );
#endif
		Qcommon_Frame( time );

		oldtime = newtime;
	}

	// never gets here
	return TRUE;
}
