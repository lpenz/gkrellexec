
cmake_minimum_required (VERSION 3.0)

project (gkrellexec)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -W")
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -W")
endif(CMAKE_COMPILER_IS_GNUCXX)

find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB REQUIRED glib-2.0)
pkg_check_modules (GTK2 REQUIRED gtk+-2.0)

include_directories ("${GLIB_INCLUDE_DIRS}")
include_directories ("${GTK2_INCLUDE_DIRS}")

add_library (gkrellexec
  SHARED
  gkrellexec.c)
