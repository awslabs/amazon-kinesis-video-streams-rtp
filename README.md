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

## License

This project is licensed under the Apache-2.0 License.

