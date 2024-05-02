/* API includes. */
#include "rtp_endianness.h"

#define SWAP_BYTES_32( value )           \
    ( ( ( ( value ) >> 24 ) & 0xFF )  |  \
      ( ( ( value ) >> 8 ) & 0xFF00 ) |  \
      ( ( ( value ) & 0xFF00 ) << 8 ) |  \
      ( ( ( value ) & 0xFF ) << 24 ) )

/*-----------------------------------------------------------*/

void WriteUint32Swap( uint8_t * pDst,
                      uint32_t val )
{
    *( ( uint32_t * )( pDst ) ) = SWAP_BYTES_32( val );
}

/*-----------------------------------------------------------*/

uint32_t ReadUint32Swap( const uint8_t * pSrc )
{
    return SWAP_BYTES_32( *( ( uint32_t * )( pSrc ) ) );
}

/*-----------------------------------------------------------*/

void WriteUint32NoSwap( uint8_t * pDst,
                        uint32_t val )
{
    *( ( uint32_t * )( pDst ) ) = ( val );
}

/*-----------------------------------------------------------*/

uint32_t ReadUint32NoSwap( const uint8_t * pSrc )
{
    return *( ( uint32_t * )( pSrc ) );
}

/*-----------------------------------------------------------*/

void Rtp_InitReadWriteFunctions( RtpReadWriteFunctions_t * pReadWriteFunctions )
{
    uint8_t isLittleEndian;

    isLittleEndian = ( *( uint8_t * )( &( uint16_t ) { 1 } ) == 1 );

    if( isLittleEndian != 0 )
    {
        pReadWriteFunctions->writeUint32Fn = WriteUint32Swap;
        pReadWriteFunctions->readUint32Fn = ReadUint32Swap;
    }
    else
    {
        pReadWriteFunctions->writeUint32Fn = WriteUint32NoSwap;
        pReadWriteFunctions->readUint32Fn = ReadUint32NoSwap;
    }
}

/*-----------------------------------------------------------*/
