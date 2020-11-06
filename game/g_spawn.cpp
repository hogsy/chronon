/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>

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

#include <string>
#include <map>

#include "g_local.h"

// As provided by entity.dat
struct EntityCustomClassDeclaration {
	char className[ 64 ];
	char modelPath[ MAX_QPATH ];

	vec3_t scale;

	char entityType[ 64 ];

	vec3_t bbMins;
	vec3_t bbMaxs;

	bool showShadow;

	unsigned int solidFlag;

	float walkSpeed;
	float runSpeed;
	float speed;

	unsigned int lighting, blending;
	char targetSequence[ 64 ];
	int miscValue;
	bool noMip;
	char spawnSequence[ 64 ];
	char description[ 64 ];
};
static std::map< std::string, EntityCustomClassDeclaration > entityCustomClasses;

static void ProtoSpawner( edict_t *self, const EntityCustomClassDeclaration &spawnData ) {
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex( spawnData.modelPath );

	VectorCopy( spawnData.scale, self->size );

	VectorCopy( spawnData.bbMins, self->mins );
	VectorCopy( spawnData.bbMaxs, self->maxs );

	gi.linkentity( self );
}

typedef void( *EntityCustomClassSpawnFunction )( edict_t *self, const EntityCustomClassDeclaration &spawnData );
static std::map< std::string, EntityCustomClassSpawnFunction > entityTypes = {
	{ "char", ProtoSpawner },
	{ "charfly", ProtoSpawner },
	{ "charhover", ProtoSpawner },
	{ "charroll", ProtoSpawner },
	{ "effect", ProtoSpawner },
	{ "pickup", ProtoSpawner },
	{ "container", ProtoSpawner },
	{ "keyitem", ProtoSpawner },
	{ "scavenger", ProtoSpawner },
	{ "trashpawn", ProtoSpawner },
	{ "bugspawn", ProtoSpawner },
	{ "general", ProtoSpawner },
	{ "bipidri", ProtoSpawner },
	{ "sprite", ProtoSpawner },
	{ "playerchar", ProtoSpawner },
	{ "noclip", ProtoSpawner },
	{ "floater", ProtoSpawner },
};

static void Spawn_ParseCustomClass( const char *lineDef, size_t lineLength ) {
	EntityCustomClassDeclaration customClass;
	memset( &customClass, 0, sizeof( EntityCustomClassDeclaration ) );

	unsigned int i;
	for( i = 0; i < 24; ++i ) {
		const char *token = Script_Parse( &lineDef, "|\n" );
		if( token == nullptr ) {
			break;
		}

		// This is fucking awful, but I can't think of a better way right now
		switch( i ) {
		case 0: // classname
			snprintf( customClass.className, sizeof( customClass.className ), "%s", token );
			break;
		case 1: // model_path
			snprintf( customClass.modelPath, sizeof( customClass.modelPath ), "%s", token );
			break;

		case 2: // scale_x
			customClass.scale[ 0 ] = strtof( token, nullptr );
			break;
		case 3: // scale_y
			customClass.scale[ 1 ] = strtof( token, nullptr );
			break;
		case 4: // scale_z
			customClass.scale[ 2 ] = strtof( token, nullptr );
			break;

		case 5: // entity_type
			snprintf( customClass.entityType, sizeof( customClass.entityType ), "%s", token );
			break;

		case 6: // box_xmin
			customClass.bbMins[ 0 ] = strtof( token, nullptr );
			break;
		case 7: // box_ymin
			customClass.bbMins[ 1 ] = strtof( token, nullptr );
			break;
		case 8: // box_zmin
			customClass.bbMins[ 2 ] = strtof( token, nullptr );
			break;

		case 9: // box_xmax
			customClass.bbMaxs[ 0 ] = strtof( token, nullptr );
			break;
		case 10: // box_ymax
			customClass.bbMaxs[ 1 ] = strtof( token, nullptr );
			break;
		case 11: // box_zmax
			customClass.bbMaxs[ 2 ] = strtof( token, nullptr );
			break;

		case 12: // noshadow
			customClass.showShadow = ( Q_strcasecmp( token, "shadow" ) == 0 );
			break;

		default:
			gi.dprintf( "Skipping field %s\n", token );
			break;
		}
	}

	if( customClass.className == '\0' ) {
		gi.error( "Invalid classname for custom entity class in \"models/entity.dat\"!\n" );
	}

	entityCustomClasses.insert( std::make_pair( customClass.className, customClass ) );
}

