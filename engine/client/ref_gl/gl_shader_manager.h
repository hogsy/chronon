/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>

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
	class GLShaderProgram
	{
	public:
		GLShaderProgram();

		bool LoadShaderStage( const std::string &path );

	protected:
	private:
		std::string path_;
	};

	class GLShaderManager
	{
	public:
	protected:
	private:
		std::map< std::string, GLShaderProgram > programs_;
	};
}// namespace nox
