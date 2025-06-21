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

namespace chr
{
	struct MDAPass
	{
		enum class BlendMode
		{
			NONE,
			NORMAL,
			MULTIPLY,
			ADD,
		};
		BlendMode blend{ BlendMode::NONE };

		enum class DepthFunc
		{
			NONE,
			EQUAL,
			LESS,
		};
		DepthFunc depth{ DepthFunc::NONE };
		bool      depthWrite{};

		enum class UVGen
		{
			NONE,
			SPHERE,
		};
		UVGen uvgen{ UVGen::NONE };

		enum class UVMod
		{
			NONE,
			SCROLL,
		};
		UVMod   uvMod{ UVMod::NONE };
		Vector2 uvModScroll{};

		enum class RGBGen
		{
			NONE,
			IDENTITY,
			DIFFUSE_ZERO,
			AMBIENT,
		};
		RGBGen rgb{ RGBGen::NONE };

		enum class CullMode
		{
			NONE,
			FRONT,
			BACK,
		};
		CullMode cull{ CullMode::FRONT };

		enum class AlphaFunc
		{
			NONE,
			LT128,// glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_LESS, 0.5f)
			GE128,// glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GEQUAL, 0.5f)
			GT0,  // glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.0f)
		};
		AlphaFunc alpha{ AlphaFunc::NONE };

		image_t *map{};

		bool Parse( std::stringstream &ss );
	};

	struct MDASkin
	{
		std::vector< MDAPass > passes{};

		bool Parse( std::stringstream &ss );
	};

	struct MDAProfile
	{
		std::string tag;
		std::string evaluation;

		std::vector< MDASkin > skins{};

		bool Parse( std::stringstream &ss );
	};

	class MDAModel
	{
	public:
		MDAModel();
		~MDAModel();

		void Draw( entity_t *entity );

		bool Parse( const std::string &buf );

	private:
		std::map< std::string, MDAProfile > profiles{};

		vec3_t headTriangle{};

		model_t *baseModel{};
	};
}// namespace chr
