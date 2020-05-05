cmake_minimum_required( VERSION 2.8 )

project( lines )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif(WIN32)

find_package( OpenGL REQUIRED )
find_package( GLFW3 REQUIRED )

include_directories( "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/extern" )

add_executable( lines main.c extern/glad.c )
target_link_libraries( lines glfw3 ${OPENGL_gl_LIBRARY} )
#target_compile_options( lines PRIVATE -std=c11 -Wall )