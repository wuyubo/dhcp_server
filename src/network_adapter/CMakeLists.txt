CMAKE_MINIMUM_REQUIRED (VERSION 3.6)

list(APPEND NETWORK_ADAPTER_SRCS
	network_adapter.h
                network_adapter.cc
                net_config.h
                net_config.cc)

add_library(network_adapter ${NETWORK_ADAPTER_SRCS})
target_link_libraries(network_adapter ${LIB_POCO})
set_target_properties(network_adapter PROPERTIES FOLDER "network_adapter")