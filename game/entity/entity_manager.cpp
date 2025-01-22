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

#include "../g_local.h"

#include "entity_manager.h"
#include "entity.h"

std::map< std::string, EntityManager::constructor > EntityManager::classRegistry __attribute__( ( init_priority( 1000 ) ) );

bool EntityManager::ParseSpawnVariables( const char **buf, SpawnVariables &variables )
{
	while ( true )
	{
		const char *token = COM_Parse( buf );
		if ( *token == '}' )
		{
			break;
		}

		if ( *buf == nullptr )
		{
			gi.dprintf( "EOF without closing brace!\n" );
			return false;
		}

		std::string key = token;

		token = COM_Parse( buf );
		if ( *buf == nullptr )
		{
			gi.dprintf( "EOF without closing brace!\n" );
			return false;
		}

		if ( *token == '}' )
		{
			gi.dprintf( "Closing brace without data!\n" );
			return false;
		}

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if ( key[ 0 ] == '_' )
		{
			continue;
		}

		variables[ key ] = { key, token };
	}

	return !variables.empty();
}

Entity *EntityManager::CreateEntity( edict_t *edict, const std::string &classname )
{
	const auto spawn = classRegistry.find( classname );
	if ( spawn == classRegistry.end() )
	{
		return nullptr;
	}

	return spawn->second( edict );
}
