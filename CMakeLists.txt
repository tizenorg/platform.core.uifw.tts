# Copyright (c) 2011-2014 Samsung Electronics Co., Ltd All Rights Reserved
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
#
# @file        CMakeLists.txt
# @author      Dongyeol Lee (dy3.lee@samsung.com)
# @version     0.1
# @brief

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(tts)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "${PREFIX}")
SET(VERSION 0.2.41)

IF (LIBDIR MATCHES "/usr/lib64")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLIB64")
ENDIF (LIBDIR)

## Include common directory ##
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/common")

## Dependent packages ##
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED  
	aul capi-media-audio-io capi-system-info dbus-1 dlog ecore 
	glib-2.0 libtzplatform-config libxml-2.0 vconf
)

pkg_check_modules(pkgs_test REQUIRED
    ecore dlog
)

## API ##
ADD_SUBDIRECTORY(include)

## Client library ##
ADD_SUBDIRECTORY(client)

## Server daemon ##
ADD_SUBDIRECTORY(server)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE RENAME tts DESTINATION /usr/share/license)

## Config ##
INSTALL(FILES ${CMAKE_SOURCE_DIR}/tts-config.xml DESTINATION ${LIBDIR}/voice/tts/1.0)

INSTALL(FILES ${CMAKE_SOURCE_DIR}/tts-server.conf DESTINATION /etc/dbus-1/system.d)

## Dbus service ##
INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.voice.ttsserver.service DESTINATION /usr/share/dbus-1/system-services)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.voice.ttsnotiserver.service DESTINATION /usr/share/dbus-1/system-services)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.voice.ttssrserver.service DESTINATION /usr/share/dbus-1/system-services)

## Sample application ##
ADD_SUBDIRECTORY(test)
