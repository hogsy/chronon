// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "../client/client.h"

namespace chr::ape
{
	enum class OpCode
	{
		OP_STARTSWITCH = 49,
		OP_THINKSWITCH = 50,
		OP_STARTCONSOLE = 65,
		OP_BACKGROUND = 68,
		OP_FONT = 70,
		OP_SUBWINDOW = 72,
		OP_IMAGE = 73,
		OP_CAM = 77,
		OP_FINISHCONSOLE = 78,
		OP_NEXTWINDOW = 79,
		OP_TITLE = 84,
		OP_STYLE = 87,
	};

	static constexpr unsigned int MAGIC = 0x3D010000;
}// namespace nox::ape
