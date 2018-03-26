set (PROJECT_NAME crow)
PROJECT(${PROJECT_NAME})

CMAKE_MINIMUM_REQUIRED(VERSION 2.7)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG=1")

file(GLOB HDRS "src/*.h" "include/*.h")
file(GLOB SRCS "src/*.cpp" "src/*.cc")

add_library (${PROJECT_NAME} ${SRCS} ${HDRS})

if($ENV{MAKE_TESTS})
  file(GLOB TESTSRCS "tests/*.c*")

  ADD_EXECUTABLE(crowtests ${TESTSRCS})
  TARGET_LINK_LIBRARIES(crowtests crow gtest)
endif()

install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")
