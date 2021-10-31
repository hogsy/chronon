/*
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

struct image_t;

namespace nox::renderer
{
	class Pass
	{
	public:
		enum class BlendMode
		{
			BLENDMODE_NONE,
			BLENDMODE_ADD,
			BLENDMODE_NORMAL,
			BLENDMODE_MULTIPLY,
		};

		enum class CullMode
		{
			CULLMODE_DISABLE,
			CULLMODE_FRONT,
			CULLMODE_BACK,
		};

		enum class DepthFuncMode
		{
			DEPTHFUNC_NEVER,
			DEPTHFUNC_LESS,
			DEPTHFUNC_EQUAL,
			DEPTHFUNC_LEQUAL,
			DEPTHFUNC_GREATER,
			DEPTHFUNC_NOTEQUAL,
			DEPTHFUNC_GEQUAL,
			DEPTHFUNC_ALWAYS,
		};

		enum class UVGenMode
		{
			UVGENMODE_NONE,
			UVGENMODE_SPHERE,
		};

		void Draw();// todo: pass draw data

	private:
		image_t *map_{ nullptr };

		BlendMode     blendMode_{ BlendMode::BLENDMODE_NONE };
		CullMode      cullMode_{ CullMode::CULLMODE_BACK };
		DepthFuncMode depthFuncMode_{ DepthFuncMode::DEPTHFUNC_LEQUAL };
		UVGenMode     uvGenMode_{ UVGenMode::UVGENMODE_NONE };
	};

	class Profile
	{
	public:
		Profile()  = default;
		~Profile() = default;

		[[nodiscard]] inline std::string GetName() const { return name_; }

		void Draw();// todo: pass draw data

	private:
		std::string name_;

	public:
		struct Skin
		{
			std::vector< Pass > passes_;
		};

		void        AddSkin( const Skin &skin );
		const Skin &GetSkin( unsigned int index );

	private:
		std::vector< Skin > skins_;
	};
}// namespace nox::renderer

