set (PROJECT_NAME crow)
PROJECT(${PROJECT_NAME})

CMAKE_MINIMUM_REQUIRED(VERSION 2.7)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF) # want c++11 not gnu++11

set(CMAKE_SUPPRESS_REGENERATION "1") # avoid ALL_BUILD targets

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG=1")

include_directories(./dyno/include)

if($ENV{MAKE_TESTS})
  file(GLOB TESTSRCS "tests/*.c*")
  file(GLOB HDRS "include/*.h*" "include/crow/*.h*" "include/crow/private/*.h*")

  ADD_EXECUTABLE(crowtests ${TESTSRCS} ${HDRS})
  TARGET_LINK_LIBRARIES(crowtests gtest)
  TARGET_LINK_LIBRARIES(crowtests pthread)
else()
  message("===================================")
  message("NOTE: This is a header-only library")
  message("===================================")
endif()

#install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")
