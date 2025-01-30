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

#include <sstream>

#include "qcommon/qcommon.h"
#include "renderer/renderer.h"
#include "renderer/ref_gl/gl_local.h"

#include "model_mda.h"
#include "model_alias.h"

chr::MDAModel::MDAModel()  = default;
chr::MDAModel::~MDAModel() = default;

void chr::MDAModel::Draw( entity_t *entity )
{
	if ( baseModel == nullptr || baseModel->type != mod_alias )
	{
		return;
	}

#if 0//TODO this is just an outline for now...
	Profile *profile;

	//TODO: revisit this...if we dont have a profile set, just use the first (groan)
	const auto &i = profiles.find( entity->profile );
	if ( i == profiles.end() )
	{
		profile = &profiles.begin()->second;
	}
	else
	{
		profile = &i->second;
	}

	for ( const auto &skin : profile->skins )
	{
		for ( const auto &pass : skin.passes )
		{
			switch ( pass.alpha )
			{
				case Pass::AlphaFunc::NONE:
					break;
				case Pass::AlphaFunc::LT128:
					glEnable( GL_ALPHA_TEST );
					glAlphaFunc( GL_LESS, 0.5f );
					break;
				case Pass::AlphaFunc::GE128:
					glEnable( GL_ALPHA_TEST );
					glAlphaFunc( GL_GEQUAL, 0.5f );
					break;
				case Pass::AlphaFunc::GT0:
					glEnable( GL_ALPHA_TEST );
					glAlphaFunc( GL_GEQUAL, 0.5f );
					break;
			}

			switch ( pass.blend )
			{
				case Pass::BlendMode::NONE:
					break;
				case Pass::BlendMode::NORMAL:
					//TODO: what the hell does "normal" mean?
					break;
				case Pass::BlendMode::MULTIPLY:
					glEnable( GL_BLEND );
					glBlendFunc( GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA );
					break;
				case Pass::BlendMode::ADD:
					glEnable( GL_BLEND );
					glBlendFunc( GL_ONE, GL_ONE );
					break;
			}

			// draw

			if ( pass.blend != Pass::BlendMode::NONE )
			{
				glDisable( GL_BLEND );
			}
			if ( pass.alpha != Pass::AlphaFunc::NONE )
			{
				glDisable( GL_ALPHA_TEST );
			}
		}
	}
#endif

	( ( AliasModel * ) baseModel->extradata )->Draw( entity );
}

bool chr::MDAModel::Parse( const std::string &buf )
{
	std::stringstream ss( &buf[ 4 ] );

	std::string token;
	while ( ss >> token )
	{
		//TODO: we're temporarily ignoring the special $ and & tokens here for now!
		if ( token == "#" || token[ 0 ] == '$' || token[ 0 ] == '&' )
		{
			ss.ignore( std::numeric_limits< std::streamsize >::max(), '\n' );
		}
		else if ( token == "basemodel" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected filename after 'basemodel'!\n" );
				return false;
			}

			token = io::SanitizePath( token );

			baseModel = Mod_RegisterModel( token.c_str() );
			if ( baseModel == nullptr )
			{
				Com_Printf( "Failed to load model (%s) for MDA!\n", token.c_str() );
				return false;
			}

			ss.ignore( std::numeric_limits< std::streamsize >::max(), '\n' );
		}
		else if ( token == "profile" )
		{
			std::string tag;
			if ( !( ss >> tag ) )
			{
				Com_Printf( "Expected tag or '{' after 'profile'!\n" );
				return false;
			}

			// oh this really sucks! i assumed all profiles have a name, but they don't...
			if ( tag == "{" )
			{
				tag = "DFLT";
			}
			else if ( !( ss >> token ) && token != "{" )
			{
				Com_Printf( "Expected '{' after tag!\n" );
				return false;
			}

			Profile profile;
			if ( !profile.Parse( ss ) )
			{
				Com_Printf( "Failed to parse MDA profile (%s)!\n", tag.c_str() );
				return false;
			}

			profiles.insert( std::make_pair( tag, profile ) );
		}
		else if ( token == "headtri" )
		{
			float x, y, z;
			if ( !( ss >> x >> y >> z ) )
			{
				Com_Printf( "Expected 'x y z' after 'headtri!\n" );
				return false;
			}

			headTriangle[ 0 ] = x;
			headTriangle[ 1 ] = y;
			headTriangle[ 2 ] = z;

			ss.ignore( std::numeric_limits< std::streamsize >::max(), '\n' );
		}
		else
		{
			Com_Printf( "Unknown token (%s), ignoring!\n", token.c_str() );
		}
	}

	if ( profiles.empty() )
	{
		Com_Printf( "No profiles specified in MDA!\n" );
		return false;
	}

	return true;
}

bool chr::MDAModel::Profile::Parse( std::stringstream &ss )
{
	std::string token;
	while ( ss >> token )
	{
		if ( token == "}" )
		{
			break;
		}

		if ( token == "#" )
		{
			ss.ignore( std::numeric_limits< std::streamsize >::max(), '\n' );
		}
		else if ( token == "evaluate" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected expression after 'evaluate!\n" );
				return false;
			}

			evaluation = token;
			evaluation.erase( std::remove( evaluation.begin(), evaluation.end(), '\"' ),
			                  evaluation.end() );
		}
		else if ( token == "skin" )
		{
			if ( !( ss >> token ) || token != "{" )
			{
				Com_Printf( "Expected '{' after 'skin'!\n" );
				return false;
			}

			Skin skin;
			if ( !skin.Parse( ss ) )
			{
				Com_Printf( "Failed to parse MDA skin!\n" );
				return false;
			}

			skins.push_back( skin );
		}
		else
		{
			Com_Printf( "Unknown token (%s), ignoring!\n", token.c_str() );
		}
	}

	if ( skins.empty() )
	{
		Com_Printf( "Encountered an empty profile!\n" );
		return false;
	}

	return true;
}

