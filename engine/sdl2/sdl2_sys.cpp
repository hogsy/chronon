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

#include "qcommon/qcommon.h"

#include <SDL2/SDL.h>

#if defined( _WIN32 )
#	include <debugapi.h>
#endif

#include "../client/keys.h"

/*
========================================================================
========================================================================
*/

unsigned int sys_msg_time;
unsigned int sys_frame_time;
void         Sys_SendKeyEvents()
{
	SDL_PumpEvents();

	sys_msg_time = Sys_Milliseconds();
}

int curtime;
int Sys_Milliseconds()
{
	// todo: curtime should be unsigned
	curtime = ( int ) SDL_GetTicks();
	return curtime;
}

/*
========================================================================
========================================================================
*/

void nox::Sys_MessageBox( const char *error, MessageBoxType boxType )
{
	Uint32 flags = 0;
	switch ( boxType )
	{
		case MessageBoxType::MB_ERROR:
			flags |= SDL_MESSAGEBOX_ERROR;
			break;
		case MessageBoxType::MB_WARNING:
			flags |= SDL_MESSAGEBOX_WARNING;
			break;
		case MessageBoxType::MB_INFO:
			flags |= SDL_MESSAGEBOX_INFORMATION;
			break;
	}

	SDL_ShowSimpleMessageBox( flags, ENGINE_NAME, error, nullptr );
}

void Sys_Init()
{
	int status = SDL_Init(
			SDL_INIT_VIDEO |
			SDL_INIT_EVENTS |
			SDL_INIT_GAMECONTROLLER |
			SDL_INIT_HAPTIC |
			SDL_INIT_JOYSTICK |
			SDL_INIT_TIMER );
	if ( status != 0 )
	{
		Sys_Error( "Failed to initialized SDL2: %s\n", SDL_GetError() );
	}
}

char *Sys_ConsoleInput()
{
	return nullptr;
}

void Sys_ConsoleOutput( char *string )
{
#if defined( _WIN32 ) && defined( _MSC_VER )
	OutputDebugString( string );
#else
	printf( "%s", string );
#endif
}

void Sys_AppActivate()
{
}

/*
========================================================================
GAME DLL
========================================================================
*/

static void *gameDll = nullptr;

void Sys_UnloadGame()
{
	if ( gameDll == nullptr )
	{
		return;
	}

	SDL_UnloadObject( gameDll );
}

#define GAME_MODULE_NAME "game"
#if defined( _WIN32 )
#	define GAME_MODULE_EXT ".dll"
#else
#	define GAME_MODULE_EXT ".so"
#endif
#define GAME_MODULE_PATH GAME_MODULE_NAME GAME_MODULE_EXT

/**
 * Loads the game dll.
 */
void *Sys_GetGameAPI( void *parms )
{
	if ( gameDll != nullptr )
	{
		Com_Error( ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame" );
	}

	char *path = nullptr;
	while ( ( path = FS_NextPath( path ) ) != nullptr )
	{
		char fullPath[ MAX_OSPATH ];
		snprintf( fullPath, sizeof( fullPath ), "%s/%s", path, GAME_MODULE_PATH );
		gameDll = SDL_LoadObject( fullPath );
		if ( gameDll != nullptr )
		{
			Com_DPrintf( "LoadLibrary (%s)\n", fullPath );
			break;
		}
	}

	typedef void *( *GetGameAPI )( void * );
	auto gameApi = ( GetGameAPI ) SDL_LoadFunction( gameDll, "GetGameAPI" );
	if ( gameApi == nullptr )
	{
		Sys_UnloadGame();
		return nullptr;
	}

	return gameApi( parms );
}

/*
========================================================================
========================================================================
*/

static int Sys_MapSDLKey( int key )
{
	switch ( key )
	{
		case SDLK_UP:
			return K_UPARROW;
		case SDLK_DOWN:
			return K_DOWNARROW;
		case SDLK_LEFT:
			return K_LEFTARROW;
		case SDLK_RIGHT:
			return K_RIGHTARROW;

		case SDLK_BACKSPACE:
			return K_BACKSPACE;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			return K_SHIFT;

		default:
			return key;
	}
}

bool ActiveApp = true;// todo: murder.

static void Sys_PollEvents()
{
	SDL_Event event;
	while ( SDL_PollEvent( &event ) )
	{
		switch ( event.type )
		{
			case SDL_MOUSEWHEEL:
				if ( event.wheel.y > 0 )
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

			case SDL_KEYDOWN:
			case SDL_KEYUP:
				Key_Event( Sys_MapSDLKey( event.key.keysym.sym ), ( event.key.state == SDL_PRESSED ), sys_msg_time );
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				void IN_MouseEvent( int mstate );
				int  button;
				switch ( event.button.button )
				{
					case SDL_BUTTON_LEFT:
						button = 1;
						break;
					case SDL_BUTTON_RIGHT:
						button = 2;
						break;
					case SDL_BUTTON_MIDDLE:
						button = 4;
						break;
					default:
						button = 0;
						break;
				}
				IN_MouseEvent( button );
			}

			case SDL_WINDOWEVENT:
			{
				switch ( event.window.event )
				{
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						ActiveApp = true;
						Key_ClearStates();
						break;
					case SDL_WINDOWEVENT_FOCUS_LOST:
						ActiveApp = false;
						Key_ClearStates();
						break;
					case SDL_WINDOWEVENT_CLOSE:
						Com_Quit();
						break;
				}
				break;
			}

			case SDL_QUIT:
				Com_Quit();
				break;
		}
	}
}

#pragma clang diagnostic push
#pragma ide diagnostic   ignored "EndlessLoop"

extern "C" int main( int argc, char **argv )
{
	Qcommon_Init( argc, argv );

	int time, newTime;
	int oldTime = Sys_Milliseconds();
	while ( true )
	{
		Sys_PollEvents();

		do
		{
			newTime = Sys_Milliseconds();
			time = newTime - oldTime;
		} while ( time < 1 );

		Qcommon_Frame( time );

		oldTime = newTime;
	}
}

#pragma clang diagnostic pop
