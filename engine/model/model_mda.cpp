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
			if ( !ParseProfile( profile, ss ) )
			{
				Com_Printf( "Failed to parse MDA profile (%s)!\n", tag.c_str() );
				return false;
			}

			profiles.insert( std::make_pair( tag, Profile() ) );
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

bool chr::MDAModel::ParseProfile( Profile &profile, std::stringstream &ss )
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

			profile.evaluation = token;
			profile.evaluation.erase( std::remove( profile.evaluation.begin(), profile.evaluation.end(), '\"' ),
			                          profile.evaluation.end() );
		}
		else if ( token == "skin" )
		{
			if ( !( ss >> token ) || token != "{" )
			{
				Com_Printf( "Expected '{' after 'skin'!\n" );
				return false;
			}

			Skin skin;
			if ( !ParseSkin( skin, ss ) )
			{
				Com_Printf( "Failed to parse MDA skin!\n" );
				return false;
			}

			profile.skins.push_back( skin );
		}
		else
		{
			Com_Printf( "Unknown token (%s), ignoring!\n", token.c_str() );
		}
	}

	if ( profile.skins.empty() )
	{
		Com_Printf( "Encountered an empty profile!\n" );
		return false;
	}

	return true;
}

bool chr::MDAModel::ParseSkin( Skin &skin, std::stringstream &ss )
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
			if ( !ParsePass( pass, ss ) )
			{
				Com_Printf( "Failed to parse MDA pass!\n" );
				return false;
			}

			skin.passes.push_back( pass );
		}
		else
		{
			Com_Printf( "Unknown token (%s), ignoring!\n", token.c_str() );
		}
	}

	return false;
}

bool chr::MDAModel::ParsePass( Pass &pass, std::stringstream &ss )
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

			pass.map = GL_FindImage( token.c_str(), it_skin );
		}
		else if ( token == "alphafunc" )
		{
			if ( !( ss >> token ) )
			{
				Com_Printf( "Expected type after 'alphafunc'!\n" );
				return false;
			}

			if ( token == "gt0" ) { pass.alpha = Pass::AlphaFunc::GT0; }
			else if ( token == "ge128" ) { pass.alpha = Pass::AlphaFunc::GE128; }
			else if ( token == "lt128" ) { pass.alpha = Pass::AlphaFunc::LT128; }
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

			if ( token == "none" ) { pass.rgb = Pass::RGBGen::NONE; }
			else if ( token == "identity" ) { pass.rgb = Pass::RGBGen::IDENTITY; }
			else if ( token == "diffusezero" ) { pass.rgb = Pass::RGBGen::DIFFUSE_ZERO; }
			else if ( token == "ambient" ) { pass.rgb = Pass::RGBGen::AMBIENT; }
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

			if ( token == "none" ) { pass.blend = Pass::BlendMode::NONE; }
			else if ( token == "normal" ) { pass.blend = Pass::BlendMode::NORMAL; }
			else if ( token == "multiply" ) { pass.blend = Pass::BlendMode::MULTIPLY; }
			else if ( token == "add" ) { pass.blend = Pass::BlendMode::ADD; }
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

			if ( token == "none" ) { pass.cull = Pass::CullMode::NONE; }
			else if ( token == "front" ) { pass.cull = Pass::CullMode::FRONT; }
			else if ( token == "back" ) { pass.cull = Pass::CullMode::BACK; }
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

			if ( token == "sphere" ) { pass.uvgen = Pass::UVGen::SPHERE; }
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
				pass.uvMod = Pass::UVMod::SCROLL;
				if ( !( ss >> pass.uvModScroll.x >> pass.uvModScroll.y ) )
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

			if ( token == "none" ) { pass.depth = Pass::DepthFunc::NONE; }
			else if ( token == "equal" ) { pass.depth = Pass::DepthFunc::EQUAL; }
			else if ( token == "less" ) { pass.depth = Pass::DepthFunc::LESS; }
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

			if ( token == "1" ) { pass.depthWrite = true; }
			else if ( token == "0" ) { pass.depthWrite = false; }
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
