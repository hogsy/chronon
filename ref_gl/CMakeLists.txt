#[[
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
]]

project(ref_gl)

file(GLOB REFGL_SOURCE_FILES
        gl_draw.cpp
        gl_image.cpp
        gl_light.cpp
        gl_mesh.cpp
        gl_model.cpp
        gl_rmain.cpp
        gl_rmisc.cpp
        gl_rsurf.cpp
        gl_warp.cpp
        qgl.h
        ../game/q_shared.cpp
        )

add_library(ref_gl STATIC ${REFGL_SOURCE_FILES})

if (WIN32 OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    build_glew()
    target_link_libraries(ref_gl glew_s)
else()
    target_link_libraries(ref_gl GLEW GL)
endif()

target_include_directories(ref_gl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/glew/include/)

set_target_properties(ref_gl
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/"
        PREFIX ""
        )
