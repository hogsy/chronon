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

#include "entity.h"

class Player : public Entity
{
	IMPLEMENT_ENTITY( Player, Entity )

public:
	explicit Player( edict_t *edict ) : Entity( edict ) {}
	~Player() override = default;

	void Spawn( const EntityManager::SpawnVariables &variables ) override;

	void OnDisconnect() const;

private:
	void UpdateWeapon();
	void UpdateView();
	void UpdateStats();

public:
	void OnEndServerFrame();

	[[nodiscard]] gclient_t      *GetClient() const { return edict->client; }
	[[nodiscard]] player_state_t *GetPlayerState() const { return &edict->client->ps; }

private:
	void SelectSpawnPoint() const;

	static constexpr vec3_t PLAYER_MINS = { -16.0f, -16.0f, -24.0f };
	static constexpr vec3_t PLAYER_MAXS = { 16.0f, 16.0f, 32.0f };

	static constexpr float VIEW_HEIGHT     = 22.0f;
	static constexpr float VIEW_MIN_HEIGHT = 5.0f;
	static constexpr float VIEW_STIFFNESS  = 0.5f;
	static constexpr float VIEW_DAMPENING  = 0.8f;

	vec3_t oldViewAngles{};

	float bobCycle{};
	float springHeight{}, springVelocity{};
};
