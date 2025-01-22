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

#include "entity.h"

Entity::Entity( edict_t *edict ) : edict( edict )
{
}

void Entity::SetModel( const std::string &path ) const
{
	gi.setmodel( edict, path.c_str() );
}

void Entity::SetSolid( const solid_t solid ) const
{
	edict->solid = solid;
}

void Entity::SetOrigin( const vec3_t &origin ) const
{
	VectorCopy( edict->s.origin, edict->s.old_origin );
	VectorCopy( origin, edict->s.origin );
}

void Entity::SetAngles( const vec3_t &angles ) const
{
	VectorCopy( angles, edict->s.angles );
}

void Entity::SetSize( const vec3_t &mins, const vec3_t &maxs ) const
{
	VectorCopy( mins, edict->mins );
	VectorCopy( maxs, edict->maxs );
}

void Entity::Link() const
{
	gi.linkentity( edict );
}

void Entity::Unlink() const
{
	gi.unlinkentity( edict );
}
