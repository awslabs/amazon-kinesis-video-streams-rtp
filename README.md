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
    2. Populate the fields of the RTP packet with the desired values.
    3. Call `Rtp_Serialize()` to serialize the RTP packet passed.

### Deserializer
    1. Call `Rtp_Init()` to initialize the RTP Context.
    2. Pass the serialized packet along with its length to `Rtp_DeSerialize()` to
       deserialize the packet.

## Packetization
    1. Call `<Codec>Packetization_Init()` to intitializae the particular codec context.
    2. In case of H.264 Codedc packetization,` is returned. call `H264Packetizer_AddFrame()` or
       `H264Packetizer_AddNalu()` to add frame or individual NALUs.
    3. Call `<Codec>Packetizer_GetPacket()` in a loop to retrieve the packets iteratively
       unless `<Codec>_RESULT_NO_MORE_PACKETS` is returned.

## Depacketization
    1. Call `<Codec>Depacketization_Init()` to initialize the codec context.
    2. Call `<Codec>Depacketizer_AddPacket()` to add all packets received corresponding
       to one frame.
    3. In case of H.264 Call  `H264Depacketizer_GetNalu()` iteratively to get all the NALUs
       one by one untill H264_RESULT_NO_MORE_NALUS is returned.
    4. Call `<Codec>Depacketizer_GetFrame()` to get the complete frame once all packets are
       added.

## License

This project is licensed under the Apache-2.0 License.

