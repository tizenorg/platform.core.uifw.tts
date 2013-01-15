CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(libtts)

## Include common directory ##
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/common")

## Dependent packages ##
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED  
	mm-player vconf dlog ecore ecore-file dbus-1 
)

## Client library ##
ADD_SUBDIRECTORY(client)

## Server daemon ##
ADD_SUBDIRECTORY(server)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.APLv2.0 RENAME tts DESTINATION /usr/share/license)

