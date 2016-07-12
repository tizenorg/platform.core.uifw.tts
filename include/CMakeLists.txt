## configure pkgconfig files ##
CONFIGURE_FILE(tts.pc.in "${PROJECT_NAME}.pc" @ONLY)
CONFIGURE_FILE(tts-setting.pc.in "${PROJECT_NAME}-setting.pc" @ONLY)
CONFIGURE_FILE(tts-engine.pc.in "${PROJECT_NAME}-engine.pc" @ONLY)

## Install header, pc files ##
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}.pc" DESTINATION ${LIBDIR}/pkgconfig)
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}-setting.pc" DESTINATION ${LIBDIR}/pkgconfig)
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}-engine.pc" DESTINATION ${LIBDIR}/pkgconfig)
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/tts.h" DESTINATION ${INCLUDEDIR})
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/tts_setting.h" DESTINATION ${INCLUDEDIR})
#INSTALL(FILES "${CMAKE_BINARY_DIR}/server/ttsp.h" DESTINATION ${INCLUDEDIR})
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/ttse.h" DESTINATION ${INCLUDEDIR})
