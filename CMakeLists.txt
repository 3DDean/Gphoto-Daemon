cmake_minimum_required(VERSION 3.0.0)
project(GphotoDeamon VERSION 0.1.0)

include(CTest)
enable_testing()

set(SRC_DIR src)


set(SROURCE_FILES main.cpp Camera.cpp
)
list(TRANSFORM SROURCE_FILES PREPEND ${SRC_DIR}/)

add_executable(GphotoDeamon ${SROURCE_FILES})
include_directories(include libgphoto2/gphoto2)

target_link_libraries(GphotoDeamon gphoto2 gphoto2_port)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
