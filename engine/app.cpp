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
