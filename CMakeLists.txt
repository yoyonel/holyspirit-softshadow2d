#Change this if you need to target a specific CMake version
cmake_minimum_required(VERSION 2.6)
project(light_test)

# Enable debug symbols by default
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()
# (you can also set it on the command line: -D CMAKE_BUILD_TYPE=Release)

# Set version information in a config.h file
set(myproject_VERSION_MAJOR 1)
set(myproject_VERSION_MINOR 0)
configure_file(
  "${PROJECT_SOURCE_DIR}/src/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )
include_directories("${PROJECT_BINARY_DIR}")

# Define sources and executable
set(EXECUTABLE_NAME "light_test")

AUX_SOURCE_DIRECTORY(src SRCFILES)

MESSAGE(SRCFILES: ${SRCFILES})

include_directories(src)
#include_directories("/home/latty/__DEV__/__LOISIRS__/sfml2_custom/src")

add_executable(
	${EXECUTABLE_NAME} 
	${SRCFILES}
	)


# Detect and add SFML
# local [MOG]
set(SFMLDIR "/home/latty/__DEV__/__LOISIRS__/install/sfml2_custom")
MESSAGE([LOCAL] - SFMLDIR: ${SFMLDIR})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
#MESSAGE(CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH})
#Find any version 2.X of SFML
#See the FindSFML.cmake file for additional details and instructions
find_package(SFML 2 REQUIRED system window graphics network audio)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(${EXECUTABLE_NAME} ${SFML_LIBRARIES})
endif()

# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
if (OPENGL_FOUND)
  target_link_libraries(${EXECUTABLE_NAME} ${OPENGL_LIBRARIES})
  # or: target_link_libraries(${EXECUTABLE_NAME} ${OPENGL_gl_LIBRARY})
  target_link_libraries(${EXECUTABLE_NAME} m)  # if you use maths.h
endif()

# OpenGL
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRSq})
if (GLEW_FOUND)
  target_link_libraries(${EXECUTABLE_NAME} ${GLEW_LIBRARIES})
endif()

# Install target
install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)


# CPack packaging
include(InstallRequiredSystemLibraries)
IF(EXISTS "COPYING")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
endif()
set(CPACK_PACKAGE_VERSION_MAJOR "${myproject_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${myproject_VERSION_MINOR}")
include(CPack)