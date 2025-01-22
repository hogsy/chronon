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
	class AliasModel
	{
	public:
		AliasModel();
		~AliasModel();

		bool LoadFromBuffer( const void *buffer );

		inline const std::vector< std::string > &GetSkins() const
		{
			return skinNames_;
		}

		inline int GetNumFrames() const
		{
			return numFrames_;
		}

	private:
		void LoadSkins( const dmdl_t *mdl, int numSkins );
		void LoadTriangles( const dmdl_t *mdl );
		void LoadTaggedTriangles( const dmdl_t *mdl );
		void LoadCommands( const dmdl_t *mdl );
		void LoadFrames( const dmdl_t *mdl, int resolution );

		struct VertexGroup
		{
			uint vertex[ 3 ];
			uint normalIndex;
		};

		void LerpVertices( const VertexGroup *v, const VertexGroup *ov, const VertexGroup *verts, Vector3 *lerp, float move[ 3 ], float frontv[ 3 ], float backv[ 3 ] ) const;
		void ApplyLighting( const entity_t *e );
		void DrawFrameLerp( entity_t *e );

	public:
		void Draw( entity_t *e );

	private:
		bool Cull( vec3_t bbox[ 8 ], entity_t *e );

		struct Triangle
		{
			uint vertexIndices[ 3 ]{ 0, 0, 0 };
			uint stIndices[ 3 ]{ 0, 0, 0 };
		};

		struct Frame
		{
			std::string name;
			vec3_t scale{ 0.0f, 0.0f, 0.0f };
			vec3_t translate{ 0.0f, 0.0f, 0.0f };
			std::vector< VertexGroup > vertices;
			vec3_t bounds[ 2 ];
		};

		int numGLCmds_{ 0 };

		int numVertices_{ 0 };
		int numTriangles_{ 0 };
		int numFrames_{ 0 };
		int numSurfaces_{ 0 };

		std::vector< std::string > skinNames_;

		vec3_t shadeVector_{ 0.0f, 0.0f, 0.0f };
		float shadeLight_[ 3 ]{ 0.0f, 0.0f, 0.0f };
		float *shadeDots_{ nullptr };

		std::vector< Triangle > triangles_;
		std::map< std::string, uint > taggedTriangles_;

		std::vector< Vector2 > stCoords_;
		std::vector< int > glCmds_;
		std::vector< Vector3 > lerpedVertices_;

		std::vector< Frame > frames_;
	};
}// namespace nox
