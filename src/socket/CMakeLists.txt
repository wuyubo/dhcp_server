CMAKE_MINIMUM_REQUIRED (VERSION 3.6)

list(APPEND SOCKET_SRCS
                datagram_socket.h
                datagram_socket.cc)

add_library(socket ${SOCKET_SRCS})
set_target_properties(socket PROPERTIES FOLDER "socket")