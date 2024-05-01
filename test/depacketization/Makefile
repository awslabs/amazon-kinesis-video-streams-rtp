APP_NAME = "depacketizationTest.bin"

# Source Files
SRCS += "depacketization_test.c"
SRCS += "../../codec_packetizers/h264/h264_depacketizer.c"
SRCS += "../../codec_packetizers/vp8/vp8_depacketizer.c"
SRCS += "../../codec_packetizers/g711/g711_depacketizer.c"
SRCS += "../../codec_packetizers/opus/opus_depacketizer.c"

# Include Directories
INCLUDE_DIRS += "-I../../codec_packetizers/h264/include/"
INCLUDE_DIRS += "-I../../codec_packetizers/vp8/include/"
INCLUDE_DIRS += "-I../../codec_packetizers/g711/include/"
INCLUDE_DIRS += "-I../../codec_packetizers/opus/include/"

OBJS=$(SRCS:.c=.o)

CFLAGS+=-ggdb

.DEFAULT_GOAL:=all

all: build run

build:
	$(CC) -o $(APP_NAME) $(SRCS) $(INCLUDE_DIRS) $(CFLAGS)

run:
	./$(APP_NAME)

clean:
	rm -rf $(APP_NAME)

.PHONY: all build run clean