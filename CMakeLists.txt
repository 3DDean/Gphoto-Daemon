cmake_minimum_required(VERSION 3.0.0)
project(GphotoDeamon VERSION 0.1.0)

include(CTest)
enable_testing()

set(SRC_DIR src)


set(SROURCE_FILES main.cpp gphoto.cpp named_pipe.cpp)

list(TRANSFORM SROURCE_FILES PREPEND ${SRC_DIR}/)

add_executable(GphotoDeamon ${SROURCE_FILES})

include_directories(include libgphoto2)
include_directories(compile-time-regular-expressions/include)

target_link_libraries(GphotoDeamon gphoto2 gphoto2_port)

set(SROURCE_FILES_C main.c GPhoto2D.c GPhoto2DUtils.c GPhoto2DCmd.c)

list(TRANSFORM SROURCE_FILES_C PREPEND ${SRC_DIR}/C_files/)

add_executable(GphotoDeamonC ${SROURCE_FILES_C})
include_directories(include libgphoto2/gphoto2)

target_link_libraries(GphotoDeamonC gphoto2 gphoto2_port)

set_property(TARGET GphotoDeamonC PROPERTY C_STANDARD 99)

set_property(TARGET GphotoDeamon PROPERTY CXX_STANDARD 20)
set_property(TARGET GphotoDeamon PROPERTY C_STANDARD 20)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

