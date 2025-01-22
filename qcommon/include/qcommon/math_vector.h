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
	struct Vector2
	{
		float x{ 0.0f };
		float y{ 0.0f };

		virtual inline float &operator[]( const uint i )
		{
			assert( i < 2 );
			return *( ( &x ) + i );
		}

		virtual inline void Zero()
		{
			x = y = 0.0f;
		}
	};

	struct Vector3 : Vector2
	{
		float z{ 0.0f };

		inline float &operator[]( const uint i ) override
		{
			assert( i < 3 );
			return *( ( &x ) + i );
		}

		inline float Length() const
		{
			return std::sqrt( x * x + y * y + z * z );
		}

		inline float Normalize()
		{
			float length = Length();
			if ( length > 0.0f )
			{
				float i = 1.0f / length;
				x *= i;
				y *= i;
				z *= i;
			}

			return length;
		}

		inline void Zero() override
		{
			x = y = z = 0.0f;
		}
	};

	struct Vector4 : Vector3
	{
		float w{ 0.0f };

		inline float &operator[]( const uint i ) override
		{
			assert( i < 4 );
			return *( ( &x ) + i );
		}
	};
}// namespace nox

// Everything below will eventually be nixed

#define OLD_VECTOR_TYPES

typedef float vec_t;
#ifdef OLD_VECTOR_TYPES
typedef vec_t vec2_t[ 2 ];
#else
typedef nox::Vector2 vec2_t;
#endif
#ifdef OLD_VECTOR_TYPES
typedef vec_t vec3_t[ 3 ];
#else
typedef nox::Vector3 vec3_t;
#endif
#ifdef OLD_VECTOR_TYPES
typedef vec_t vec4_t[ 4 ];
#else
typedef nox::Vector4 vec4_t;
#endif

extern vec3_t vec3_origin;
extern vec4_t vec4_origin;

#define VectorClear( a )        ( a[ 0 ] = a[ 1 ] = a[ 2 ] = 0 )
#define VectorNegate( a, b )    ( b[ 0 ] = -a[ 0 ], b[ 1 ] = -a[ 1 ], b[ 2 ] = -a[ 2 ] )
#define VectorSet( v, x, y, z ) ( v[ 0 ] = ( x ), v[ 1 ] = ( y ), v[ 2 ] = ( z ) )

void VectorMA( vec3_t veca, float scale, vec3_t vecb, vec3_t vecc );

// just in case you don't want to use the macros

static inline vec_t DotProduct( const vec3_t v1, const vec3_t v2 )
{
	return v1[ 0 ] * v2[ 0 ] + v1[ 1 ] * v2[ 1 ] + v1[ 2 ] * v2[ 2 ];
}

template< typename TA, typename TB, typename TC >
static inline void VectorSubtract( const TA veca, const TB vecb, TC out )
{
	out[ 0 ] = veca[ 0 ] - vecb[ 0 ];
	out[ 1 ] = veca[ 1 ] - vecb[ 1 ];
	out[ 2 ] = veca[ 2 ] - vecb[ 2 ];
}

static inline void VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[ 0 ] = veca[ 0 ] + vecb[ 0 ];
	out[ 1 ] = veca[ 1 ] + vecb[ 1 ];
	out[ 2 ] = veca[ 2 ] + vecb[ 2 ];
}

template< typename TA, typename TB >
static inline void VectorCopy( const TA in, TB out )
{
	out[ 0 ] = in[ 0 ];
	out[ 1 ] = in[ 1 ];
	out[ 2 ] = in[ 2 ];
}

static inline void ClearBounds( vec3_t mins, vec3_t maxs )
{
	mins[ 0 ] = mins[ 1 ] = mins[ 2 ] = 99999;
	maxs[ 0 ] = maxs[ 1 ] = maxs[ 2 ] = -99999;
}

void AddPointToBounds( vec3_t v, vec3_t mins, vec3_t maxs );
int VectorCompare( vec3_t v1, vec3_t v2 );

static inline vec_t VectorLength( const vec3_t v )
{
	float length = 0.0f;
	for ( chr::uint i = 0; i < 3; i++ )
		length += v[ i ] * v[ i ];

	return std::sqrt( length );
}

void CrossProduct( vec3_t v1, vec3_t v2, vec3_t cross );

static inline vec_t VectorNormalize( vec3_t v )
{
	float length = VectorLength( v );
	if ( length > 0.0f )
	{
		float i = 1 / length;
		v[ 0 ] *= i;
		v[ 1 ] *= i;
		v[ 2 ] *= i;
	}

	return length;
}

static inline void VectorScale( const vec3_t in, vec_t scale, vec3_t out )
{
	out[ 0 ] = in[ 0 ] * scale;
	out[ 1 ] = in[ 1 ] * scale;
	out[ 2 ] = in[ 2 ] * scale;
}
