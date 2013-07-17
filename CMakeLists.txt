CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(libtts)

## Include common directory ##
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/common")

## Dependent packages ##
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED  
	mm-player dlog ecore ecore-file dbus-1 mm-session capi-system-runtime-info vconf vconf-internal-keys
)

## Client library ##
ADD_SUBDIRECTORY(client)

## Server daemon ##
ADD_SUBDIRECTORY(server)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.APLv2 RENAME tts DESTINATION /usr/share/license)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/sysinfo-tts.xml DESTINATION /etc/config)
