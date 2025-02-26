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

#include <cctype>

#include "q_shared.h"

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

vec3_t vec3_origin = { 0, 0, 0 };
vec4_t vec4_origin = { 0, 0, 0, 0 };

//============================================================================

#if defined( _MSC_VER )
#	pragma optimize( "", off )
#endif

void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float  m[ 3 ][ 3 ];
	float  im[ 3 ][ 3 ];
	float  zrot[ 3 ][ 3 ];
	float  tmpmat[ 3 ][ 3 ];
	float  rot[ 3 ][ 3 ];
	int    i;
	vec3_t vr, vup, vf;

	vf[ 0 ] = dir[ 0 ];
	vf[ 1 ] = dir[ 1 ];
	vf[ 2 ] = dir[ 2 ];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[ 0 ][ 0 ] = vr[ 0 ];
	m[ 1 ][ 0 ] = vr[ 1 ];
	m[ 2 ][ 0 ] = vr[ 2 ];

	m[ 0 ][ 1 ] = vup[ 0 ];
	m[ 1 ][ 1 ] = vup[ 1 ];
	m[ 2 ][ 1 ] = vup[ 2 ];

	m[ 0 ][ 2 ] = vf[ 0 ];
	m[ 1 ][ 2 ] = vf[ 1 ];
	m[ 2 ][ 2 ] = vf[ 2 ];

	memcpy( im, m, sizeof( im ) );

	im[ 0 ][ 1 ] = m[ 1 ][ 0 ];
	im[ 0 ][ 2 ] = m[ 2 ][ 0 ];
	im[ 1 ][ 0 ] = m[ 0 ][ 1 ];
	im[ 1 ][ 2 ] = m[ 2 ][ 1 ];
	im[ 2 ][ 0 ] = m[ 0 ][ 2 ];
	im[ 2 ][ 1 ] = m[ 1 ][ 2 ];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[ 0 ][ 0 ] = zrot[ 1 ][ 1 ] = zrot[ 2 ][ 2 ] = 1.0F;

	zrot[ 0 ][ 0 ] = cos( DEG2RAD( degrees ) );
	zrot[ 0 ][ 1 ] = sin( DEG2RAD( degrees ) );
	zrot[ 1 ][ 0 ] = -sin( DEG2RAD( degrees ) );
	zrot[ 1 ][ 1 ] = cos( DEG2RAD( degrees ) );

	R_ConcatRotations( m, zrot, tmpmat );
	R_ConcatRotations( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ )
	{
		dst[ i ] = rot[ i ][ 0 ] * point[ 0 ] + rot[ i ][ 1 ] * point[ 1 ] + rot[ i ][ 2 ] * point[ 2 ];
	}
}

#if defined( _MSC_VER )
#	pragma optimize( "", on )
#endif


void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
	float        angle;
	static float sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[ YAW ] * ( M_PI * 2 / 360 );
	sy    = sin( angle );
	cy    = cos( angle );
	angle = angles[ PITCH ] * ( M_PI * 2 / 360 );
	sp    = sin( angle );
	cp    = cos( angle );
	angle = angles[ ROLL ] * ( M_PI * 2 / 360 );
	sr    = sin( angle );
	cr    = cos( angle );

	if ( forward )
	{
		forward[ 0 ] = cp * cy;
		forward[ 1 ] = cp * sy;
		forward[ 2 ] = -sp;
	}
	if ( right )
	{
		right[ 0 ] = ( -1 * sr * sp * cy + -1 * cr * -sy );
		right[ 1 ] = ( -1 * sr * sp * sy + -1 * cr * cy );
		right[ 2 ] = -1 * sr * cp;
	}
	if ( up )
	{
		up[ 0 ] = ( cr * sp * cy + -sr * -sy );
		up[ 1 ] = ( cr * sp * sy + -sr * cy );
		up[ 2 ] = cr * cp;
	}
}


void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float  d;
	vec3_t n;
	float  inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

	d = DotProduct( normal, p ) * inv_denom;

	n[ 0 ] = normal[ 0 ] * inv_denom;
	n[ 1 ] = normal[ 1 ] * inv_denom;
	n[ 2 ] = normal[ 2 ] * inv_denom;

	dst[ 0 ] = p[ 0 ] - d * n[ 0 ];
	dst[ 1 ] = p[ 1 ] - d * n[ 1 ];
	dst[ 2 ] = p[ 2 ] - d * n[ 2 ];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int    pos;
	int    i;
	float  minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[ i ] ) < minelem )
		{
			pos     = i;
			minelem = fabs( src[ i ] );
		}
	}
	tempvec[ 0 ] = tempvec[ 1 ] = tempvec[ 2 ] = 0.0F;
	tempvec[ pos ]                             = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}


