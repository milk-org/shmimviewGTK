cmake_minimum_required (VERSION 3.5)

PROJECT("shmimviewGTK" C)



# =======================================
# VERSION
# =======================================

# Version number
set ( VERSION_MAJOR 0 )
set ( VERSION_MINOR 1 )
set ( VERSION_PATCH 01 )

message("====================================================")
message("VERSION  ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
message("====================================================")
message("PROJECT_SOURCE_DIR       = ${PROJECT_SOURCE_DIR}")

# Configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/src/Config.h.in"
  "${PROJECT_SOURCE_DIR}/src/Config.h"
  )


# =======================================
# REQUIRED PACKAGES
# =======================================

# Use the package PkgConfig to detect GTK+ headers/library files
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)



# =======================================
# PERFORMANCE ORIENTED COMPILING OPTIONS
# =======================================

# Set Release build flags
set(CMAKE_C_FLAGS_RELEASE     "-Ofast -DNDEBUG")

# Set a default build type if none was specified
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}")
endif()

message("CMAKE_BUILD_TYPE : ${CMAKE_BUILD_TYPE}")

# Adds options to the compiler command line for targets in the current directory 
# and below that are added after this command is invoked

add_compile_options(-std=gnu11)
add_compile_options(-march=native)

message("COMPILE_OPTIONS:  ${COMPILE_OPTIONS}")


# A common flag is -pipe. This flag has no effect on the generated code, 
# but it makes the compilation process faster. It tells the compiler 
# to use pipes instead of temporary files during the different stages 
# of compilation, which uses more memory. On systems with low memory, 
# GCC might get killed. In those cases do not use this flag. 
add_compile_options(-pipe)







ADD_EXECUTABLE(shmimview
src/shmimview.c src/shmimview-view.c src/shmimview-scale.c src/shmimview-process.c shmimview-ui.c)


SET(CMAKE_C_FLAGS "-g -O2 -Wall -Wextra -rdynamic")



include_directories(${GTK_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GTK_LIBRARIES})


# add ImageStreamIO
set(LIBS ${LIBS} ImageStreamIO)


target_link_libraries(shmimview ${LIBS} m)

install(TARGETS shmimview DESTINATION bin)