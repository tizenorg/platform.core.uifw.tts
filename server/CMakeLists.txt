SET(SRCS 
	ttsd_config.c
	ttsd_data.cpp
	ttsd_dbus.c
	ttsd_dbus_server.c
	ttsd_engine_agent.c
	ttse.c
	ttsd_network.c
	ttsd_player.c
	ttsd_server.c
	../common/tts_config_mgr.c
	../common/tts_config_parser.c
)
	
SET(NOTI_SRCS
	ttsd_config.c
	ttsd_data.cpp
	ttsd_dbus.c
	ttsd_dbus_server.c
	ttsd_engine_agent.c
	ttsd_main_noti.c
	ttsd_network.c
	ttsd_player.c
	ttsd_server.c
	../common/tts_config_mgr.c
	../common/tts_config_parser.c
)	

SET(SR_SRCS
 	ttsd_config.c
	ttsd_data.cpp
	ttsd_dbus.c
	ttsd_dbus_server.c
	ttsd_engine_agent.c
	ttsd_main_sr.c
	ttsd_network.c
	ttsd_player.c
	ttsd_server.c
	../common/tts_config_mgr.c
	../common/tts_config_parser.c
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/common)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIE")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIE")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -fPIE")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -fPIE")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

## Executable ##
ADD_LIBRARY("${PROJECT_NAME}_engine" SHARED ${SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}_engine" -ldl ${pkgs_LDFLAGS})

## Install ##
INSTALL(TARGETS "${PROJECT_NAME}_engine" DESTINATION lib COMPONENT RuntimeLibraries)
