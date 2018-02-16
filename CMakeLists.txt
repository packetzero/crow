set (PROJECT_NAME simplepb)
PROJECT(${PROJECT_NAME})

CMAKE_MINIMUM_REQUIRED(VERSION 2.7)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG=1")

if($ENV{CONAN})
  INCLUDE(conanbuildinfo.cmake)
  CONAN_BASIC_SETUP()
endif()

file(GLOB HDRS "src/*.h" "include/*.h")
file(GLOB SRCS "src/*.cpp" "src/*.cc")

add_library (${PROJECT_NAME} ${SRCS} ${HDRS})

#include_directories('.deplibs')
#link_directories('.deblibs')

file(GLOB TESTSRCS "tests/*.c*")

if($ENV{CONAN})
  ADD_EXECUTABLE(simplepbtests ${TESTSRCS})
  TARGET_LINK_LIBRARIES(simplepbtests simplepb ${CONAN_LIBS} )
endif()
