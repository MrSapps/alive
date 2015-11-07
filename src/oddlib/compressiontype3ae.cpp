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
}


namespace Oddlib
{
    std::vector<Uint8> CompressionType3Ae::Decompress(IStream& stream, Uint32 size, Uint32 w, Uint32 h)
    {
        LOG_INFO("Data size is " << size);


        // TODO: Shouldn't need to be * 4
        std::vector< unsigned char > buffer(w*h * 4);

        unsigned char *aDbufPtr = &buffer[0];

        int control_byte; // esi@1
        int dstIndex; // edx@2
        unsigned char *dstPtr; // edi@2
        int count; // ebx@3
        unsigned char blackBytes; // al@8
        unsigned int srcByte; // edx@8
        int bytesToWrite; // ebx@8
        int control_byte_sub6; // esi@8
        int i; // ecx@9
        unsigned char *dstBlackBytesPtr; // edi@9
        unsigned int doubleBBytes; // ecx@9
        int byteCount; // ecx@18
        char dstByte; // al@23
        int v18; // [sp+8h] [bp-14h]@1
        int v19; // [sp+8h] [bp-14h]@8
        int width; // [sp+10h] [bp-Ch]@1
        int height; // [sp+14h] [bp-8h]@2
        unsigned char bytes; // [sp+20h] [bp+4h]@17

        control_byte = 0;
        width = ReadUint16(stream);
        v18 = 0;
        height = ReadUint16(stream);
        if (height > 0)
        {
            dstIndex = 0;// (int)aDbufPtr;
            dstPtr = aDbufPtr;
            do
            {
                count = 0;
                while (count < width)
                {
                    ReadNextSource(stream, control_byte, dstIndex);

                    blackBytes = dstIndex & 0x3F;
                    control_byte_sub6 = control_byte - 6;
                    srcByte = (unsigned int)dstIndex >> 6;

                    v19 = v18 - 1;
                    bytesToWrite = blackBytes + count;
                    if (blackBytes > 0)
                    {
                        doubleBBytes = (unsigned int)blackBytes >> 2;
                        memset(dstPtr, 0, 4 * doubleBBytes);
                        dstBlackBytesPtr = &dstPtr[4 * doubleBBytes];
                        for (i = blackBytes & 3; i; --i)
                        {
                            *dstBlackBytesPtr++ = 0;
                        }
                        dstPtr = &aDbufPtr[blackBytes];
                        aDbufPtr += blackBytes;
                    }

                    ReadNextSource(stream, control_byte_sub6, srcByte);

                    control_byte = control_byte_sub6 - 6;
                    bytes = srcByte & 0x3F;
                    dstIndex = srcByte >> 6;

                    v18 = v19 - 1;
                    count = bytes + bytesToWrite;
                    if ((signed int)bytes > 0)
                    {
                        byteCount = bytes;
                        v18 -= bytes;
                        do
                        {
                            ReadNextSource(stream, control_byte, dstIndex);

                            control_byte -= 6;
                            dstByte = dstIndex & 0x3F;
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
