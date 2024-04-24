# This file is to add source files and include directories
# into variables so that it can be reused from different repositories
# in their Cmake based build system by including this file.

# RTP library source files.
set( RTP_SOURCES
     "${CMAKE_CURRENT_LIST_DIR}/source/rtp_api.c"
     "${CMAKE_CURRENT_LIST_DIR}/source/rtp_endianness.c" )

# RTP library Public Include directories.
set( RTP_INCLUDE_PUBLIC_DIRS
     "${CMAKE_CURRENT_LIST_DIR}/source/include" )

# RTP library public include header files.
set( RTP_INCLUDE_PUBLIC_FILES
     "lib/include/rtp_api.h"
     "lib/include/rtp_data_types.h"
     "lib/include/rtp_endianness.h" )