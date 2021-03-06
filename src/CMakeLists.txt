PROJECT(transsip-toolkit C)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
SET(EXECUTABLE_INSTALL_PATH /usr/bin)
SET(CONFIG_INSTALL_PATH /etc/trafgen)

SET(VERSION "0.5.0")

INCLUDE(CheckStrictAlign)
INCLUDE(Pod2Man)

ADD_DEFINITIONS(
	-O3
	-fstack-protector
	-fpie
	-std=gnu99
	-fno-strict-aliasing
	-D_FORTIFY_SOURCE=2
	-D_REENTRANT
	-march=native
	-mtune=native
	-Wall
	-Wundef
	-Wstrict-prototypes
	-Werror-implicit-function-declaration
	-Wformat=2
	-Wcomments
	-Wendif-labels
	-Wuninitialized
	-Winit-self
	-Wstrict-overflow=1
	-Wtype-limits
	-Wmissing-parameter-type
	-Wclobbered
	-Wempty-body
	-Wignored-qualifiers
	-Wmissing-include-dirs
	-Woverlength-strings
	-Wmissing-field-initializers
	-Wmissing-declarations
	-Wmissing-prototypes
	-Wlogical-op
	-Woverride-init
	-Wnested-externs
	-Wredundant-decls
	-Winvalid-pch
	-Wdisabled-optimization
	-Wstack-protector
	-Waggregate-return
	-Wcast-align
	-Wunsafe-loop-optimizations
	-Wfloat-equal
	-Wunknown-pragmas
	-Wunused-function
)

IF (CMAKE_BUILD_TYPE STREQUAL profile)
	ADD_DEFINITIONS(-g -pg)
ENDIF (CMAKE_BUILD_TYPE STREQUAL profile)

ENABLE_TESTING()

ADD_MANPAGE_TARGET()

INCLUDE_DIRECTORIES(.)

ADD_SUBDIRECTORY(transsip)