/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations( float in1[ 3 ][ 3 ], float in2[ 3 ][ 3 ], float out[ 3 ][ 3 ] )
{
	out[ 0 ][ 0 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 0 ] +
	                in1[ 0 ][ 2 ] * in2[ 2 ][ 0 ];
	out[ 0 ][ 1 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 1 ] +
	                in1[ 0 ][ 2 ] * in2[ 2 ][ 1 ];
	out[ 0 ][ 2 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 2 ] +
	                in1[ 0 ][ 2 ] * in2[ 2 ][ 2 ];
	out[ 1 ][ 0 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 0 ] +
	                in1[ 1 ][ 2 ] * in2[ 2 ][ 0 ];
	out[ 1 ][ 1 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 1 ] +
	                in1[ 1 ][ 2 ] * in2[ 2 ][ 1 ];
	out[ 1 ][ 2 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 2 ] +
	                in1[ 1 ][ 2 ] * in2[ 2 ][ 2 ];
	out[ 2 ][ 0 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 0 ] +
	                in1[ 2 ][ 2 ] * in2[ 2 ][ 0 ];
	out[ 2 ][ 1 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 1 ] +
	                in1[ 2 ][ 2 ] * in2[ 2 ][ 1 ];
	out[ 2 ][ 2 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 2 ] +
	                in1[ 2 ][ 2 ] * in2[ 2 ][ 2 ];
}


/*
================
R_ConcatTransforms
================
*/
void R_ConcatTransforms( float in1[ 3 ][ 4 ], float in2[ 3 ][ 4 ], float out[ 3 ][ 4 ] )
{
	out[ 0 ][ 0 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 0 ] +
	                in1[ 0 ][ 2 ] * in2[ 2 ][ 0 ];
	out[ 0 ][ 1 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 1 ] +
	                in1[ 0 ][ 2 ] * in2[ 2 ][ 1 ];
	out[ 0 ][ 2 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 2 ] +
	                in1[ 0 ][ 2 ] * in2[ 2 ][ 2 ];
	out[ 0 ][ 3 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 3 ] +
	                in1[ 0 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 0 ][ 3 ];
	out[ 1 ][ 0 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 0 ] +
	                in1[ 1 ][ 2 ] * in2[ 2 ][ 0 ];
	out[ 1 ][ 1 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 1 ] +
	                in1[ 1 ][ 2 ] * in2[ 2 ][ 1 ];
	out[ 1 ][ 2 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 2 ] +
	                in1[ 1 ][ 2 ] * in2[ 2 ][ 2 ];
	out[ 1 ][ 3 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 3 ] +
	                in1[ 1 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 1 ][ 3 ];
	out[ 2 ][ 0 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 0 ] +
	                in1[ 2 ][ 2 ] * in2[ 2 ][ 0 ];
	out[ 2 ][ 1 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 1 ] +
	                in1[ 2 ][ 2 ] * in2[ 2 ][ 1 ];
	out[ 2 ][ 2 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 2 ] +
	                in1[ 2 ][ 2 ] * in2[ 2 ][ 2 ];
	out[ 2 ][ 3 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 3 ] +
	                in1[ 2 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 2 ][ 3 ];
}


//============================================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle( float a2, float a1, float frac )
{
	if ( a1 - a2 > 180 )
		a1 -= 360;
	if ( a1 - a2 < -180 )
		a1 += 360;
	return a2 + frac * ( a1 - a2 );
}


float anglemod( float a )
{
#if 0
	if( a >= 0 )
		a -= 360 * (int)( a / 360 );
	else
		a += 360 * ( 1 + (int)( -a / 360 ) );
#endif
	a = ( 360.0 / 65536 ) * ( ( int ) ( a * ( 65536 / 360.0 ) ) & 65535 );
	return a;
}

