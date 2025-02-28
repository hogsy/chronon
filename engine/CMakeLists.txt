#[[
Copyright (C) 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

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
]]

if (WIN32)
    file(GLOB ENGINE_PLATFORM_SOURCE
            win32/cd_win.cpp
            win32/net_wins.cpp
            win32/q_shwin.cpp
            win32/snd_win.cpp
    )
    link_directories(../3rdparty/sdl2/lib/)
elseif (APPLE)
    file(GLOB ENGINE_PLATFORM_SOURCE
            null/cd_null.cpp
            null/snd_null.cpp
            linux/glob.cpp
            linux/net_udp.cpp
            linux/q_shlinux.cpp
    )
elseif (UNIX)
    file(GLOB ENGINE_PLATFORM_SOURCE
            linux/cd_linux.cpp
            linux/glob.cpp
            linux/net_udp.cpp
            linux/q_shlinux.cpp
            linux/snd_linux.cpp
    )
endif ()

add_executable(chronon-engine WIN32
        app.cpp
        hunk.cpp
        input.cpp
        system.cpp

        # Client
        client/cl_cin.cpp
        client/cl_ents.cpp
        client/cl_fx.cpp
        client/cl_input.cpp
        client/cl_inv.cpp
        client/cl_main.cpp
        client/cl_newfx.cpp
        client/cl_parse.cpp
        client/cl_pred.cpp
        client/cl_scrn.cpp
        client/cl_tent.cpp
        client/cl_view.cpp
        client/console.cpp
        client/keys.cpp
        client/menu.cpp
        client/qmenu.cpp
        client/snd_dma.cpp
        client/snd_mem.cpp
        client/snd_mix.cpp
        client/vid_menu.cpp
        ../game/m_flash.cpp
        ../game/q_shared.cpp
        # Common
        ../qcommon/cmd.cpp
        ../qcommon/cmodel.cpp
        ../qcommon/common.cpp
        ../qcommon/crc.cpp
        ../qcommon/cvar.cpp
        ../qcommon/files.cpp
        ../qcommon/md4.cpp
        ../qcommon/net_chan.cpp
        ../qcommon/pmove.cpp

        model/model_alias.cpp
        model/model_mda.cpp

        # Renderer
        # -- gl
        renderer/ref_gl/gl_draw.cpp
        renderer/ref_gl/gl_image.cpp
        renderer/ref_gl/gl_model.cpp
        renderer/ref_gl/gl_rmain.cpp
        renderer/ref_gl/gl_rmisc.cpp
        renderer/ref_gl/gl_rsurf.cpp
        renderer/ref_gl/gl_shader_manager.cpp
        renderer/ref_gl/gl_warp.cpp
        renderer/ref_gl/qgl.cpp
        # -- generic
        renderer/light.cpp
        renderer/material.cpp
        renderer/shader_manager.cpp

        # Server
        server/sv_ccmds.cpp
        server/sv_ents.cpp
        server/sv_game.cpp
        server/sv_init.cpp
        server/sv_main.cpp
        server/sv_send.cpp
        server/sv_user.cpp
        server/sv_world.cpp

        # Platform
        sdl2/sdl2_sys.cpp
        sdl2/sdl2_vid.cpp
        ${ENGINE_PLATFORM_SOURCE}

        # APE Virtual Machine
        ape/ape.cpp

        # 3rd Party
        ../3rdparty/miniz/miniz.c
        ../3rdparty/glew/src/glew.c
)

set_target_properties(chronon-engine PROPERTIES OUTPUT_NAME Chronon)

target_compile_definitions(chronon-engine PRIVATE
        GLEW_STATIC
)
target_include_directories(chronon-engine PRIVATE
        ./
        ../
        ../3rdparty/
        ../3rdparty/glew/include/
)

target_link_libraries(chronon-engine chronon-game)

find_package(SDL2 CONFIG)
if (${SDL2_FOUND})
    target_link_libraries(chronon-engine ${SDL2_LIBRARIES})
else ()
    message("Linking against SDL2 in repository.")
    target_include_directories(chronon-engine PRIVATE ../3rdparty/sdl2/include/)
    target_link_directories(chronon-engine PRIVATE ../3rdparty/sdl2/lib/)
    target_link_libraries(chronon-engine SDL2)
endif ()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
target_link_libraries(chronon-engine ${OPENGL_LIBRARIES})

if (WIN32)
    target_link_libraries(chronon-engine
            mingw32
            Imm32
            Setupapi
            Version
            Ws2_32
            Winmm
    )
elseif (UNIX AND NOT APPLE)
    target_link_libraries(chronon-engine X11 Xext dl)
endif ()
