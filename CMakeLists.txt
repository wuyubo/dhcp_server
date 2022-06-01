CMAKE_MINIMUM_REQUIRED (VERSION 3.6)
PROJECT (poe_dhcp_server)
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/Debug/lib)
message(STATUS ${LIB_DIR})

ADD_DEFINITIONS(-DDEBUG)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(LIB_POCO CONAN_PKG::Poco)

file(REMOVE ${CMAKE_SOURCE_DIR}/conanfile.txt)
execute_process(COMMAND python "${CMAKE_SOURCE_DIR}/conanfile.py" ${CONAN_PARAMS})
file(REMOVE ${CMAKE_BINARY_DIR}/conan.cmake)
file(DOWNLOAD https://artifactory.gz.cvte.cn/artifactory/binaries/1602/buildtool/master.zip!/conan.cmake ${CMAKE_BINARY_DIR}/conan.cmake)

include(${CMAKE_BINARY_DIR}/conan.cmake)

include_directories(src/network_adapter)
include_directories(src/dhcp_server)
include_directories(src/log)
include_directories(api/inc)

add_subdirectory(src/network_adapter)
add_subdirectory(src/log)
add_subdirectory(src/dhcp_server)


list(APPEND API_SRC
            ${CMAKE_SOURCE_DIR}/version.h
            ${CMAKE_SOURCE_DIR}/api/inc/api_poe_dhcp_server.h
            ${CMAKE_SOURCE_DIR}/api/src/poe_dhcp_server_impl.h
            ${CMAKE_SOURCE_DIR}/api/src/poe_dhcp_server_impl.cc
)

SOURCE_GROUP("API Files" FILES ${API_SRC})

#set(LIB_TYPE SHARED)
#if(DEBUG)
set(LIB_TYPE STATIC)
#endif()

add_library(poe_dhcp_server ${LIB_TYPE} ${API_SRC})
target_link_libraries(poe_dhcp_server network_adapter dhcp_server log)
set_target_properties(poe_dhcp_server PROPERTIES FOLDER "api")

add_subdirectory(demo)