// this is the slow, general version
int BoxOnPlaneSide2( vec3_t emins, vec3_t emaxs, struct cplane_s *p )
{
	int    i;
	float  dist1, dist2;
	int    sides;
	vec3_t corners[ 2 ];

	for ( i = 0; i < 3; i++ )
	{
		if ( p->normal[ i ] < 0 )
		{
			corners[ 0 ][ i ] = emins[ i ];
			corners[ 1 ][ i ] = emaxs[ i ];
		}
		else
		{
			corners[ 1 ][ i ] = emins[ i ];
			corners[ 0 ][ i ] = emaxs[ i ];
		}
	}
	dist1 = DotProduct( p->normal, corners[ 0 ] ) - p->dist;
	dist2 = DotProduct( p->normal, corners[ 1 ] ) - p->dist;
	sides = 0;
	if ( dist1 >= 0 )
		sides = 1;
	if ( dist2 < 0 )
		sides |= 2;

	return sides;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, struct cplane_s *p )
{
	float dist1, dist2;
	int   sides;

	// fast axial cases
	if ( p->type < 3 )
	{
		if ( p->dist <= emins[ p->type ] )
			return 1;
		if ( p->dist >= emaxs[ p->type ] )
			return 2;
		return 3;
	}

	// general case
	switch ( p->signbits )
	{
		case 0:
			dist1 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			dist2 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			break;
		case 1:
			dist1 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			dist2 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			break;
		case 2:
			dist1 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			dist2 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			break;
		case 3:
			dist1 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			dist2 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			break;
		case 4:
			dist1 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			dist2 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			break;
		case 5:
			dist1 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			dist2 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			break;
		case 6:
			dist1 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			dist2 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			break;
		case 7:
			dist1 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			dist2 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			break;
		default:
			dist1 = dist2 = 0;// shut up compiler
			assert( 0 );
			break;
	}

	sides = 0;
	if ( dist1 >= p->dist )
		sides = 1;
	if ( dist2 < p->dist )
		sides |= 2;

	assert( sides != 0 );

	return sides;
}

void AddPointToBounds( vec3_t v, vec3_t mins, vec3_t maxs )
{
	int   i;
	vec_t val;

	for ( i = 0; i < 3; i++ )
	{
		val = v[ i ];
		if ( val < mins[ i ] )
			mins[ i ] = val;
		if ( val > maxs[ i ] )
			maxs[ i ] = val;
	}
}


int VectorCompare( vec3_t v1, vec3_t v2 )
{
	if ( v1[ 0 ] != v2[ 0 ] || v1[ 1 ] != v2[ 1 ] || v1[ 2 ] != v2[ 2 ] )
		return 0;

	return 1;
}

void VectorMA( vec3_t veca, float scale, vec3_t vecb, vec3_t vecc )
{
	vecc[ 0 ] = veca[ 0 ] + scale * vecb[ 0 ];
	vecc[ 1 ] = veca[ 1 ] + scale * vecb[ 1 ];
	vecc[ 2 ] = veca[ 2 ] + scale * vecb[ 2 ];
}

void CrossProduct( vec3_t v1, vec3_t v2, vec3_t cross )
{
	cross[ 0 ] = v1[ 1 ] * v2[ 2 ] - v1[ 2 ] * v2[ 1 ];
	cross[ 1 ] = v1[ 2 ] * v2[ 0 ] - v1[ 0 ] * v2[ 2 ];
	cross[ 2 ] = v1[ 0 ] * v2[ 1 ] - v1[ 1 ] * v2[ 0 ];
}

double sqrt( double x );

int Q_log2( int val )
{
	int answer = 0;
	while ( val >>= 1 )
		answer++;
	return answer;
}


//====================================================================================

