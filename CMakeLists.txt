cmake_minimum_required(VERSION 3.13)
project(Allie VERSION 0.1 HOMEPAGE_URL https://github.com/kyechou/allie LANGUAGES CXX)

#
# compile options
#
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Werror -O2)

#
# release/debug compile options
#
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O3")
set(CMAKE_C_FLAGS_DEBUG "-DENABLE_DEBUG -g3 -O0 -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_CXX_FLAGS_DEBUG "-DENABLE_DEBUG -g3 -O0 -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

#
# third party libraries
#
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
add_subdirectory(third_party/spdlog)
set(XXHASH_BUILD_ENABLE_INLINE_API ON)
set(XXHASH_BUILD_XXHSUM OFF)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(third_party/xxHash/cmake_unofficial EXCLUDE_FROM_ALL)
add_subdirectory(third_party/simdjson)
find_package(Threads REQUIRED)
find_package(PI REQUIRED)
find_package(gRPC REQUIRED)
find_package(PcapPlusPlus REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED
    program_options
    filesystem
)

#
# source directory and files
#
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
FILE(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)
set(P4_SRC ${SRC_DIR}/switch.p4)

#
# controller
#
add_executable(controller ${SRC_FILES})
target_include_directories(controller PRIVATE ${SRC_DIR})
target_include_directories(controller SYSTEM PRIVATE ${spdlog_INCLUDE_DIRS})
target_link_libraries(controller PRIVATE
    spdlog::spdlog
    xxHash::xxhash
    Threads::Threads
    PI::PI
    gRPC::grpc++
    PcapPlusPlus::pcap++
    simdjson
    Boost::program_options
    Boost::filesystem
)
# add_custom_target(setcap ALL
#     sudo setcap "cap_sys_admin+pe cap_net_admin+pe cap_net_raw+pe cap_sys_ptrace+pe" controller
#     DEPENDS controller)

#
# switch
#
add_custom_target(switch ALL DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/switch.json
    ${CMAKE_CURRENT_BINARY_DIR}/p4info.txt)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/switch.json
           ${CMAKE_CURRENT_BINARY_DIR}/p4info.txt
    COMMAND p4c --target bmv2 --arch v1model --std p4-16
                --p4runtime-files ${CMAKE_CURRENT_BINARY_DIR}/p4info.txt
                -o ${CMAKE_CURRENT_BINARY_DIR} ${P4_SRC}
    DEPENDS ${P4_SRC})

#
# install
#
#install(TARGETS controller DESTINATION bin)
