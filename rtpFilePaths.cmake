# This file is to add source files and include directories
# into variables so that it can be reused from different repositories
# in their Cmake based build system by including this file.

# RTP library source files.
file( GLOB RTP_SOURCES
     "${CMAKE_CURRENT_LIST_DIR}/source/*.c"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/g711/*.c"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/h264/*.c"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/opus/*.c"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/vp8/*.c" )

# RTP library Public Include directories.
set( RTP_INCLUDE_PUBLIC_DIRS
     "${CMAKE_CURRENT_LIST_DIR}/source/include"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/g711/include"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/h264/include"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/opus/include"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/vp8/include" )

# RTP library public include header files.
file( GLOB RTP_INCLUDE_PUBLIC_FILES
     "${CMAKE_CURRENT_LIST_DIR}/source/include/*.h"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/g711/include/*.h"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/h264/include/*.h"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/opus/include/*.h"
     "${CMAKE_CURRENT_LIST_DIR}/codec_packetizers/vp8/include/*.h" )