/*
============
COM_SkipPath
============
*/
char *COM_SkipPath( char *pathname )
{
	char *last;

	last = pathname;
	while ( *pathname )
	{
		if ( *pathname == '/' )
			last = pathname + 1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out )
{
	while ( *in && *in != '.' )
		*out++ = *in++;
	*out = 0;
}

const char *COM_FileExtension( char *in )
{
	static char exten[ 8 ];
	int         i;

	while ( *in && *in != '.' )
		in++;
	if ( !*in )
		return "";
	in++;
	for ( i = 0; i < 7 && *in; i++, in++ )
		exten[ i ] = *in;
	exten[ i ] = 0;
	return exten;
}

/*
============
COM_FileBase
============
*/
void COM_FileBase( char *in, char *out )
{
	char *s, *s2;

	s = in + strlen( in ) - 1;

	while ( s != in && *s != '.' )
		s--;

	for ( s2 = s; s2 != in && *s2 != '/'; s2-- )
		;

	if ( s - s2 < 2 )
		out[ 0 ] = 0;
	else
	{
		s--;
		strncpy( out, s2 + 1, s - s2 );
		out[ s - s2 ] = 0;
	}
}

/*
============
COM_FilePath

Returns the path up to, but not including the last /
============
*/
void COM_FilePath( char *in, char *out )
{
	char *s;

	s = in + strlen( in ) - 1;

	while ( s != in && *s != '/' )
		s--;

	strncpy( out, in, s - in );
	out[ s - in ] = 0;
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char *path, char *extension )
{
	char *src;
	//
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	//
	src = path + strlen( path ) - 1;

	while ( *src != '/' && src != path )
	{
		if ( *src == '.' )
			return;// it has an extension
		src--;
	}

	strcat( path, extension );
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

bool bigendien;

// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
short ( *_BigShort )( short l );
short ( *_LittleShort )( short l );
int ( *_BigLong )( int l );
int ( *_LittleLong )( int l );
float ( *_BigFloat )( float l );
float ( *_LittleFloat )( float l );

short BigShort( short l ) { return _BigShort( l ); }
short LittleShort( short l ) { return _LittleShort( l ); }
int   BigLong( int l ) { return _BigLong( l ); }
int   LittleLong( int l ) { return _LittleLong( l ); }
float BigFloat( float l ) { return _BigFloat( l ); }
float LittleFloat( float l ) { return _LittleFloat( l ); }

short ShortSwap( short l )
{
	byte b1, b2;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;

	return ( b1 << 8 ) + b2;
}

short ShortNoSwap( short l )
{
	return l;
}

int LongSwap( int l )
{
	byte b1, b2, b3, b4;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;
	b3 = ( l >> 16 ) & 255;
	b4 = ( l >> 24 ) & 255;

	return ( ( int ) b1 << 24 ) + ( ( int ) b2 << 16 ) + ( ( int ) b3 << 8 ) + b4;
}

int LongNoSwap( int l )
{
	return l;
}

float FloatSwap( float f )
{
	union
	{
		float f;
		byte  b[ 4 ];
	} dat1, dat2;


	dat1.f      = f;
	dat2.b[ 0 ] = dat1.b[ 3 ];
	dat2.b[ 1 ] = dat1.b[ 2 ];
	dat2.b[ 2 ] = dat1.b[ 1 ];
	dat2.b[ 3 ] = dat1.b[ 0 ];
	return dat2.f;
}

float FloatNoSwap( float f )
{
	return f;
}

/*
================
Swap_Init
================
*/
void Swap_Init( void )
{
	byte swaptest[ 2 ] = { 1, 0 };

	// set the byte swapping variables in a portable manner
	if ( *( short * ) swaptest == 1 )
	{
		bigendien    = false;
		_BigShort    = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong     = LongSwap;
		_LittleLong  = LongNoSwap;
		_BigFloat    = FloatSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien    = true;
		_BigShort    = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong     = LongNoSwap;
		_LittleLong  = LongSwap;
		_BigFloat    = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char *va( const char *format, ... )
{
	va_list     argptr;
	static char string[ 1024 ];

	va_start( argptr, format );
	vsprintf( string, format, argptr );
	va_end( argptr );

	return string;
}


static char com_token[ MAX_TOKEN_CHARS ];

/*
==============
Script_GetLineLength

Check how long the current line is, so we can allocate a buffer to store it.
==============
*/
size_t Script_GetLineLength( const char *data )
{
	const char *p = data;
	while ( *p != '\0' && *p != '\n' && *p != '\r' )
	{
		p++;
	}

	return p - data;
}

/*
==============
Script_GetLine

Read the line into the specified buffer.
==============
*/
const char *Script_GetLine( const char *data, char *dest, size_t destSize )
{
	const char *p = data;
	char       *d = dest;

	size_t length = Script_GetLineLength( data );
	for ( size_t i = 0; i < length; ++i )
	{
		*d++ = *p++;
		if ( d - dest >= destSize )
		{
#if !defined( NDEBUG )
			assert( 0 );
#endif
			break;
		}
	}

	return p;
}

const char *Script_SkipWhitespace( const char *data )
{
	char c;
	while ( ( c = *data ) <= ' ' )
	{
		if ( c == 0 )
		{
			return nullptr;
		}
		data++;
	}

	return data;
}

const char *Script_SkipComment( const char *data )
{
	// Single line comment
	if ( data[ 0 ] == '/' && data[ 1 ] == '/' )
	{
		while ( *data && *data != '\n' )
		{
			data++;
		}
	}

	// Multi line comment
	if ( data[ 0 ] == '/' && data[ 1 ] == '*' )
	{
		while ( *data && data[ 0 ] != '*' && data[ 1 ] != '/' )
		{
			data++;
		}
	}

	return data;
}

const char *Script_SkipLine( const char *p )
{
	while ( *p && *p != '\n' ) p++;
	return p;
}

/*
==============
Script_Parse

Like COM_Parse, but let's you specify what deliminator you want to use before
returning a token.
==============
*/
const char *Script_Parse( const char **buffer, const char *deliminator )
{
	com_token[ 0 ] = '\0';

	const char *data = *buffer;
	if ( data == nullptr )
	{
		*buffer = nullptr;
		return "";
	}

	int length = 0;
	while ( true )
	{
		data = Script_SkipWhitespace( data );
		if ( data == nullptr )
		{
			*buffer = nullptr;
			return "";
		}

		const char *oldPos = data;
		data               = Script_SkipComment( data );
		if ( data != oldPos )
		{
			continue;
		}

		while ( true )
		{
			char c = *data++;

			// Check for the deliminators
			const char *d = deliminator;
			while ( *d )
			{
				if ( c == '\0' || *d == c )
				{
					*buffer             = data;
					com_token[ length ] = '\0';
					return com_token;
				}

				d++;
			}

			if ( length < MAX_TOKEN_CHARS )
			{
				com_token[ length ] = c;
				length++;
			}
		}

		if ( length >= MAX_TOKEN_CHARS )
		{
			Com_Printf( "Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS );
			length = 0;
		}

		com_token[ length ] = '\0';
		break;
	}

	*buffer = data;
	return com_token;
}

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
const char *COM_Parse( const char **data_p )
{
	com_token[ 0 ] = 0;

	const char *data = *data_p;
	if ( !data )
	{
		*data_p = nullptr;
		return "";
	}

	// skip whitespace
skipwhite:
	data = Script_SkipWhitespace( data );
	if ( data == nullptr )
	{
		*data_p = nullptr;
		return "";
	}

	const char *oldPos = data;
	data               = Script_SkipComment( data );
	if ( data != oldPos )
	{
		goto skipwhite;
	}

	// handle quoted strings specially
	int  len = 0;
	char c   = *data;
	if ( c == '\"' )
	{
		data++;
		while ( true )
		{
			c = *data++;
			if ( c == '\"' || !c )
			{
				com_token[ len ] = 0;
				*data_p          = data;
				return com_token;
			}
			if ( len < MAX_TOKEN_CHARS )
			{
				com_token[ len ] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do {
		if ( len < MAX_TOKEN_CHARS )
		{
			com_token[ len ] = c;
			len++;
		}
		data++;
		c = *data;
	} while ( c > 32 );

	if ( len == MAX_TOKEN_CHARS )
	{
		Com_Printf( "Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS );
		len = 0;
	}
	com_token[ len ] = 0;

	*data_p = data;
	return com_token;
}


/*
===============
Com_PageInMemory

===============
*/
int paged_total;

void Com_PageInMemory( byte *buffer, int size )
{
	int i;

	for ( i = size - 1; i > 0; i -= 4096 )
		paged_total += buffer[ i ];
}


/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

// FIXME: replace all Q_stricmp with Q_strcasecmp
int Q_stricmp( const char *s1, const char *s2 )
{
#if defined( WIN32 )
	return _stricmp( s1, s2 );
#else
	return strcasecmp( s1, s2 );
#endif
}


int Q_strncasecmp( const char *s1, const char *s2, int n )
{
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- )
			return 0;// strings are equal until end point

		if ( c1 != c2 )
		{
			if ( c1 >= 'a' && c1 <= 'z' )
				c1 -= ( 'a' - 'A' );
			if ( c2 >= 'a' && c2 <= 'z' )
				c2 -= ( 'a' - 'A' );
			if ( c1 != c2 )
				return -1;// strings not equal
		}
	} while ( c1 );

	return 0;// strings are equal
}

int Q_strcasecmp( const char *s1, const char *s2 )
{
	return Q_strncasecmp( s1, s2, 99999 );
}

char *Q_strtolower( char *s )
{
	for ( size_t i = 0; s[ i ] != '\0'; ++i )
	{
		s[ i ] = ( char ) tolower( s[ i ] );
	}
	return s;
}

char *Q_strntolower( char *s, size_t n )
{
	for ( size_t i = 0; i < n; ++i )
	{
		if ( s[ i ] == '\0' )
		{
			break;
		}
		s[ i ] = ( char ) tolower( s[ i ] );
	}
	return s;
}

/* https://stackoverflow.com/a/19692380 */
int Q_vscprintf( const char *format, va_list pArgs )
{
	va_list argCopy;
	va_copy( argCopy, pArgs );
	int retVal = vsnprintf( NULL, 0, format, argCopy );
	va_end( argCopy );
	return retVal;
}

void Com_sprintf( char *dest, int size, const char *fmt, ... )
{
	va_list argPtr;
	va_start( argPtr, fmt );

	int length = Q_vscprintf( fmt, argPtr ) + 1;
	if ( length >= size )
	{
		Com_Printf( "Com_sprintf: overflow of %i in %i\n", length, size );
	}

	char *msg = ( char * ) malloc( length );
	vsprintf( msg, fmt, argPtr );

	va_end( argPtr );

	strncpy( dest, msg, size - 1 );

	free( msg );
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
const char *Info_ValueForKey( char *s, const char *key )
{
	char        pkey[ 512 ];
	static char value[ 2 ][ 512 ];// use two buffers so compares
	                              // work without stomping on each other
	static int valueindex;
	char      *o;

	valueindex ^= 1;
	if ( *s == '\\' )
		s++;
	while ( 1 )
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[ valueindex ];

		while ( *s != '\\' && *s )
		{
			if ( !*s )
				return "";
			*o++ = *s++;
		}
		*o = 0;

		if ( !strcmp( key, pkey ) )
			return value[ valueindex ];

		if ( !*s )
			return "";
		s++;
	}
}

void Info_RemoveKey( char *s, const char *key )
{
	char *start;
	char  pkey[ 512 ];
	char  value[ 512 ];
	char *o;

	if ( strstr( key, "\\" ) )
	{
		//		Com_Printf ("Can't use a key with a \\\n");
		return;
	}

	while ( 1 )
	{
		start = s;
		if ( *s == '\\' )
			s++;
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while ( *s != '\\' && *s )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;

		if ( !strcmp( key, pkey ) )
		{
			strcpy( start, s );// remove this part
			return;
		}

		if ( !*s )
			return;
	}
}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
bool Info_Validate( char *s )
{
	if ( strstr( s, "\"" ) )
		return false;
	if ( strstr( s, ";" ) )
		return false;
	return true;
}

void Info_SetValueForKey( char *s, const char *key, const char *value )
{
	char         newi[ MAX_INFO_STRING ], *v;
	int          c;
	unsigned int maxsize = MAX_INFO_STRING;

	if ( strstr( key, "\\" ) || strstr( value, "\\" ) )
	{
		Com_Printf( "Can't use keys or values with a \\\n" );
		return;
	}

	if ( strstr( key, ";" ) )
	{
		Com_Printf( "Can't use keys or values with a semicolon\n" );
		return;
	}

	if ( strstr( key, "\"" ) || strstr( value, "\"" ) )
	{
		Com_Printf( "Can't use keys or values with a \"\n" );
		return;
	}

	if ( strlen( key ) > MAX_INFO_KEY - 1 || strlen( value ) > MAX_INFO_KEY - 1 )
	{
		Com_Printf( "Keys and values must be < 64 characters.\n" );
		return;
	}
	Info_RemoveKey( s, key );
	if ( !value || !strlen( value ) )
		return;

	Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, value );

	if ( strlen( newi ) + strlen( s ) > maxsize )
	{
		Com_Printf( "Info string length exceeded\n" );
		return;
	}

	// only copy ascii values
	s += strlen( s );
	v = newi;
	while ( *v )
	{
		c = *v++;
		c &= 127;// strip high bits
		if ( c >= 32 && c < 127 )
			*s++ = c;
	}
	*s = 0;
}

//====================================================================
