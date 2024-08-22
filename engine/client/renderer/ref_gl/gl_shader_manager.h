// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "../shader_manager.h"

namespace chr::renderer::gl
{
	class ShaderProgram : public IShaderProgram
	{
	public:
		ShaderProgram();
		~ShaderProgram();

		bool LoadShaderStage( const std::string &path, Stage stage );

		void Enable() override;
		void Disable() override;

		void Reload() override;

	protected:
	private:
		uint32_t glProgram{ 0 };
		uint32_t glStages[ ( uint32_t ) Stage::MAX_STAGES ];

		std::string fragmentPath;
		std::string vertexPath;
	};

	class ShaderManager : public IShaderManager
	{
	public:
		ShaderManager() {}
		~ShaderManager() {}


	protected:
	private:
	};
}// namespace chr::renderer::gl
