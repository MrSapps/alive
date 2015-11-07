#include "oddlib/compressiontype3ae.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

static Uint16 ReadUint16(Oddlib::IStream& stream)
{
    Uint16 ret = 0;
    stream.ReadUInt16(ret);
    return ret;
}

static Uint32 ReadUint32(Oddlib::IStream& stream)
{
    Uint32 ret = 0;
    stream.ReadUInt32(ret);
    return ret;
}

template<typename T>
static void ReadNextSource(Oddlib::IStream& stream, int& control_byte, T& dstIndex)
{
    if (control_byte)
    {
        if (control_byte == 0xE)
        {
            control_byte = 0x1Eu;
            dstIndex |= ReadUint16(stream) << 14;
        }
    }
    else
    {
        dstIndex = ReadUint32(stream);
        control_byte = 0x20u;
    }
    control_byte -= 6;
}


namespace Oddlib
{
    std::vector<Uint8> CompressionType3Ae::Decompress(IStream& stream, Uint32 size, Uint32 w, Uint32 h)
    {
        LOG_INFO("Data size is " << size);

        // TODO: Shouldn't need to be * 4
        std::vector< unsigned char > buffer(w*h * 4);

        unsigned char *aDbufPtr = &buffer[0];
        int dstIndex; // edx@2
        unsigned char *dstPtr; // edi@2
   
        int control_byte = 0;
        int width = ReadUint16(stream);
        int height = ReadUint16(stream);
        if (height > 0)
        {
            dstIndex = 0;// (int)aDbufPtr;
            dstPtr = aDbufPtr;
            do
            {
                int count = 0;
                while (count < width)
                {
                    ReadNextSource(stream, control_byte, dstIndex);

                    const unsigned char blackBytes = dstIndex & 0x3F;
                   
                     unsigned int srcByte = (unsigned int)dstIndex >> 6;


                    const int bytesToWrite = blackBytes + count;
                    if (blackBytes > 0)
                    {
                        const unsigned int doubleBBytes = (unsigned int)blackBytes >> 2;
                        memset(dstPtr, 0, 4 * doubleBBytes);
                        unsigned char* dstBlackBytesPtr = &dstPtr[4 * doubleBBytes];
                        for (int i = blackBytes & 3; i; --i)
                        {
                            *dstBlackBytesPtr++ = 0;
                        }
                        dstPtr = &aDbufPtr[blackBytes];
                        aDbufPtr += blackBytes;
                    }

                    ReadNextSource(stream, control_byte, srcByte);

                    const unsigned char bytes = srcByte & 0x3F;
                    dstIndex = srcByte >> 6;

                    count = bytes + bytesToWrite;
                    if (bytes > 0)
                    {
                        int byteCount = bytes;
                        do
                        {
                            ReadNextSource(stream, control_byte, dstIndex);
                          
                            const char dstByte = dstIndex & 0x3F;
                            dstIndex = (unsigned int)dstIndex >> 6;

                            *dstPtr++ = dstByte;
                            --byteCount;
                        } while (byteCount);
                        aDbufPtr = dstPtr;
                    }
                }
                if (count & 3)
                {
                    do
                    {
                        ++dstPtr;
                        ++count;
                    } while (count & 3);
                    aDbufPtr = dstPtr;
                }
            } while (height-- != 1);
        }

        return buffer;
    }
}
