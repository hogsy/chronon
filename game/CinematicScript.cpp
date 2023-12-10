/*
Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

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

#include <sstream>

#include "g_local.h"

#include "CinematicScript.h"

bool chr::game::CinematicScript::ParseFile( const std::string &filename )
{
	void *buffer = nullptr;
	gi.LoadFile( filename.c_str(), &buffer );
	if ( buffer == nullptr )
	{
		Com_Printf( "Unable to load cinematic file: %s\n", filename.c_str() );
		return false;
	}

	bool status = true;

	std::string line;
	unsigned int lineNumber = 0;
	std::istringstream streamBuffer( ( const char * ) buffer );
	while ( std::getline( streamBuffer, line ) )
	{
		lineNumber++;

		if ( line.empty() || line[ 0 ] == '\r' || line[ 0 ] == '\n' || line[ 0 ] == '#' )
			continue;

		std::istringstream iss( line );
		std::string prefix;
		std::getline( iss, prefix, ':' );

		if ( prefix == "script" )
		{
			if ( !ParseScriptStatement( line ) )
			{
				Com_Printf( "Failed to parse script statement (%s:%u)\n", filename.c_str(), lineNumber );
				status = false;
				break;
			}
			continue;
		}
		else if ( prefix == "cineid" )
		{
			char *token = std::strtok( &line[ 0 ], ":" );// split line with colon delimiter

			int count = 0;
			while ( token != nullptr )
			{
				if ( count == 1 )
					cineId = std::strtoul( token, nullptr, 16 );

				count++;
				token = std::strtok( nullptr, ":" );
			}

			if ( count != 3 )// If we have not exactly two colons (three tokens)
			{
				Com_Printf( "Failed to cineid statement (%s:%u)\n", filename.c_str(), lineNumber );
				status = false;
				break;
			}

			continue;
		}
		else if ( prefix == "block" )
		{
			if ( !ParseBlockStatement( line ) )
			{
				Com_Printf( "Failed to parse block statement (%s:%u)\n", filename.c_str(), lineNumber );
				status = false;
				break;
			}
			continue;
		}
		else if ( prefix == "path" )
		{
			if ( !ParsePathStatement( line ) )
			{
				Com_Printf( "Failed to parse path statement (%s:%u)\n", filename.c_str(), lineNumber );
				status = false;
				break;
			}
			continue;
		}
		else if ( prefix == "node" )
		{
			if ( !ParseNodeStatement( line ) )
			{
				Com_Printf( "Failed to parse node statement (%s:%u)\n", filename.c_str(), lineNumber );
				status = false;
				break;
			}
			continue;
		}
		else
		{
			Com_Printf( "Unknown statement (%s) (%s:%u)\n", prefix.c_str(), filename.c_str(), lineNumber );
#if NDEBUG// For non-debug builds, we'll return false so that we don't try to parse the rest of the cinematic script
			return false;
#endif
		}
	}

#if 1
	printf( "script %s %u %u\n", name.c_str(), version, blockCount );
	for ( const auto &i : blocks )
	{
		printf( "block %s %u\n", i.name.c_str(), i.flags );
		for ( const auto &j : i.paths )
		{
			printf( "path %u %s %u %u %u %u\n",
			        j.order,
			        j.name.c_str(),
			        j.type,
			        j.flags,
			        j.timeOffset,
			        j.maxLength );
			for ( const auto &k : j.nodes )
				printf( "node %u %u %f\n", k.type, k.flags, k.timeLength );
		}
	}
#endif

	gi.FreeFile( buffer );

	return status;
}

bool chr::game::CinematicScript::ParseNodeStatement( std::string &line )
{
	char *token = std::strtok( &line[ 0 ], ":" );// split line with colon delimiter

	if ( blocks.empty() )
		return false;

	Block *block = &blocks.back();

	int count = 0;
	while ( token != nullptr )
	{
		switch ( count )
		{
			case 1:
				block->paths.emplace_back();
				block->paths.back().name = token;
				break;
			case 2:
				blocks.back().flags = std::strtoul( token, nullptr, 16 );
				break;
			default:
				break;
		}
		count++;
		token = std::strtok( nullptr, ":" );
	}

	if ( count != 3 )// If we have not exactly two colons (three tokens)
		return false;

	return true;
}

bool chr::game::CinematicScript::ParsePathStatement( std::string &line )
{
	if ( blocks.empty() )
		return false;

	Path *path = &blocks.back().paths.emplace_back();

	char *token = std::strtok( &line[ 0 ], ":" );
	int count = 0;
	while ( token != nullptr )
	{
		switch ( count )
		{
			case 1:
				path->order = std::strtoul( token, nullptr, 16 );
				break;
			case 2:
				path->name = token;
				break;
			case 3:
				path->type = std::strtoul( token, nullptr, 16 );
				break;
			case 4:
				path->flags = std::strtoul( token, nullptr, 16 );
				break;
			case 5:
				path->timeOffset = std::stoi( token );
				break;
			default:
				break;
		}
		count++;
		token = std::strtok( nullptr, ":" );
	}

	if ( count != Path::NUM_FIELDS )// If we have not exactly two colons (three tokens)
		return false;

	return true;
}

bool chr::game::CinematicScript::ParseBlockStatement( std::string &line )
{
	blocks.emplace_back();
	Block *block = &blocks.back();

	char *token = std::strtok( &line[ 0 ], ":" );// split line with colon delimiter
	int count = 0;
	while ( token != nullptr )
	{
		switch ( count )
		{
			case 1:
				block->name = token;
				break;
			case 2:
				block->flags = std::strtoul( token, nullptr, 16 );
				break;
			default:
				break;
		}
		count++;
		token = std::strtok( nullptr, ":" );
	}

	if ( count != Block::NUM_FIELDS )// If we have not exactly two colons (three tokens)
		return false;

	return true;
}

bool chr::game::CinematicScript::ParseScriptStatement( std::string &line )
{
	char *token = std::strtok( &line[ 0 ], ":" );// Split line with colon delimiter
	int count = 0;
	while ( token != nullptr )
	{
		switch ( count )
		{
			case 1:
				name = token;
				break;
			case 2:
				version = std::stoi( token );
				break;
			case 3:
				blockCount = std::stoi( token );
				break;
			default:
				break;
		}
		count++;
		token = std::strtok( nullptr, ":" );
	}

	if ( count != NUM_FIELDS )// If we have not exactly three colons (four tokens)
		return false;

	return true;
}
