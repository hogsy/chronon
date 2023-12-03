// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

namespace chr::renderer
{
	class IShaderProgram
	{
	public:
		enum Stage
		{
			STAGE_VERTEX,
			STAGE_PIXEL,

			MAX_STAGES
		};
		virtual bool LoadShaderStage( const std::string &path, Stage stage ) = 0;

		virtual void Enable() = 0;
		virtual void Disable() = 0;

		virtual void Reload() = 0;
	};

	class IShaderManager
	{
	public:
		void SetupDefaultPrograms();

		IShaderProgram         *FetchProgram( const std::string &name );
		virtual IShaderProgram *LoadProgram( const std::string &name, const std::string &vertexPath, const std::string &pixelPath ) = 0;

	private:
		std::map< std::string, IShaderProgram * > programs;
	};

	chr::renderer::IShaderProgram *InitializeShaderManager();
}// namespace nox::renderer
