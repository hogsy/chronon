#[[
Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>

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

project(engine)

if (WIN32)
    file(GLOB ENGINE_PLATFORM_SOURCE
            win32/cd_win.cpp
            win32/net_wins.cpp
            win32/q_shwin.cpp
            win32/snd_win.cpp
            sdl2/sdl2_sys.cpp
            sdl2/sdl2_vid.cpp
            )
elseif (UNIX)
    file(GLOB ENGINE_PLATFORM_SOURCE
            linux/cd_linux.cpp
            linux/glob.cpp
            linux/in_linux.cpp
            linux/net_udp.cpp
            linux/q_shlinux.cpp
            linux/snd_linux.cpp
            sdl2/sdl2_sys.cpp
            sdl2/sdl2_vid.cpp
            )
endif ()

add_executable(engine WIN32
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
        client/x86.cpp
        ../game/m_flash.cpp
        ../game/q_shared.cpp
        # Common
        ../qcommon/cmd.cpp
        ../qcommon/cmodel.cpp
        ../qcommon/common.cpp
        ../qcommon/crc.cpp
        ../qcommon/cvar.cpp
        ../qcommon/files.cpp
        ../qcommon/llist.cpp
        ../qcommon/md4.cpp
        ../qcommon/net_chan.cpp
        ../qcommon/pmove.cpp

        # Renderer
        # -- gl
        client/ref_gl/gl_draw.cpp
        client/ref_gl/gl_image.cpp
        client/ref_gl/gl_light.cpp
        client/ref_gl/gl_model.cpp
        client/ref_gl/gl_rmain.cpp
        client/ref_gl/gl_rmisc.cpp
        client/ref_gl/gl_rsurf.cpp
        client/ref_gl/gl_shader_manager.cpp
        client/ref_gl/gl_warp.cpp
        client/ref_gl/qgl.cpp
        # -- generic
        client/renderer/material.cpp

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
        ${ENGINE_PLATFORM_SOURCE}

        # APE Virtual Machine
        ape/ape.cpp

        # 3rd Party
        ../3rdparty/miniz/miniz.c
        ../3rdparty/glew/src/glew.c
        )
add_dependencies(engine game)

set_target_properties(engine PROPERTIES OUTPUT_NAME hosae)

target_compile_definitions(engine PRIVATE
        GLEW_STATIC
        )
target_link_directories(engine PRIVATE
        ../3rdparty/glew/lib/Release/x64/
        )
target_include_directories(engine PRIVATE
        ./
        ../
        ../3rdparty/
        ../3rdparty/glew/include/
        ../3rdparty/sdl2/include/
        )

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_directories(engine PRIVATE
                ../3rdparty/sdl2/lib/x64/
                )
    else ()
        target_link_directories(engine PRIVATE
                ../3rdparty/sdl2/lib/x86/
                )
    endif ()

    if (MSVC)
        target_link_options(engine PRIVATE
                /NODEFAULTLIB:libcmt.lib
                /NODEFAULTLIB:libcmtd.lib
                )
    elseif (MINGW)
        target_link_libraries(engine
                mingw32
                )
    endif ()
    target_link_libraries(engine
            SDL2main
            SDL2
            Imm32
            Setupapi
            Version
            Opengl32
            Ws2_32
            Winmm
            )
elseif (UNIX)
    target_link_libraries(engine
            dl
            GL
            SDL2
            )
endif ()
