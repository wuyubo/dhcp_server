CMAKE_MINIMUM_REQUIRED (VERSION 3.6)

set(demo_project "device_search_demo")

set(DEVICE_SEARCH_LIB device_search)

list(APPEND DEMO_SRC
                main.cc)

add_executable(${demo_project} ${DEMO_SRC})

target_link_libraries(${demo_project}  poe_dhcp_server)
add_dependencies(${demo_project}   poe_dhcp_server)
