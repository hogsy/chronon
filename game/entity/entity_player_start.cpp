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

class PlayerStart : public Entity
{
	IMPLEMENT_ENTITY( PlayerStart, Entity )

public:
	explicit PlayerStart( edict_t *edict ) : Entity( edict ) {}
	~PlayerStart() override = default;

	void Spawn( const EntityManager::SpawnVariables &variables ) override;
};

void PlayerStart::Spawn( const EntityManager::SpawnVariables &variables )
{
}

REGISTER_ENTITY_CLASS( info_player_start, PlayerStart )
REGISTER_ENTITY_CLASS( info_player_coop, PlayerStart )
REGISTER_ENTITY_CLASS( info_player_deathmatch, PlayerStart )
REGISTER_ENTITY_CLASS( info_player_intermission, PlayerStart )
