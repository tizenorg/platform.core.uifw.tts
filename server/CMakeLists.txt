CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(tts-daemon)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "${PREFIX}")
SET(LIBDIR "${PREFIX}/lib")
SET(INCLUDEDIR "${PREFIX}/include")
SET(VERSION 0.0.1)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "${PREFIX}")
SET(LIBDIR "${PREFIX}/lib/voice/tts/1.0/engine")
SET(INCLUDEDIR "${PREFIX}/include")
SET(VERSION 0.0.1)

SET(SRCS 
	ttsd_data.cpp
	ttsd_player.c
	ttsd_engine_agent.c
	ttsd_config.c
	ttsd_server.c
	ttsd_network.c
	ttsd_dbus.c
	ttsd_dbus_server.c
	ttsd_main.c
)
	
SET(NOTI_SRCS
    ttsd_data.cpp
	ttsd_player.c
	ttsd_engine_agent.c
	ttsd_config_noti.c
	ttsd_server.c
	ttsd_network.c
	ttsd_dbus.c
	ttsd_dbus_server.c
	ttsd_main_noti.c
)	

SET(SR_SRCS
    ttsd_data.cpp
	ttsd_player.c
	ttsd_engine_agent.c
	ttsd_config_sr.c
	ttsd_server.c
	ttsd_network.c
	ttsd_dbus.c
	ttsd_dbus_server.c
	ttsd_main_sr.c
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${INCLUDEDIR})	
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/../common")

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_RELEASE "-O2")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

## Add definitions ##
ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DVENDOR=\"${VENDOR}\"")
ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE}\"")
ADD_DEFINITIONS("-DPACKAGE_NAME=\"${PKGNAME}\"")
ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")
ADD_DEFINITIONS("-DEDJDIR=\"${EDJDIR}\"")
ADD_DEFINITIONS("-DIMAGEDIR=\"${IMAGEDIR}\"")

## Executable ##
ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -ldl ${pkgs_LDFLAGS})

ADD_EXECUTABLE("${PROJECT_NAME}-noti" ${NOTI_SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}-noti" -ldl ${pkgs_LDFLAGS})

ADD_EXECUTABLE("${PROJECT_NAME}-sr" ${SR_SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}-sr" -ldl ${pkgs_LDFLAGS})


## Install ##
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
INSTALL(TARGETS "${PROJECT_NAME}-noti" DESTINATION bin)
INSTALL(TARGETS "${PROJECT_NAME}-sr" DESTINATION bin)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ttsp.h DESTINATION include)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ttsd.conf DESTINATION lib/voice/tts/1.0)

