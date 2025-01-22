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
#pragma once

#include <map>
#include <memory>

class Entity;
class EntityManager
{
protected:
	typedef Entity *( *constructor )( edict_t *edict );
	static std::map< std::string, constructor > classRegistry;

public:
	class ClassRegistration
	{
	public:
		ClassRegistration( const std::string &name, constructor constructor ) : name( name )
		{
			classRegistry[ name ] = constructor;
		}
		ClassRegistration() = delete;

	private:
		std::string name;
	};

	struct SpawnVariable
	{
		std::string key;
		std::string value;
	};
	typedef std::map< std::string, SpawnVariable > SpawnVariables;

	static bool ParseSpawnVariables( const char **buf, SpawnVariables &variables );

	static EntityManager *GetInstance()
	{
		static EntityManager instance;
		return &instance;
	}

	static Entity *CreateEntity( edict_t *edict, const std::string &classname );

private:
	EntityManager()  = default;
	~EntityManager() = default;
};

#define REGISTER_ENTITY_CLASS( NAME, CLASS )                                           \
	static Entity *NAME##_make( edict_t *edict ) { return new CLASS( edict ); }        \
	static EntityManager::ClassRegistration __attribute__( ( init_priority( 2000 ) ) ) \
	_register_entity_##NAME##_name( #NAME, NAME##_make );
