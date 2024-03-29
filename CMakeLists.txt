cmake_minimum_required(VERSION 3.0.0)
project(GphotoDeamon VERSION 0.1.0)

include(CTest)
enable_testing()

set(SRC_DIR src)

set(GPHOTO_WRAPPER_SRC
	file.cpp
	list.cpp
	camera.cpp
	camera_storage.cpp
	port_info.cpp
	camera_abilities.cpp
)

list(TRANSFORM GPHOTO_WRAPPER_SRC PREPEND gphoto_wrapper/)

set(SROURCE_FILES 
	main.cpp
	gphoto.cpp
	named_pipe.cpp
	daemon_config.cpp
	linux_error.cpp
	${GPHOTO_WRAPPER_SRC}
)

list(TRANSFORM SROURCE_FILES PREPEND ${SRC_DIR}/)

add_executable(GphotoDeamon ${SROURCE_FILES})

include_directories(include libgphoto2)
include_directories(compile-time-regular-expressions/include)

target_link_libraries(GphotoDeamon gphoto2 gphoto2_port pthread)
set_property(TARGET GphotoDeamon PROPERTY CXX_STANDARD 20)
set_property(TARGET GphotoDeamon PROPERTY C_STANDARD 20)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

