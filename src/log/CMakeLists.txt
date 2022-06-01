CMAKE_MINIMUM_REQUIRED (VERSION 3.6)

list(APPEND LOG_SRCS
                log.h
                log.cc)

add_library(log ${LOG_SRCS})
set_target_properties(log PROPERTIES FOLDER "log")