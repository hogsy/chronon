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

#include "../client/client.h"

namespace chr
{
	enum class ApeOpCode
	{
		OP_STARTSWITCH   = 49,
		OP_THINKSWITCH   = 50,
		OP_STARTCONSOLE  = 65,
		OP_BACKGROUND    = 68,
		OP_FONT          = 70,
		OP_SUBWINDOW     = 72,
		OP_IMAGE         = 73,
		OP_CAM           = 77,
		OP_FINISHCONSOLE = 78,
		OP_NEXTWINDOW    = 79,
		OP_TITLE         = 84,
		OP_STYLE         = 87,
	};

	class ApeInstance
	{
	public:
		ApeInstance();
		~ApeInstance();

		bool Load( const std::string &filename );

	private:
		bool Parse( const void *ptr, size_t size );
	};
}// namespace chr