bool chr::MDAModel::Skin::Parse( std::stringstream &ss )
{
	std::string token;
	while ( ss >> token )
	{
		if ( token == "}" )
		{
			return true;
		}

		if ( token == "#" )
		{
			ss.ignore( std::numeric_limits< std::streamsize >::max(), '\n' );
		}
		else if ( token == "pass" )
		{
			if ( !( ss >> token ) || token != "{" )
			{
				Com_Printf( "Expected '{' after 'pass'!\n" );
				return false;
			}

			Pass pass;
			if ( !pass.Parse( ss ) )
			{
				Com_Printf( "Failed to parse MDA pass!\n" );
				return false;
			}

			passes.push_back( pass );
		}
		else
		{
			Com_Printf( "Unknown token (%s), ignoring!\n", token.c_str() );
		}
	}

	return false;
}

bool chr::MDAModel::Pass::Parse( std::stringstream &ss )
{
	std::string token;
	while ( ss >> token )
	{
		if ( token == "}" )
		{
			return true;
		}

		if ( token == "#" )
		{
			ss.ignore( std::numeric_limits< std::streamsize >::max(), '\n' );
		}
		else if ( token == "map" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected filename after 'map'!\n" );
				return false;
			}

			token = io::SanitizePath( token );

			size_t p = token.find_last_of( '.' );
			if ( p == std::string::npos )
			{
				token += ".tga";
			}

			map = GL_FindImage( token, it_skin );
		}
		else if ( token == "alphafunc" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected type after 'alphafunc'!\n" );
				return false;
			}

			if ( token == "gt0" ) { alpha = AlphaFunc::GT0; }
			else if ( token == "ge128" ) { alpha = AlphaFunc::GE128; }
			else if ( token == "lt128" ) { alpha = AlphaFunc::LT128; }
			else
			{
				Com_Printf( "Unknown alphafunc type (%s)!\n", token.c_str() );
			}
		}
		else if ( token == "rgbgen" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected type after 'rgbgen'!\n" );
				return false;
			}

			if ( token == "none" ) { rgb = RGBGen::NONE; }
			else if ( token == "identity" ) { rgb = RGBGen::IDENTITY; }
			else if ( token == "diffusezero" ) { rgb = RGBGen::DIFFUSE_ZERO; }
			else if ( token == "ambient" ) { rgb = RGBGen::AMBIENT; }
			else
			{
				Com_Printf( "Unknown rgbgen type (%s)!\n", token.c_str() );
			}
		}
		else if ( token == "blendmode" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected type after 'blendmode'!\n" );
				return false;
			}

			if ( token == "none" ) { blend = BlendMode::NONE; }
			else if ( token == "normal" ) { blend = BlendMode::NORMAL; }
			else if ( token == "multiply" ) { blend = BlendMode::MULTIPLY; }
			else if ( token == "add" ) { blend = BlendMode::ADD; }
			else
			{
				Com_Printf( "Unknown blend mode (%s)!\n", token.c_str() );
			}
		}
		else if ( token == "cull" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected type after 'cull'!\n" );
				return false;
			}

			if ( token == "none" ) { cull = CullMode::NONE; }
			else if ( token == "front" ) { cull = CullMode::FRONT; }
			else if ( token == "back" ) { cull = CullMode::BACK; }
			else
			{
				Com_Printf( "Unknown cull mode (%s)!\n", token.c_str() );
			}
		}
		else if ( token == "uvgen" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected type after 'uvgen'!\n" );
				return false;
			}

			if ( token == "sphere" ) { uvgen = UVGen::SPHERE; }
			else
			{
				Com_Printf( "Unknown uvgen mode (%s)!\n", token.c_str() );
			}
		}
		else if ( token == "uvmod" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected type after 'uvmod'!\n" );
				return false;
			}

			if ( token == "scroll" )
			{
				uvMod = UVMod::SCROLL;
				if ( !( ss >> uvModScroll.x >> uvModScroll.y ) )
				{
					Com_Printf( "Expected 'x y' after 'scroll'!\n" );
					return false;
				}
			}
			else
			{
				Com_Printf( "Unknown uvmod mode (%s)!\n", token.c_str() );
			}
		}
		else if ( token == "depthfunc" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected type after 'depthfunc'!\n" );
				return false;
			}

			if ( token == "none" ) { depth = DepthFunc::NONE; }
			else if ( token == "equal" ) { depth = DepthFunc::EQUAL; }
			else if ( token == "less" ) { depth = DepthFunc::LESS; }
			else
			{
				Com_Printf( "Unknown depthfunc mode (%s)!\n", token.c_str() );
			}
		}
		else if ( token == "depthwrite" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected value after 'depthwrite'!\n" );
				return false;
			}

			if ( token == "1" ) { depthWrite = true; }
			else if ( token == "0" ) { depthWrite = false; }
			else
			{
				Com_Printf( "Unknown depthwrite mode (%s)!\n", token.c_str() );
			}
		}
		else
		{
			Com_Printf( "Unknown token (%s), ignoring!\n", token.c_str() );
		}
	}

	return false;
}
