cmake_minimum_required(VERSION 3.6.3)
project(kvsrtp C)

include(rtpFilePaths.cmake)

add_library(kvsrtp ${RTP_SOURCES})

target_include_directories(kvsrtp PUBLIC
                           ${RTP_INCLUDE_PUBLIC_DIRS})

# install header files
install(
    FILES ${RTP_INCLUDE_PUBLIC_FILES}
    DESTINATION include/kvsrtp)

# install RTP library
install(
    TARGETS kvsrtp
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")