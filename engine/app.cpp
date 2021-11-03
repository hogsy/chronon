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

#include <SDL2/SDL.h>

#include "qcommon/qcommon.h"

#include "app.h"
#include "client/keys.h"

nox::App *nox::globalApp = nullptr;

void nox::App::Initialize()
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

unsigned int sys_msg_time = 0;  // todo: kill
unsigned int sys_frame_time = 0;// todo: kill

void nox::App::SendKeyEvents()
{
	SDL_PumpEvents();
	sys_msg_time = Sys_Milliseconds();
}

unsigned int nox::App::GetNumTicks()
{
	lastTick_ = SDL_GetTicks();
	return lastTick_;
}

char *nox::App::GetClipboardData()
{
	if ( !SDL_HasClipboardText() )
	{
		return nullptr;
	}

	return SDL_GetClipboardText();
}

int nox::App::MapKey( int key )
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

		case SDLK_PAGEUP:
			return K_PGUP;
		case SDLK_PAGEDOWN:
			return K_PGDN;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			return K_SHIFT;

		case SDLK_F1:
            return K_F1;
		case SDLK_F2:
			return K_F2;
		case SDLK_F3:
			return K_F3;
		case SDLK_F4:
			return K_F4;
		case SDLK_F5:
			return K_F5;
		case SDLK_F6:
			return K_F6;
		case SDLK_F7:
			return K_F7;
		case SDLK_F8:
			return K_F8;
		case SDLK_F9:
			return K_F9;
		case SDLK_F10:
			return K_F10;
		case SDLK_F11:
			return K_F11;
		case SDLK_F12:
			return K_F12;

		default:
			break;
	}

	if ( key >= K_MAX )
	{
		assert( 0 );
		return K_INVALID;
	}

	return key;
}

void IN_MouseEvent( int mstate );

bool ActiveApp = true;// todo: murder.
void nox::App::PollEvents()
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
			{
				int key = MapKey( event.key.keysym.sym );
				if ( key == K_INVALID )
				{
					break;
				}

				Key_Event( key, ( event.key.state == SDL_PRESSED ), sys_msg_time );
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
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
	nox::globalApp = new nox::App( argc, argv );
	nox::globalApp->Initialize();

	Qcommon_Init( argc, argv );

	int time, newTime;
	int oldTime = Sys_Milliseconds();
	while ( true )
	{
		nox::globalApp->PollEvents();

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