static void Spawn_PopulateCustomClassList( void ) {
	char *fileBuffer;
	gi.LoadFile( "models/entity.dat", (void **)&fileBuffer );
	if( fileBuffer == nullptr ) {
		gi.error( "Failed to find \"models/entity.dat\"!\n" );
	}

	const char *p = fileBuffer;
	while( true ) {
		p = Script_SkipWhitespace( p );
		if( p == nullptr ) {
			break;
		}

		const char *oldPos = p;
		p = Script_SkipComment( p );
		if( p != oldPos ) {
			continue;
		}

		// Read in the line
		size_t lLength = Script_GetLineLength( p ) + 1;
		char *lBuf = (char *)gi.TagMalloc( lLength, TAG_GAME );

		p = Script_GetLine( p, lBuf, lLength );
		if( *lBuf == ';' || *lBuf == '\0' ) {
			gi.TagFree( lBuf );
			break;
		}

		Spawn_ParseCustomClass( lBuf, lLength );

		gi.TagFree( lBuf );
	}

	gi.FreeFile( fileBuffer );
}

void SP_info_player_start( edict_t *ent );
void SP_func_plat( edict_t *ent );
void SP_func_rotating( edict_t *ent );
void SP_func_button( edict_t *ent );
void SP_func_door( edict_t *ent );
void SP_func_door_secret( edict_t *ent );
void SP_func_door_rotating( edict_t *ent );
void SP_func_water( edict_t *ent );
void SP_func_train( edict_t *ent );
void SP_func_conveyor( edict_t *self );
void SP_func_wall( edict_t *self );
void SP_func_object( edict_t *self );
void SP_func_explosive( edict_t *self );
void SP_func_timer( edict_t *self );
void SP_func_areaportal( edict_t *ent );
void SP_func_clock( edict_t *ent );
void SP_func_killbox( edict_t *ent );
void SP_trigger_always( edict_t *ent );
void SP_trigger_once( edict_t *ent );
void SP_trigger_multiple( edict_t *ent );
void SP_trigger_relay( edict_t *ent );
void SP_trigger_push( edict_t *ent );
void SP_trigger_hurt( edict_t *ent );
void SP_trigger_key( edict_t *ent );
void SP_trigger_counter( edict_t *ent );
void SP_trigger_elevator( edict_t *ent );
void SP_trigger_gravity( edict_t *ent );
void SP_target_temp_entity( edict_t *ent );
void SP_target_speaker( edict_t *ent );
void SP_target_lightramp( edict_t *self );
void SP_worldspawn( edict_t *ent );
void SP_viewthing( edict_t *ent );
void SP_light( edict_t *self );
void SP_info_null( edict_t *self );
void SP_info_notnull( edict_t *self );
void SP_path_corner( edict_t *self );

void SP_monster_berserk( edict_t *self );

