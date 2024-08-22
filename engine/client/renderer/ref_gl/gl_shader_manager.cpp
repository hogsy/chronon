// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "gl_local.h"
#include "gl_shader_manager.h"

chr::renderer::gl::ShaderProgram::ShaderProgram()
{
	Q_ZERO( glStages, sizeof( uint32_t ) * Stage::MAX_STAGES );
}

chr::renderer::gl::ShaderProgram::~ShaderProgram()
{
}

bool chr::renderer::gl::ShaderProgram::LoadShaderStage( const std::string &path, Stage stage )
{
	assert( glStages[ stage ] == 0 );
	if ( glStages[ stage ] != 0 )
	{
		Com_Printf( "A stage of this type has already been cached!\n" );
		return false;
	}

	GLenum glStage;
	if ( stage == Stage::STAGE_PIXEL )
		glStage = GL_FRAGMENT_SHADER;
	else if ( stage == Stage::STAGE_VERTEX )
		glStage = GL_VERTEX_SHADER;
	else
	{
		Com_Printf( "Unsupported shader stage: %u\n", stage );
		return false;
	}

	XGL_CALL( glStages[ stage ] = glCreateShader( glStage ) );
	if ( glStages[ stage ] == 0 )
	{
		Com_Printf( "An error occurred on stage creation!\n" );
		return false;
	}

	char *buffer;
	int   length = FS_LoadFile( path.c_str(), ( void   **) &buffer );
	if ( length == -1 )
	{
		Com_Printf( "Failed to open shader: %s\n", path.c_str() );
		XGL_CALL( glDeleteShader( glStages[ stage ] ) );
		glStages[ stage ] = 0;
		return false;
	}

	XGL_CALL( glShaderSource( glStages[ stage ], 1, &buffer, &length ) );
	XGL_CALL( glCompileShader( glStages[ stage ] ) );

	FS_FreeFile( buffer );

	GLint compileStatus;
	XGL_CALL( glGetShaderiv( glStages[ stage ], GL_COMPILE_STATUS, &compileStatus ) );
	if ( compileStatus == GL_FALSE )
	{
		GLint maxLength;
		XGL_CALL( glGetShaderiv( glStages[ stage ], GL_INFO_LOG_LENGTH, &maxLength ) );

		std::vector< GLchar > infoLog( maxLength );
		XGL_CALL( glGetShaderInfoLog( glStages[ stage ], maxLength, &maxLength, &infoLog[ 0 ] ) );

		Com_Printf( "Failed to compile shader: %s\n%s\n", path.c_str(), &infoLog[ 0 ] );

		XGL_CALL( glDeleteShader( glStages[ stage ] ) );

		return false;
	}

	return true;
}

void chr::renderer::gl::ShaderProgram::Enable()
{
	assert( glProgram != 0 );
	if ( glProgram == 0 )
	{
		//TODO: use fallback
		return;
	}
	XGL_CALL( glUseProgram( glProgram ) );
}

void chr::renderer::gl::ShaderProgram::Disable()
{
	XGL_CALL( glUseProgram( 0 ) );
}

void chr::renderer::gl::ShaderProgram::Reload()
{
}
