// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 1997-2001 Id Software, Inc.
// Copyright (C) 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#ifdef _WIN32
#	include <windows.h>

#	define GLEW_STATIC 1
#endif

#include <GL/glew.h>

#if !defined( NDEBUG )
#	define XGL_CALL( X )                     \
		{                                     \
			glGetError();                     \
			X;                                \
			unsigned int _err = glGetError(); \
			assert( _err == GL_NO_ERROR );    \
		}
#else
#	define XGL_CALL( X ) X
#endif

bool QGL_Init( void );