typedef void( *EntitySpawnFunction )( edict_t *self );
std::map< std::string, EntitySpawnFunction > entitySpawnClasses = {
	{"info_player_start", SP_info_player_start},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_door_rotating", SP_func_door_rotating},
	{"func_rotating", SP_func_rotating},
	{"func_train", SP_func_train},
	{"func_water", SP_func_water},
	{"func_conveyor", SP_func_conveyor},
	{"func_areaportal", SP_func_areaportal},
	{"func_clock", SP_func_clock}, // Not actually in Anox, but could be useful for custom maps?
	{"func_wall", SP_func_wall},
	// todo: func_fog
	// todo: func_particle
	{"func_object", SP_func_object}, // Not actually in Anox, but could be useful for custom maps?
	{"func_timer", SP_func_timer}, // Not actually in Anox, but could be useful for custom maps?
	{"func_explosive", SP_func_explosive}, // Not actually in Anox, but could be useful for custom maps?
	{"func_killbox", SP_func_killbox}, // Not actually in Anox, but could be useful for custom maps?

	{"trigger_always", SP_trigger_always},
	{"trigger_once", SP_trigger_once},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_relay", SP_trigger_relay},
	{"trigger_push", SP_trigger_push},
	{"trigger_hurt", SP_trigger_hurt}, // Not actually in Anox, but could be useful for custom maps?
	{"trigger_key", SP_trigger_key}, // Not actually in Anox, but could be useful for custom maps?
	{"trigger_counter", SP_trigger_counter},
	{"trigger_elevator", SP_trigger_elevator},
	{"trigger_gravity", SP_trigger_gravity},
	// todo: trigger_watercurrent

	{"target_temp_entity", SP_target_temp_entity}, // Not actually in Anox, but could be useful for custom maps?
	{"target_speaker", SP_target_speaker},
	{"target_lightramp", SP_target_lightramp},

	{"worldspawn", SP_worldspawn},
	{"viewthing", SP_viewthing},

	{"light", SP_light},
	// todo: sun
	// todo: spew (wut?)
	// todo: modellight
	// todo: dirlightsource

	{"info_null", SP_info_null},
	{"func_group", SP_info_null},
	{"info_notnull", SP_info_notnull},
	{"path_corner", SP_path_corner},
	// todo: beam_target
	// todo: path_grid_center

	// todo: effect_sprite

	// todo: trigger_changelevel
	// todo: trigger_battle
	// todo: trigger_console
	// todo: trigger_console_once
	// todo: trigger_music

	// todo: info_battle_posp
	// todo: info_battle_pose
	// todo: info_battle_cam
	// todo: info_battle_node
	// todo: info_battle_manager
	// todo: info_trash_generator
	// todo: info_bug_generator

	// todo: ob_sewagecrates
	// todo: ob_sewageexplode

	// todo: func_ridebox

	// todo: info_party_start

	// tmp
	{ "npc_alien_rowdy", SP_monster_berserk },
};

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void ED_CallSpawn( edict_t *ent ) {
	if( !ent->classname ) {
		gi.dprintf( "ED_CallSpawn: NULL classname\n" );
		return;
	}

	// check item spawn functions
	gitem_t *item;
	int i;
	for( i = 0, item = itemlist; i < game.num_items; i++, item++ ) {
		if( !item->classname )
			continue;

		if( !strcmp( item->classname, ent->classname ) ) {	// found it
			SpawnItem( ent, item );
			return;
		}
	}

	// Check custom spawn function
	auto customClass = entityCustomClasses.find( ent->classname );
	if( customClass != entityCustomClasses.end() ) {
		auto type = entityTypes.find( customClass->second.entityType );
		if( type == entityTypes.end() ) {
			gi.dprintf( "%s doesn't have a valid entity type (%s)!\n", ent->classname, customClass->second.entityType );
			return;
		}

		type->second( ent, customClass->second );
		return;
	}

	// check normal spawn functions
	auto spawnClass = entitySpawnClasses.find( ent->classname );
	if( spawnClass != entitySpawnClasses.end() ) {
		spawnClass->second( ent );
		return;
	}

	gi.dprintf( "%s doesn't have a spawn function\n", ent->classname );
}

/*
=============
ED_NewString
=============
*/
char *ED_NewString( const char *string ) {
	char *newb, *new_p;
	int		i, l;

	l = strlen( string ) + 1;

	newb = static_cast<char *>( gi.TagMalloc( l, TAG_LEVEL ) );

	new_p = newb;

	for( i = 0; i < l; i++ ) {
		if( string[ i ] == '\\' && i < l - 1 ) {
			i++;
			if( string[ i ] == 'n' )
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		} else
			*new_p++ = string[ i ];
	}

	return newb;
}

