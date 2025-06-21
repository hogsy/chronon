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
	struct MDAProfile;
	class AliasModel
	{
	public:
		AliasModel();
		~AliasModel();

		struct Skin
		{
			std::string name;
			uint16_t    numPrimitives;
		};

		bool LoadFromBuffer( const void *buffer );

		inline const std::vector< Skin > &GetSkins() const
		{
			return skins;
		}

		inline int GetNumFrames() const
		{
			return numFrames;
		}

	private:
		void LoadSkins( const dmdl_t *mdl, int numSkins );
		void LoadTriangles( const dmdl_t *mdl );
		void LoadTaggedTriangles( const dmdl_t *mdl );
		void LoadCommands( const dmdl_t *mdl );
		void LoadFrames( const dmdl_t *mdl, int resolution );
		void LoadPrimitives( const dmdl_t *mdl );

		struct VertexGroup
		{
			unsigned int vertex[ 3 ];
			unsigned int normalIndex;
		};

		void LerpVertices( const VertexGroup *v, const VertexGroup *ov, const VertexGroup *verts, Vector3 *lerp, const Vector3 &move, const Vector3 &frontv, const Vector3 &backv ) const;
		void ApplyLighting( const entity_t *e );
		void DrawFrameLerp( entity_t *e, const MDAProfile *profile );
		int *DrawPrimitive( const Skin *skin, float alpha, const VertexGroup *v, int *order );

	public:
		void Draw( entity_t *e, const MDAProfile *profile );

	private:
		bool Cull( vec3_t bbox[ 8 ], entity_t *e );

		struct Triangle
		{
			unsigned int vertexIndices[ 3 ]{};
			unsigned int stIndices[ 3 ]{};
		};

		struct Frame
		{
			std::string                name;
			Vector3                    scale{};
			Vector3                    translate{};
			std::vector< VertexGroup > vertices;
			Vector3                    bounds[ 2 ];
		};

		int numGLCmds{};

		int numVertices{};
		int numTriangles{};
		int numFrames{};

		std::vector< Skin > skins;

		Vector3 shadeVector{};
		float   shadeLight[ 3 ]{};
		float  *shadeDots{};

		std::vector< Triangle >       triangles;
		std::map< std::string, uint > taggedTriangles;

		std::vector< Vector2 > stCoords;
		std::vector< int >     glCmds;
		std::vector< Vector3 > lerpedVertices;

		std::vector< Frame > frames;
	};
}// namespace chr
