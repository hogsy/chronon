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

#include "../qcommon/qcommon.h"

static int hunkcount;

static byte *membase;

static size_t hunkmaxsize;
static size_t cursize;

void *Hunk_Begin( size_t maxsize )
{
	// reserve a huge chunk of memory, but don't commit any yet
	cursize = 0;
	hunkmaxsize = maxsize;
	membase = ( byte * ) calloc( maxsize, sizeof( byte ) );
	if ( membase == nullptr )
	{
		Sys_Error( "VirtualAlloc reserve failed" );
	}

	return ( void * ) membase;
}

void *Hunk_Alloc( size_t size )
{
	// round to cacheline
	size = ( size + 31 ) & ~31;

	cursize += size;
	if ( cursize > hunkmaxsize )
	{
		Sys_Error( "Hunk_Alloc overflow" );
	}

	return ( void * ) ( membase + cursize - size );
}

size_t Hunk_End()
{
	// free the remaining unused virtual memory
	hunkcount++;
	//Com_Printf ("hunkcount: %i\n", hunkcount);
	return cursize;
}

void Hunk_Free( void *base )
{
	if ( base )
	{
		free( base );
	}

	hunkcount--;
}
