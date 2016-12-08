cmake_minimum_required(VERSION 2.8.3)

project(Phase2)

find_package (Threads)


source_group("Headers" REGULAR_EXPRESSION ".*\\.h$")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(
	.
	)

#add here sources that are common
SET(SRC_COMMONS
	dsm.c
	common.c
)


#add here source files you need to compile
SET(SRC_DSMEXEC
	dsmexec.c
)

add_executable(dsmexec ${SRC_DSMEXEC} ${SRC_COMMONS})
target_link_libraries (dsmexec ${CMAKE_THREAD_LIBS_INIT})


#add here source files you need to compile
SET(SRC_DSMWRAP
	dsmwrap.c
)

add_executable(dsmwrap ${SRC_DSMWRAP} ${SRC_COMMONS})
target_link_libraries (dsmwrap ${CMAKE_THREAD_LIBS_INIT})


#add here source files you need to compile
SET(SRC_TEST
	test.c
)

add_executable(test ${SRC_DSMWRAP} ${SRC_COMMONS})
target_link_libraries (test ${CMAKE_THREAD_LIBS_INIT})


