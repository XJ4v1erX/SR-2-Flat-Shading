cmake_minimum_required(VERSION 3.25)
project(SR_2_Flat_Shading)

set(CMAKE_CXX_STANDARD 17)
set(SDL2_INCLUDE_DIR C:/Users/javie/Documents/SDL2-devel-2.28.1-VC/SDL2-2.28.1/include)
set(SDL2_LIB_DIR C:/Users/javie/Documents/SDL2-devel-2.28.1-VC/SDL2-2.28.1/lib/x64)

include_directories(${SDL2_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})

add_executable(SR_2_Flat_Shading main.cpp GraphicsStructures.h ShaderUtilities.h ObjLoader.h)

target_link_libraries(${PROJECT_NAME} SDL2main SDL2)