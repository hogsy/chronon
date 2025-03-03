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

bool s_win95;

int  starttime;
bool ActiveApp;
bool Minimized;

static HANDLE hinput, houtput;

unsigned sys_msg_time;
unsigned sys_frame_time;

static HANDLE qwclsemaphore;

#define MAX_NUM_ARGVS 128
int   argc;
char *argv[ MAX_NUM_ARGVS ];

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

/*
================
Sys_ScanForCD

================
*/
char *Sys_ScanForCD( void )
{
	static char cddir[ MAX_OSPATH ];
	static bool done;
#ifndef DEMO
	char  drive[ 4 ];
	FILE *f;
	char  test[ MAX_QPATH ];

	if ( done )// don't re-check
		return cddir;

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	drive[ 0 ] = 'c';
	drive[ 1 ] = ':';
	drive[ 2 ] = '\\';
	drive[ 3 ] = 0;

	done = true;

	// scan the drives
	for ( drive[ 0 ] = 'c'; drive[ 0 ] <= 'z'; drive[ 0 ]++ )
	{
		// where activision put the stuff...
		sprintf( cddir, "%sinstall\\data", drive );
		sprintf( test, "%sinstall\\data\\quake2.exe", drive );
		f = fopen( test, "r" );
		if ( f )
		{
			fclose( f );
			if ( GetDriveType( drive ) == DRIVE_CDROM ) return cddir;
		}
	}
#endif

	cddir[ 0 ] = 0;

	return nullptr;
}

//================================================================

/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents()
{
	MSG msg;

	while ( PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE ) )
	{
		if ( !GetMessage( &msg, nullptr, 0, 0 ) ) Sys_Quit();
		sys_msg_time = msg.time;
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// grab frame time
	sys_frame_time = timeGetTime();// FIXME: should this be at start?
}

/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData()
{
	char *data = nullptr;
	char *cliptext;

	if ( OpenClipboard( nullptr ) != 0 )
	{
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != nullptr )
		{
			if ( ( cliptext = static_cast< char * >( GlobalLock( hClipboardData ) ) ) != nullptr )
			{
				data = static_cast< char * >( malloc( GlobalSize( hClipboardData ) + 1 ) );
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
void Sys_AppActivate()
{
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
void Sys_UnloadGame()
{
	if ( !FreeLibrary( game_library ) )
		Com_Error( ERR_FATAL, "FreeLibrary failed for game library" );
	game_library = nullptr;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI( void *parms )
{
	void *( *GetGameAPI )( void * );
	char  name[ MAX_OSPATH ];
	char *path;
	char  cwd[ MAX_OSPATH ];

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

	if ( game_library )
		Com_Error( ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame" );

	// check the current debug directory first for development purposes
	if ( _getcwd( cwd, sizeof( cwd ) ) == nullptr )
	{
		Com_Error( ERR_FATAL, "Failed to get current working directory!\n" );
	}
	Com_sprintf( name, sizeof( name ), "%s/%s/%s", cwd, debugdir, gamename );
	game_library = LoadLibrary( name );
	if ( game_library )
	{
		Com_DPrintf( "LoadLibrary (%s)\n", name );
	}
	else
	{
#if !defined( NDEBUG )
		// check the current directory for other development purposes
		Com_sprintf( name, sizeof( name ), "%s/%s", cwd, gamename );
		game_library = LoadLibrary( name );
		if ( game_library )
		{
			Com_DPrintf( "LoadLibrary (%s)\n", name );
		}
		else
#endif
		{
			// now run through the search paths
			path = nullptr;
			while ( true )
			{
				path = FS_NextPath( path );
				if ( !path ) return nullptr;// couldn't find one anywhere
				Com_sprintf( name, sizeof( name ), "%s/%s", path, gamename );
				game_library = LoadLibrary( name );
				if ( game_library )
				{
					Com_DPrintf( "LoadLibrary (%s)\n", name );
					break;
				}
			}
		}
	}

	GetGameAPI = ( void *( * ) ( void * ) ) GetProcAddress( game_library, "GetGameAPI" );
	if ( !GetGameAPI )
	{
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
void ParseCommandLine( LPSTR lpCmdLine )
{
	argc      = 1;
	argv[ 0 ] = "exe";

	while ( *lpCmdLine && ( argc < MAX_NUM_ARGVS ) )
	{
		while ( *lpCmdLine && ( ( *lpCmdLine <= 32 ) || ( *lpCmdLine > 126 ) ) )
			lpCmdLine++;

		if ( *lpCmdLine )
		{
			argv[ argc ] = lpCmdLine;
			argc++;

			while ( *lpCmdLine && ( ( *lpCmdLine > 32 ) && ( *lpCmdLine <= 126 ) ) )
				lpCmdLine++;

			if ( *lpCmdLine )
			{
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

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow )
{
	Q_UNUSED( nCmdShow );

	MSG   msg;
	int   time, oldtime, newtime;
	char *cddir;

	/* previous instances do not exist in Win32 */
	if ( hPrevInstance ) return 0;

	ParseCommandLine( lpCmdLine );

	// if we find the CD, add a +set cddir xxx command line
	cddir = Sys_ScanForCD();
	if ( cddir && argc < MAX_NUM_ARGVS - 3 )
	{
		int i;

		// don't override a cddir on the command line
		for ( i = 0; i < argc; i++ )
			if ( !strcmp( argv[ i ], "cddir" ) ) break;
		if ( i == argc )
		{
			argv[ argc++ ] = "+set";
			argv[ argc++ ] = "cddir";
			argv[ argc++ ] = cddir;
		}
	}

	Qcommon_Init( argc, argv );
	oldtime = Sys_Milliseconds();

	/* main window message loop */
	while ( true )
	{
		// if at a full screen console, don't update unless needed
		if ( Minimized || ( dedicated && ( dedicated->value > 0 ) ) )
		{
			Sleep( 1 );
		}

		while ( PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE ) )
		{
			if ( !GetMessage( &msg, nullptr, 0, 0 ) ) Com_Quit();
			sys_msg_time = msg.time;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		do {
			newtime = Sys_Milliseconds();
			time    = newtime - oldtime;
		} while ( time < 1 );
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
