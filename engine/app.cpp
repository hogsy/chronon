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

#include <SDL2/SDL.h>

#if defined( _WIN32 )
#	include <windows.h>
#	include <debugapi.h>
#endif

#include "qcommon/qcommon.h"

#include "client/keys.h"
#include "client/input.h"

chr::App *chr::globalApp = nullptr;

void chr::App::Initialize()
{
	int status = SDL_Init( SDL_INIT_EVERYTHING );
	if ( status != 0 )
		Sys_Error( "Failed to initialized SDL2: %s\n", SDL_GetError() );

	Qcommon_Init( argc_, argv_ );
}

[[noreturn]] void chr::App::Run()
{
	unsigned int time, newTime;
	unsigned int oldTime = GetNumMilliseconds();
	while ( true )
	{
		chr::globalApp->PollEvents();

		do
		{
			newTime = GetNumMilliseconds();
			time    = newTime - oldTime;
		} while ( time < 1 );

#if defined( Q_PLATFORM_X86 )
		_controlfp( _PC_24, _MCW_PC );
#endif
		Qcommon_Frame( time );

		oldTime = newTime;
	}
}

unsigned int sys_frame_time = 0;// todo: kill

void chr::App::SendKeyEvents()
{
	SDL_PumpEvents();

	PollEvents();

	sys_frame_time = GetNumMilliseconds();// FIXME: should this be at start?
}

unsigned int chr::App::GetNumMilliseconds()
{
	lastMs_ = SDL_GetTicks();
	return lastMs_;
}

char *chr::App::GetClipboardData()
{
	if ( !SDL_HasClipboardText() )
	{
		return nullptr;
	}

	return SDL_GetClipboardText();
}

bool IN_HandleEvent( const SDL_Event &event );
void chr::App::PollEvents()
{
	SDL_Event event;
	while ( SDL_PollEvent( &event ) )
	{
		if ( IN_HandleEvent( event ) )
		{
			continue;
		}

		switch ( event.type )
		{
			case SDL_WINDOWEVENT:
			{
				switch ( event.window.event )
				{
					case SDL_WINDOWEVENT_ENTER:
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						IN_Activate( true );
						break;
					case SDL_WINDOWEVENT_LEAVE:
					case SDL_WINDOWEVENT_FOCUS_LOST:
						IN_Activate( false );
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

/**
 * This pushes the given string to the native terminal/console.
 */
void chr::App::PushConsoleOutput( const char *text )
{
#if defined( _WIN32 ) && defined( _MSC_VER )
	OutputDebugString( text );
#else
	printf( "%s", text );
#endif
}

void chr::App::ShowCursor( bool show )
{
	SDL_ShowCursor( show );
}

#if defined( _WIN32 )
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	int    argc = __argc;
	char **argv = __argv;
#else
extern "C" int main( int argc, char **argv )
{
#endif
	chr::globalApp = new chr::App( argc, argv );

	// todo: consider combining these??
	chr::globalApp->Initialize();
	chr::globalApp->Run();
}
