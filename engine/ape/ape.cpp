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

#include "ape.h"

//TODO: much of this is subject to change!!!!

bool chr::ApeInstance::Load( const std::string &filename )
{
	void *ptr;

	ssize_t size = FS_LoadFile( filename.c_str(), &ptr );
	if ( size == -1 || ptr == nullptr )
	{
		Com_Printf( "Failed to load ape file (%s)!\n", filename.c_str() );
		return false;
	}

	bool success = Parse( ptr, size );
	if ( !success )
	{
		Com_Printf( "Failed to parse ape file (%s)!\n", filename.c_str() );
	}

	FS_FreeFile( ptr );

	return success;
}

bool chr::ApeInstance::Parse( const void *ptr, size_t size )
{
	struct ApeFileHeader
	{
		uint32_t magic;
		uint32_t unk0;
	};

	ApeFileHeader *header = ( ApeFileHeader * ) ptr;

	uint32_t magic = LittleLong( header->magic );
	if ( magic != 0x3D010000 )
	{
		Com_Printf( "Invalid magic for ape!\n" );
		return false;
	}

	uint32_t unk0 = LittleLong( header->unk0 );
	if ( unk0 != 0xFFFFFFFF )
	{
		Com_Printf( "Secondary key is invalid for ape!\n" );
		return false;
	}

	//TODO: the rest of it :)

	return true;
}
