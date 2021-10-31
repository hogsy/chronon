/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2020-2021 Mark E Sowden <markelswo@gmail.com>

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

namespace nox
{
	class App
	{
	public:
		App( int argc, char **argv );
		~App();

		static void Initialize();

		static void SendKeyEvents();

		unsigned int GetNumTicks();

		static char *GetClipboardData();

	private:
		static int MapKey( int key );

	public:
		static void PollEvents();

		inline const char **GetCmdLineArgs( int *num ) const
		{
			*num = argc_;
            return argv_;
		}

	private:
		int argc_;
		const char **argv_;

		unsigned int lastTick_{ 0 };
	};

	extern App *globalApp;
}// namespace nox
