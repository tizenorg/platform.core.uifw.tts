SET(SRCS 
	tts.c
	tts_client.c
	tts_dbus.c
	../common/tts_config_mgr.c
	../common/tts_config_parser.c
)

SET(SETTING_SRCS
	tts_setting.c
	../common/tts_config_mgr.c
	../common/tts_config_parser.c
)


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/common)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

## Add definitions ##
ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")

## TTS library ##
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

## TTS-setting library ##
ADD_LIBRARY("${PROJECT_NAME}_setting" SHARED ${SETTING_SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}_setting" ${pkgs_LDFLAGS})

## Install library files ##
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)
INSTALL(TARGETS ${PROJECT_NAME}_setting DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)
