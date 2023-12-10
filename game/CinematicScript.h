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

#pragma once

namespace chr::game
{
	class CinematicScript
	{
	public:
		struct Node
		{
			unsigned int type;// almost always the same as path type
			unsigned int flags;
			float timeLength;
		};

		struct Path
		{
			unsigned int order;
			std::string name;
			unsigned int type;
			unsigned int flags;
			unsigned int timeOffset;
			unsigned int maxLength;
			std::vector< Node > nodes;

			static constexpr unsigned int NUM_FIELDS = 6;
		};

		struct Block
		{
			std::string name;
			unsigned int flags;
			std::vector< Path > paths;

			static constexpr unsigned int NUM_FIELDS = 2;
		};

		std::string name;
		unsigned int version;
		unsigned int blockCount;
		std::vector< Block > blocks;

		unsigned int cineId;//TODO: not currently sure what this does...

		bool ParseFile( const std::string &filename );

	private:
		bool ParseNodeStatement( std::string &line );
		bool ParsePathStatement( std::string &line );
		bool ParseBlockStatement( std::string &line );
		bool ParseScriptStatement( std::string &line );

		static constexpr unsigned int NUM_FIELDS = 3;
	};
}// namespace chr::game
