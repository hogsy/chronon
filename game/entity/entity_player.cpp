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

#include "entity_player.h"

void Player::Spawn( const EntityManager::SpawnVariables &variables )
{
	// a little strange for this one, but the spawn variables will always be empty for a player

	assert( edict->client != nullptr );

	edict->groundentity = nullptr;
	edict->takedamage   = DAMAGE_AIM;
	edict->movetype     = MOVETYPE_WALK;
	edict->viewheight   = VIEW_HEIGHT;
	edict->inuse        = true;
	edict->classname    = "player";
	edict->mass         = 200;
	edict->solid        = SOLID_BBOX;
	edict->deadflag     = DEAD_NO;
	edict->air_finished = level.time + 12;
	edict->clipmask     = MASK_PLAYERSOLID;
	edict->model        = "players/male/tris.md2";
	edict->pain         = player_pain;
	edict->die          = player_die;
	edict->waterlevel   = 0;
	edict->watertype    = 0;
	edict->flags &= ~FL_NO_KNOCKBACK;
	edict->svflags &= ~SVF_DEADMONSTER;
	edict->svflags &= ~SVF_NOCLIENT;
	edict->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

	SetSize( PLAYER_MINS, PLAYER_MAXS );

	SelectSpawnPoint();

	VectorClear( edict->velocity );

	springHeight = VIEW_HEIGHT;
}

void Player::OnDisconnect() const
{
	gclient_t *client = GetClient();
	if ( client == nullptr )
	{
		return;
	}

	//safe_bprintf( PRINT_HIGH, "%s disconnected\n", client->pers.netname );

	// send effect
	gi.WriteByte( svc_muzzleflash );
	gi.WriteShort( edict - g_edicts );
	gi.WriteByte( MZ_LOGOUT );
	gi.multicast( edict->s.origin, MULTICAST_PVS );

	SetSolid( SOLID_NOT );

	edict->s.modelindex           = 0;
	edict->inuse                  = false;
	edict->classname              = "disconnected";
	edict->client->pers.connected = false;

	Unlink();
}

void Player::UpdateWeapon()
{
	player_state_t *ps = GetPlayerState();

	// gun angles from delta movement
#if 0
	for ( unsigned int i = 0; i < 3; ++i )
	{
		float delta = oldViewAngles[ i ] - ps->viewangles[ i ];
		if ( delta > 180.0f )
		{
			delta -= 360.0f;
		}
		else if ( delta < -180.0f )
		{
			delta += 360.0f;
		}

		if ( delta > 45.0f )
		{
			delta = 45.0f;
		}
		else if ( delta < -45.0f )
		{
			delta = -45.0f;
		}

		if ( i == YAW )
		{
			ps->gunangles[ ROLL ] += 0.1f * delta;
		}

		ps->gunangles[ i ] += 0.2f * delta;
	}
#endif

	gclient_t *client = GetClient();
	assert( client != nullptr );

	vec3_t forward, right, up;
	AngleVectors( client->v_angle, forward, right, up );

	static constexpr float BASE_X_OFFSET = 5.0f;
	static constexpr float BASE_Y_OFFSET = 2.0f;
	static constexpr float BASE_Z_OFFSET = -10.0f;

	// gun height
	VectorClear( ps->gunoffset );
	for ( unsigned int i = 0; i < 3; ++i )
	{
		ps->gunoffset[ i ] += right[ i ] * ( BASE_X_OFFSET + gun_x->value );
		ps->gunoffset[ i ] += forward[ i ] * ( BASE_Y_OFFSET + gun_y->value );
		ps->gunoffset[ i ] += up[ i ] * ( BASE_Z_OFFSET + -gun_z->value );
	}
}

void Player::UpdateView()
{
	gclient_t *client = GetClient();
	assert( client != nullptr );

	player_state_t *ps = GetPlayerState();
	assert( ps != nullptr );

	VectorCopy( client->ps.viewangles, oldViewAngles );

	// add angles based on weapon kick
	VectorCopy( client->kick_angles, ps->kick_angles );

	// handle the view offset
	VectorClear( ps->viewoffset );
	springVelocity += VIEW_STIFFNESS * ( edict->viewheight - springHeight ) - VIEW_DAMPENING * springVelocity;
	springHeight += springVelocity;

	bobCycle += std::sin( level.time * 0.5f ) * 0.05f;
	ps->viewoffset[ 2 ] += springHeight + bobCycle;
	if ( ps->viewoffset[ 2 ] < VIEW_MIN_HEIGHT )
	{
		ps->viewoffset[ 2 ] = VIEW_MIN_HEIGHT;
	}

	// clear weapon kicks
	VectorClear( client->kick_origin );
	VectorClear( client->kick_angles );
}

void Player::UpdateStats()
{
	player_state_t *ps = GetPlayerState();
	assert( ps != nullptr );

	ps->stats[ STAT_HEALTH_ICON ] = level.pic_health;
	ps->stats[ STAT_HEALTH ]      = edict->health;
}

void Player::OnEndServerFrame()
{
	gclient_t *client = GetClient();
	assert( client != nullptr );

	player_state_t *ps = GetPlayerState();
	assert( ps != nullptr );

	UpdateStats();

	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	//
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	for ( unsigned int i = 0; i < 3; ++i )
	{
		ps->pmove.origin[ i ] = edict->s.origin[ i ] * 8.0f;
	}

	UpdateView();
	UpdateWeapon();

	// set model angles from view angles so other things in
	// the world can tell which direction you are looking

	edict->s.angles[ PITCH ] = client->v_angle[ PITCH ] > 180.0f ? ( -360.0f + client->v_angle[ PITCH ] ) / 3.0f : client->v_angle[ PITCH ] / 3.0f;
	edict->s.angles[ YAW ]   = client->v_angle[ YAW ];
	edict->s.angles[ ROLL ]  = 0.0f;

	VectorCopy( edict->velocity, client->oldvelocity );
}

void Player::SelectSpawnPoint() const
{
	edict_t *spot = G_Find( nullptr, FOFS( classname ), "info_player_start" );
	if ( spot == nullptr )
	{
		gi.dprintf( "Failed to find spawn point for player!\n" );
		return;
	}

	if ( spot->health > 0 )
	{
		edict->health = spot->health;
	}

	SetOrigin( { spot->s.origin[ 0 ], spot->s.origin[ 1 ], spot->s.origin[ 2 ] + 1.0f } );
	SetAngles( spot->s.angles );

	if ( deathmatch->value == 0.0f && coop->value == 0.0f )
	{
		spot->count--;
		if ( spot->count <= 0 )
		{
			spot->think     = G_FreeEdict;
			spot->nextthink = level.time + 1.0f;
		}
	}
}

REGISTER_ENTITY_CLASS( player, Player )
