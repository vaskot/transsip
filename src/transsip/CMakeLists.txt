PROJECT(transsip C)

SET(BUILD_STRING "generic")
FIND_PACKAGE(Threads)
INCLUDE(Pod2Man)

IF (CMAKE_HAVE_PTHREAD_CREATE)
	ADD_EXECUTABLE(${PROJECT_NAME} 	../xmalloc.c
					../gf.c
					../alsa.c
					../engine.c
					../notifier.c
					../call_notifier.c
					../xutils.c
					../stun.c
					../cli.c
					../clicmds.c
					../transsip.c)
	ADD_DEFINITIONS(-DPROGNAME_STRING="${PROJECT_NAME}"
		-DVERSION_STRING="${VERSION}"
		-DBUILD_STRING="${BUILD_STRING}")
	POD2MAN(${CMAKE_SOURCE_DIR}/transsip.c transsip 1)
	TARGET_LINK_LIBRARIES(transsip ${CMAKE_THREAD_LIBS_INIT} -lspeexdsp -lasound -lcelt0 -lm -lreadline)
	INSTALL(FILES ../sounds/busytone.s16 ../sounds/dialtone.s16 ../sounds/ring.s16 DESTINATION ${CONFIG_INSTALL_PATH}/sounds/)
	INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${EXECUTABLE_INSTALL_PATH})
ELSE(CMAKE_HAVE_PTHREAD_CREATE)
	MESSAGE("pthread is missing on target. Skipping ${PROJECT_NAME} build.")
ENDIF(CMAKE_HAVE_PTHREAD_CREATE)
