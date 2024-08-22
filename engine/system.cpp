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
/*
========================================================================
 Portable System Functions
========================================================================
*/

#include <SDL2/SDL_messagebox.h>

#include "qcommon/qcommon.h"

void chr::Sys_MessageBox( const char *error, MessageBoxType boxType )
{
	uint32_t flags = 0;
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

void Sys_Error( const char *error, ... )
{
	va_list argptr;
	va_start( argptr, error );
	int   len  = Q_vscprintf( error, argptr ) + 1;
	char *text = new char[ len ];
	vsprintf( text, error, argptr );
	va_end( argptr );

	Sys_MessageBox( text, chr::MessageBoxType::MB_ERROR );

	delete[] text;

	Sys_Quit();
}

void Sys_Quit()
{
	CL_Shutdown();
	Qcommon_Shutdown();

	exit( EXIT_SUCCESS );
}
