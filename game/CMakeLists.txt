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

add_library(chronon-game STATIC
        CinematicScript.cpp

        g_ai.cpp
        g_chase.cpp
        g_cmds.cpp
        g_combat.cpp
        g_func.cpp
        g_items.cpp
        g_main.cpp
        g_misc.cpp
        g_monster.cpp
        g_phys.cpp
        g_save.cpp
        g_spawn.cpp
        g_svcmds.cpp
        g_target.cpp
        g_trigger.cpp
        g_utils.cpp
        g_weapon.cpp
        m_actor.cpp
        m_flash.cpp
        m_move.cpp
        p_client.cpp
        p_hud.cpp
        p_trail.cpp
        p_view.cpp
        p_weapon.cpp
        q_shared.cpp
)

target_compile_definitions(chronon-game PRIVATE GAME_HARD_LINKED)