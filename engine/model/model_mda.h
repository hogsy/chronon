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
	class MDAModel
	{
	public:
		MDAModel();
		~MDAModel();

		void Draw( entity_t *entity );

		bool Parse( const std::string &buf );

	private:
		struct Pass
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
			UVMod uvmod{ UVMod::NONE };

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
		};

		struct Skin
		{
			std::vector< Pass > passes;
		};

		struct Profile
		{
			std::string tag;
			std::string evaluation;

			std::vector< Skin > skins;
		};

		static bool ParseProfile( Profile &profile, std::stringstream &ss );
		static bool ParseSkin( Skin &skin, std::stringstream &ss );
		static bool ParsePass( Pass &pass, std::stringstream &ss );

		std::map< std::string, Profile > profiles;

		vec3_t headTriangle{};

		model_t *baseModel{};
	};
}// namespace chr
