## amazon-kinesis-video-streams-rtp

The goal of the Real-time Transport Protocol (RTP) library is to provide
RTP Serializer and Deserializer functionalities. Along with this, RTP library
also provide codec packetization and depacketization functionality for G.711,
VP8, Opus and H.264 codecs.

## What is RTP?

[Real-Time Transport Protocol (RTP)](https://en.wikipedia.org/wiki/Real-time_Transport_Protocol),
as defined in [RFC 3550](https://datatracker.ietf.org/doc/html/rfc3550), is a data transport protocol
used to deliver real-time data such as audio or video over IP networks. An RTP packet comprises of a
header followed by the payload.

A RTP packet consist of following fields followed by an optional RTP Header extension.

```
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |V=2|P|X|  CC   |M|     PT      |       Sequence Number         |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                           Timestamp                           |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |           Synchronization Source (SSRC) identifier            |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |       Contributing Source (CSRC[0..15]) identifiers           |
        |                             ....                              |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        RTP Header extension:

         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |            Profile            |           Length              |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                        Header Extension                       |
        |                             ....                              |
```

## Using the library

### Serializer
    1. Call `Rtp_Init()` to initialize the RTP Context.
    2. Initialize an RtpPacket_t with the desired values.
    3. Call `Rtp_Serialize()` to serialize the RtpPacket_t passed.

### Deserializer
    1. Call `Rtp_Init()` to initialize the RTP Context.
    2. Pass the serialized packet along with its length to `Rtp_DeSerialize()` to
       deserialize the packet.

## Packetization
    1. Call `<Codec>Packetization_Init()` to intitializae the particular codec context.
    2. In case of H.264 Codec packetization -
        - Call `H264Packetizer_AddFrame()` to add  a frame OR
        - Call `H264Packetizer_AddNalu()` repeatedly to add NALUs of a frame.
    3. For all codes other than H.264, call `<Codec>Packetizer_AddFrame()` to add a frame.
    4. Call `<Codec>Packetizer_GetPacket()` repeatedly to retrieve the packets
       until `<Codec>_RESULT_NO_MORE_PACKETS` is returned.

## Depacketization
    1. Call `<Codec>Depacketization_Init()` to initialize the codec context.
    2. Call `<Codec>Depacketizer_AddPacket()` repeatedly to add all packets received
       corresponding to one frame.
    3. In case of H.264 Codec depacketization -
       - Call `H264Depacketizer_GetFrame()` to get a frame OR
       - Call `H264Depacketizer_GetNalu()` iteratively to get NALUs one by one until
         H264_RESULT_NO_MORE_NALUS is returned.
    4. For all codes other than H.264, Call `<Codec>Depacketizer_GetFrame()` to get the
       complete frame once all packets are added.

## Building Unit Tests

### Platform Prerequisites
- For running unit tests:
    - C99 compiler like gcc.
    - CMake 3.13.0 or later.
    - Ruby 2.0.0 or later (It is required for the CMock test framework that we
      use).
- For running the coverage target, gcov and lcov are required.

### Checkout CMock Submodule
By default, the submodules in this repository are configured with `update=none`
in [.gitmodules](./.gitmodules) to avoid increasing clone time and disk space
usage of other repositories.

To build unit tests, the submodule dependency of CMock is required. Use the
following command to clone the submodule:

```sh
git submodule update --checkout --init --recursive test/CMock
```

### Steps to build Unit Tests
1. Go to the root directory of this repository. (Make sure that the CMock
   submodule is cloned as described in [Checkout CMock Submodule](#checkout-cmock-submodule)).
1. Run the following command to generate Makefiles:

    ```sh
    cmake -S test/unit-test -B build/ -G "Unix Makefiles" \
     -DCMAKE_BUILD_TYPE=Debug \
     -DBUILD_CLONE_SUBMODULES=ON \
     -DCMAKE_C_FLAGS='--coverage -Wall -Wextra -Werror -DNDEBUG'
    ```
1. Run the following command to build the library and unit tests:

    ```sh
    make -C build all
    ```
1. Run the following command to execute all tests and view results:

    ```sh
    cd build && ctest -E system --output-on-failure
    ```

### Steps to generate code coverage report of Unit Test
1. Run Unit Tests in [Steps to build Unit Tests](#steps-to-build-unit-tests).
1. Generate coverage.info in build folder:

    ```
    make coverage
    ```
1. Get code coverage by lcov:

    ```
    lcov --rc lcov_branch_coverage=1 -r coverage.info -o coverage.info '*test*' '*CMakeCCompilerId*' '*mocks*'
    ```
1. Generage HTML report in folder `CodecovHTMLReport`:

    ```
    genhtml --rc lcov_branch_coverage=1 --ignore-errors source coverage.info --legend --output-directory=CodecovHTMLReport
    ```

### Script to run Unit Test and generate code coverage report

```sh
git submodule update --init --recursive --checkout test/CMock
cmake -S test/unit-test -B build/ -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_CLONE_SUBMODULES=ON -DCMAKE_C_FLAGS='--coverage -Wall -Wextra -Werror -DNDEBUG -DLIBRARY_LOG_LEVEL=LOG_DEBUG'
make -C build all
cd build
ctest -E system --output-on-failure
make coverage
lcov --rc lcov_branch_coverage=1 -r coverage.info -o coverage.info '*test*' '*CMakeCCompilerId*' '*mocks*'
genhtml --rc lcov_branch_coverage=1 --ignore-errors source coverage.info --legend --output-directory=CodecovHTMLReport
```

## License

This project is licensed under the Apache-2.0 License.