/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
void ED_ParseField( const char *key, const char *value, edict_t *ent ) {
	field_t *f;
	byte *b;
	float	v;
	vec3_t	vec;

	for( f = fields; f->name; f++ ) {
		if( !( f->flags & FFL_NOSPAWN ) && !Q_stricmp( f->name, key ) ) {	// found it
			if( f->flags & FFL_SPAWNTEMP )
				b = (byte *)&st;
			else
				b = (byte *)ent;

			switch( f->type ) {
			case F_LSTRING:
				*(char **)( b + f->ofs ) = ED_NewString( value );
				break;
			case F_VECTOR:
				sscanf( value, "%f %f %f", &vec[ 0 ], &vec[ 1 ], &vec[ 2 ] );
				( (float *)( b + f->ofs ) )[ 0 ] = vec[ 0 ];
				( (float *)( b + f->ofs ) )[ 1 ] = vec[ 1 ];
				( (float *)( b + f->ofs ) )[ 2 ] = vec[ 2 ];
				break;
			case F_INT:
				*(int *)( b + f->ofs ) = static_cast<int>( strtol( value, nullptr, 10 ) );
				break;
			case F_FLOAT:
				*(float *)( b + f->ofs ) = strtof( value, nullptr );
				break;
			case F_ANGLEHACK:
				v = strtof( value, nullptr );
				( (float *)( b + f->ofs ) )[ 0 ] = 0;
				( (float *)( b + f->ofs ) )[ 1 ] = v;
				( (float *)( b + f->ofs ) )[ 2 ] = 0;
				break;
			default:
			case F_IGNORE:
				break;
			}
			return;
		}
	}
	gi.dprintf( "%s is not a field\n", key );
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
static const char *ED_ParseEdict( const char *data, edict_t *ent ) {
	qboolean	init;
	char		keyname[ 256 ];
	const char *com_token;

	init = false;
	memset( &st, 0, sizeof( st ) );

	// go through all the dictionary pairs
	while( 1 ) {
		// parse key
		com_token = COM_Parse( &data );
		if( com_token[ 0 ] == '}' )
			break;
		if( !data )
			gi.error( "ED_ParseEntity: EOF without closing brace" );

		strncpy( keyname, com_token, sizeof( keyname ) - 1 );

		// parse value	
		com_token = COM_Parse( &data );
		if( !data )
			gi.error( "ED_ParseEntity: EOF without closing brace" );

		if( com_token[ 0 ] == '}' )
			gi.error( "ED_ParseEntity: closing brace without data" );

		init = true;

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if( keyname[ 0 ] == '_' )
			continue;

		ED_ParseField( keyname, com_token, ent );
	}

	if( !init )
		memset( ent, 0, sizeof( *ent ) );

	return data;
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams() {
	edict_t *e, *e2, *chain;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for( i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++ ) {
		if( !e->inuse )
			continue;
		if( !e->team )
			continue;
		if( e->flags & FL_TEAMSLAVE )
			continue;
		chain = e;
		e->teammaster = e;
		c++;
		c2++;
		for( j = i + 1, e2 = e + 1; j < globals.num_edicts; j++, e2++ ) {
			if( !e2->inuse )
				continue;
			if( !e2->team )
				continue;
			if( e2->flags & FL_TEAMSLAVE )
				continue;
			if( !strcmp( e->team, e2->team ) ) {
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	gi.dprintf( "%i teams with %i entities\n", c, c2 );
}

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities( char *mapname, const char *entities, char *spawnpoint ) {
	edict_t *ent;
	int			inhibit;
	const char *com_token;
	int			i;
	float		skill_level;

	skill_level = floorf( skill->value );
	if( skill_level < 0 )
		skill_level = 0;
	if( skill_level > 3 )
		skill_level = 3;
	if( skill->value != skill_level )
		gi.cvar_forceset( "skill", va( "%f", skill_level ) );

	SaveClientData();

	gi.FreeTags( TAG_LEVEL );

	memset( &level, 0, sizeof( level ) );
	memset( g_edicts, 0, game.maxentities * sizeof( g_edicts[ 0 ] ) );

	strncpy( level.mapname, mapname, sizeof( level.mapname ) - 1 );
	strncpy( game.spawnpoint, spawnpoint, sizeof( game.spawnpoint ) - 1 );

	// set client fields on player ents
	for( i = 0; i < game.maxclients; i++ )
		g_edicts[ i + 1 ].client = game.clients + i;

	ent = nullptr;
	inhibit = 0;

	Spawn_PopulateCustomClassList();

	// parse ents
	while( 1 ) {
		// parse the opening brace	
		com_token = COM_Parse( &entities );
		if( !entities )
			break;
		if( com_token[ 0 ] != '{' )
			gi.error( "ED_LoadFromFile: found %s when expecting {", com_token );

		if( !ent )
			ent = g_edicts;
		else
			ent = G_Spawn();
		entities = ED_ParseEdict( entities, ent );

		// yet another map hack
		if( !Q_stricmp( level.mapname, "command" ) && !Q_stricmp( ent->classname, "trigger_once" ) && !Q_stricmp( ent->model, "*27" ) )
			ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;

		// remove things (except the world) from different skill levels or deathmatch
		if( ent != g_edicts ) {
			if( deathmatch->value ) {
				if( ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH ) {
					G_FreeEdict( ent );
					inhibit++;
					continue;
				}
			} else {
				if( /* ((coop->value) && (ent->spawnflags & SPAWNFLAG_NOT_COOP)) || */
					( ( skill->value == 0 ) && ( ent->spawnflags & SPAWNFLAG_NOT_EASY ) ) ||
					( ( skill->value == 1 ) && ( ent->spawnflags & SPAWNFLAG_NOT_MEDIUM ) ) ||
					( ( ( skill->value == 2 ) || ( skill->value == 3 ) ) && ( ent->spawnflags & SPAWNFLAG_NOT_HARD ) )
					) {
					G_FreeEdict( ent );
					inhibit++;
					continue;
				}
			}

			ent->spawnflags &= ~( SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD | SPAWNFLAG_NOT_COOP | SPAWNFLAG_NOT_DEATHMATCH );
		}

		ED_CallSpawn( ent );
	}

	gi.dprintf( "%i entities inhibited\n", inhibit );

#ifdef DEBUG
	i = 1;
	ent = EDICT_NUM( i );
	while( i < globals.num_edicts ) {
		if( ent->inuse != 0 || ent->inuse != 1 )
			Com_DPrintf( "Invalid entity %d\n", i );
		i++, ent++;
	}
#endif

	G_FindTeams();

	PlayerTrail_Init();
}


//===================================================================

const char *single_statusbar =
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "

// selected item
"if 6 "
"	xv	296 "
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"	yb	-50 "
"endif "

// timer
"if 9 "
"	xv	262 "
"	num	2	10 "
"	xv	296 "
"	pic	9 "
"endif "

//  help / weapon icon 
"if 11 "
"	xv	148 "
"	pic	11 "
"endif "
;

const char *dm_statusbar =
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "

// selected item
"if 6 "
"	xv	296 "
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"	yb	-50 "
"endif "

// timer
"if 9 "
"	xv	246 "
"	num	2	10 "
"	xv	296 "
"	pic	9 "
"endif "

//  help / weapon icon 
"if 11 "
"	xv	148 "
"	pic	11 "
"endif "

//  frags
"xr	-50 "
"yt 2 "
"num 3 14 "

// spectator
"if 17 "
"xv 0 "
"yb -58 "
"string2 \"SPECTATOR MODE\" "
"endif "

// chase camera
"if 16 "
"xv 0 "
"yb -68 "
"string \"Chasing\" "
"xv 64 "
"stat_string 16 "
"endif "
;


/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"	environment map name
"skyaxis"	vector axis for rotating sky
"skyrotate"	speed of rotation in degrees/second
"sounds"	music cd track number
"gravity"	800 is default gravity
"message"	text to print at user logon
*/
void SP_worldspawn( edict_t *ent ) {
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->inuse = true;			// since the world doesn't use G_Spawn()
	ent->s.modelindex = 1;		// world model is always index 1

	//---------------

	// reserve some spots for dead player bodies for coop / deathmatch
	InitBodyQue();

	// set configstrings for items
	SetItemNames();

	if( st.nextmap )
		strcpy( level.nextmap, st.nextmap );

	// make some data visible to the server

	if( ent->message && ent->message[ 0 ] ) {
		gi.configstring( CS_NAME, ent->message );
		strncpy( level.level_name, ent->message, sizeof( level.level_name ) );
	} else
		strncpy( level.level_name, level.mapname, sizeof( level.level_name ) );

	if( st.sky && st.sky[ 0 ] )
		gi.configstring( CS_SKY, st.sky );
	else
		gi.configstring( CS_SKY, "unit1_" );

	gi.configstring( CS_SKYROTATE, va( "%f", st.skyrotate ) );

	gi.configstring( CS_SKYAXIS, va( "%f %f %f",
		st.skyaxis[ 0 ], st.skyaxis[ 1 ], st.skyaxis[ 2 ] ) );

	gi.configstring( CS_CDTRACK, va( "%i", ent->sounds ) );

	gi.configstring( CS_MAXCLIENTS, va( "%i", (int)( maxclients->value ) ) );

#if 0
	// status bar program
	if( deathmatch->value )
		gi.configstring( CS_STATUSBAR, dm_statusbar );
	else
		gi.configstring( CS_STATUSBAR, single_statusbar );
#endif

	//---------------

	// help icon for statusbar
	gi.imageindex( "i_help" );
	level.pic_health = gi.imageindex( "i_health" );
	gi.imageindex( "help" );
	gi.imageindex( "field_3" );

	if( !st.gravity )
		gi.cvar_set( "sv_gravity", "800" );
	else
		gi.cvar_set( "sv_gravity", st.gravity );

	snd_fry = gi.soundindex( "player/fry.wav" );	// standing in lava / slime

	PrecacheItem( FindItem( "Blaster" ) );

	gi.soundindex( "player/lava1.wav" );
	gi.soundindex( "player/lava2.wav" );

	gi.soundindex( "misc/pc_up.wav" );
	gi.soundindex( "misc/talk1.wav" );

	gi.soundindex( "misc/udeath.wav" );

	// gibs
	gi.soundindex( "items/respawn1.wav" );

	// sexed sounds
	gi.soundindex( "*death1.wav" );
	gi.soundindex( "*death2.wav" );
	gi.soundindex( "*death3.wav" );
	gi.soundindex( "*death4.wav" );
	gi.soundindex( "*fall1.wav" );
	gi.soundindex( "*fall2.wav" );
	gi.soundindex( "*gurp1.wav" );		// drowning damage
	gi.soundindex( "*gurp2.wav" );
	gi.soundindex( "*jump1.wav" );		// player jump
	gi.soundindex( "*pain25_1.wav" );
	gi.soundindex( "*pain25_2.wav" );
	gi.soundindex( "*pain50_1.wav" );
	gi.soundindex( "*pain50_2.wav" );
	gi.soundindex( "*pain75_1.wav" );
	gi.soundindex( "*pain75_2.wav" );
	gi.soundindex( "*pain100_1.wav" );
	gi.soundindex( "*pain100_2.wav" );

	//-------------------

	gi.soundindex( "player/gasp1.wav" );		// gasping for air
	gi.soundindex( "player/gasp2.wav" );		// head breaking surface, not gasping

	gi.soundindex( "player/watr_in.wav" );	// feet hitting water
	gi.soundindex( "player/watr_out.wav" );	// feet leaving water

	gi.soundindex( "player/watr_un.wav" );	// head going underwater

	gi.soundindex( "player/u_breath1.wav" );
	gi.soundindex( "player/u_breath2.wav" );

	gi.soundindex( "items/pkup.wav" );		// bonus item pickup
	gi.soundindex( "world/land.wav" );		// landing thud
	gi.soundindex( "misc/h2ohit1.wav" );		// landing splash

	gi.soundindex( "items/damage.wav" );
	gi.soundindex( "items/protect.wav" );
	gi.soundindex( "items/protect4.wav" );
	gi.soundindex( "weapons/noammo.wav" );

	gi.soundindex( "infantry/inflies1.wav" );

	//
	// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
	//

		// 0 normal
	gi.configstring( CS_LIGHTS + 0, "m" );

	// 1 FLICKER (first variety)
	gi.configstring( CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo" );

	// 2 SLOW STRONG PULSE
	gi.configstring( CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba" );

	// 3 CANDLE (first variety)
	gi.configstring( CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg" );

	// 4 FAST STROBE
	gi.configstring( CS_LIGHTS + 4, "mamamamamama" );

	// 5 GENTLE PULSE 1
	gi.configstring( CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj" );

	// 6 FLICKER (second variety)
	gi.configstring( CS_LIGHTS + 6, "nmonqnmomnmomomno" );

	// 7 CANDLE (second variety)
	gi.configstring( CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm" );

	// 8 CANDLE (third variety)
	gi.configstring( CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa" );

	// 9 SLOW STROBE (fourth variety)
	gi.configstring( CS_LIGHTS + 9, "aaaaaaaazzzzzzzz" );

	// 10 FLUORESCENT FLICKER
	gi.configstring( CS_LIGHTS + 10, "mmamammmmammamamaaamammma" );

	// 11 SLOW PULSE NOT FADE TO BLACK
	gi.configstring( CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba" );

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	gi.configstring( CS_LIGHTS + 63, "a" );
}

