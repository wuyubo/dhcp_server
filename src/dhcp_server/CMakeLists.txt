CMAKE_MINIMUM_REQUIRED (VERSION 3.6)

list(APPEND DHCP_SERVER_SRCS
                dhcp_server.h
                dhcp_server.cc)

add_library(dhcp_server ${DHCP_SERVER_SRCS})
target_link_libraries(dhcp_server ${LIB_POCO})
set_target_properties(dhcp_server PROPERTIES FOLDER "hdcp_server")