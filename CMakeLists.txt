cmake_minimum_required(VERSION 2.8)

project(WaterSurface)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src/)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include/)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/lib/)

set(SRC_SHADER 
    ${SRC_DIR}shaders/simple.vert
    ${SRC_DIR}shaders/simple.frag
    ${SRC_DIR}shaders/light_cube.vert
    ${SRC_DIR}shaders/light_cube.frag
    ${SRC_DIR}shaders/water_surface.vert
    ${SRC_DIR}shaders/water_surface.frag
    ${SRC_DIR}shaders/sky_box.vert
    ${SRC_DIR}shaders/sky_box.frag
    ${SRC_DIR}shaders/water_heightMap.vert
    ${SRC_DIR}shaders/water_heightMap.frag
    ${SRC_DIR}shaders/sub_screen.vert
    ${SRC_DIR}shaders/sub_screen.frag
    ${SRC_DIR}shaders/main_screen.vert
    ${SRC_DIR}shaders/main_screen.frag
    ${SRC_DIR}shaders/color_uv.vert
    ${SRC_DIR}shaders/color_uv.frag
    ${SRC_DIR}shaders/interactive_heightmap.vert
    ${SRC_DIR}shaders/interactive_heightmap.frag
    ${SRC_DIR}shaders/standard_shader.vert
    ${SRC_DIR}shaders/standard_shader.frag
    ${SRC_DIR}shaders/Particle.vert
    ${SRC_DIR}shaders/Particle.frag
    ${SRC_DIR}shaders/normal_mapping.vert
    ${SRC_DIR}shaders/normal_mapping.frag
    ${SRC_DIR}shaders/parallax_mapping.vert
    ${SRC_DIR}shaders/parallax_mapping.frag
    ${SRC_DIR}shaders/text.vert
    ${SRC_DIR}shaders/text.frag)

set(SRC_RENDER_UTILITIES
    ${SRC_DIR}RenderUtilities/BufferObject.h
    ${SRC_DIR}RenderUtilities/Shader.h
    ${SRC_DIR}RenderUtilities/Texture.h)

include_directories(${INCLUDE_DIR})
include_directories(${INCLUDE_DIR}glad4.6/include/)
include_directories(${INCLUDE_DIR}glm-0.9.8.5/glm/)
include_directories(${INCLUDE_DIR}glfw-3.1.2/include)

configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

# first create relevant static libraries requried for other projects
add_library(STB_IMAGE "src/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

add_Definitions("-D_XKEYCHECK_H")

add_executable(WaterSurface
    ${SRC_DIR}CallBacks.h
    ${SRC_DIR}ControlPoint.h
    ${SRC_DIR}Object.h
    ${SRC_DIR}Track.h
    ${SRC_DIR}TrainView.h
    ${SRC_DIR}TrainWindow.h
    ${SRC_DIR}WaterMesh.h
    ${SRC_DIR}SkyBox.h
    ${SRC_DIR}FrameBuffer.h
    ${SRC_DIR}My_functions.h
    ${SRC_DIR}Particle_system.h
    ${SRC_DIR}TextRendering.h

    ${SRC_DIR}main.cpp
    ${SRC_DIR}CallBacks.cpp
    ${SRC_DIR}ControlPoint.cpp
    ${SRC_DIR}Track.cpp
    ${SRC_DIR}TrainView.cpp
    ${SRC_DIR}TrainWindow.cpp
    ${SRC_DIR}WaterMesh.cpp
    ${SRC_DIR}SkyBox.cpp
    ${SRC_DIR}FrameBuffer.cpp
    ${SRC_DIR}Particle_system.cpp
    ${SRC_DIR}TextRendering.cpp

    ${SRC_SHADER}
    ${SRC_RENDER_UTILITIES}

    ${INCLUDE_DIR}glad4.6/src/glad.c
)
source_group("shaders" FILES ${SRC_SHADER})
source_group("RenderUtilities" FILES ${SRC_RENDER_UTILITIES})


add_library(Utilities 
    ${SRC_DIR}Utilities/ArcBallCam.h
    ${SRC_DIR}Utilities/3DUtils.h
    ${SRC_DIR}Utilities/Pnt3f.h
    ${SRC_DIR}Utilities/ArcBallCam.cpp
    ${SRC_DIR}Utilities/3DUtils.cpp
    ${SRC_DIR}Utilities/Pnt3f.cpp
    ${SRC_DIR}Utilities/objloader.hpp
    ${SRC_DIR}Utilities/objloader.cpp
    ${SRC_DIR}Utilities/Matrices.h
    ${SRC_DIR}Utilities/Matrices.cpp
    ${SRC_DIR}Utilities/Vectors.h)

add_library(OpenGL_tutorial
    ${SRC_DIR}openGL_tutorial/texture.hpp
    ${SRC_DIR}openGL_tutorial/texture.cpp
)

target_link_libraries(WaterSurface 
    debug ${LIB_DIR}Debug/fltk_formsd.lib      optimized ${LIB_DIR}Release/fltk_forms.lib
    debug ${LIB_DIR}Debug/fltk_gld.lib         optimized ${LIB_DIR}Release/fltk_gl.lib
    debug ${LIB_DIR}Debug/fltk_imagesd.lib     optimized ${LIB_DIR}Release/fltk_images.lib
    debug ${LIB_DIR}Debug/fltk_jpegd.lib       optimized ${LIB_DIR}Release/fltk_jpeg.lib
    debug ${LIB_DIR}Debug/fltk_pngd.lib        optimized ${LIB_DIR}Release/fltk_png.lib
    debug ${LIB_DIR}Debug/fltk_zd.lib          optimized ${LIB_DIR}Release/fltk_z.lib
    debug ${LIB_DIR}Debug/fltkd.lib            optimized ${LIB_DIR}Release/fltk.lib
    debug ${LIB_DIR}Debug/opencv_world341d.lib optimized ${LIB_DIR}Release/opencv_world341.lib
    debug ${LIB_DIR}Debug/freeglutd.lib        optimized ${LIB_DIR}Release/freeglut.lib
    debug ${LIB_DIR}glfw3.lib                  optimized ${LIB_DIR}glfw3.lib
    debug ${LIB_DIR}GLEW_1130.lib              optimized ${LIB_DIR}GLEW_1130.lib
    debug ${LIB_DIR}freetype.lib               optimized ${LIB_DIR}freetype.lib)

target_link_libraries(WaterSurface 
    ${LIB_DIR}OpenGL32.lib
    ${LIB_DIR}glu32.lib
    ${LIB_DIR}common.lib
    ${LIB_DIR}ex-common.lib
    ${LIB_DIR}OpenAL32.lib
    ${LIB_DIR}alut.lib
    ${LIB_DIR}alut_static.lib
    ${LIB_DIR}assimp.lib
    ${LIB_DIR}STB_IMAGE.lib)

target_link_libraries(WaterSurface Utilities)
    