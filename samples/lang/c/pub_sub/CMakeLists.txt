cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(aws-iot-cpp-lang-c-samples C)

######################################
# Section : Disable in-source builds #
######################################

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt and CMakeFiles folder." )
endif()

########################################
# Section : Common Build setttings #
########################################
# Set required compiler standard to standard c++11. Disable extensions.
set(CMAKE_CXX_STANDARD 11) # C++11...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/archive)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Configure Compiler flags
if(UNIX AND NOT APPLE)
	# Prefer pthread if found
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	set(CUSTOM_COMPILER_FLAGS "-fno-exceptions -Wall -Werror")
elseif(APPLE)
	set(CUSTOM_COMPILER_FLAGS "-fno-exceptions")
elseif(WIN32)
	set(CUSTOM_COMPILER_FLAGS "/W4")
endif()

################################
# Target : Build pub_sub_c sample #
################################
set(PUB_SUB_C_SAMPLE_TARGET_NAME pub-sub-c-sample)
# Add Target
add_executable(${PUB_SUB_C_SAMPLE_TARGET_NAME} "${CMAKE_SOURCE_DIR}/samples/lang/c/pub_sub/pub_sub.c")

# Configure Threading library
find_package(Threads REQUIRED)

# Add SDK includes
include_directories(${PUB_SUB_C_SAMPLE_TARGET_NAME} ${CMAKE_SOURCE_DIR}/include/lang/c)

set(AWS_IOT_CPP_C_LANG_TARGET_NAME aws-iot-sdk-cpp-c)
target_link_libraries(${PUB_SUB_C_SAMPLE_TARGET_NAME} PUBLIC ${AWS_IOT_CPP_C_LANG_TARGET_NAME})
#target_link_libraries(${PUB_SUB_C_SAMPLE_TARGET_NAME} "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libaws-iot-sdk-cpp-c.so")

set_property(TARGET ${PUB_SUB_C_SAMPLE_TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS ${CUSTOM_COMPILER_FLAGS})

#if(MSVC)
#	target_sources(${PUB_SUB_SAMPLE_TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/common/ConfigCommon.hpp)
#	source_group("Header Files\\Samples\\PubSub" FILES ${CMAKE_SOURCE_DIR}/common/ConfigCommon.hpp)
#	source_group("Source Files\\Samples\\PubSub" FILES ${CMAKE_SOURCE_DIR}/common/ConfigCommon.cpp)
#
#	target_sources(${PUB_SUB_SAMPLE_TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/samples/PubSub/PubSub.hpp)
#	source_group("Header Files\\Samples\\PubSub" FILES ${CMAKE_SOURCE_DIR}/samples/PubSub/PubSub.hpp)
#	source_group("Source Files\\Samples\\PubSub" FILES ${CMAKE_SOURCE_DIR}/samples/PubSub/PubSub.cpp)
#endif()
