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

namespace Oddlib
{
    std::vector<Uint8> CompressionType3Ae::Decompress(IStream& stream, Uint32 size, Uint32 w, Uint32 h)
    {
        std::vector<Uint8> ret;

        LOG_INFO("Data size is " << size);

        //Uint32 unknown = 0;
        //stream.ReadUInt32(unknown);
        //assert(unknown == size);
        /*
        std::vector<Uint8> tmp(size);
        stream.ReadBytes(tmp.data(), tmp.size());
        tmp.resize(tmp.size());

        // QByteArray d = aFrame.iFrameData.mid(4);
        unsigned char* aAnimDataPtr = (unsigned char*)tmp.data(); // (unsigned char*)d.data();

        unsigned short int *aFramePtr = (unsigned short int *)aAnimDataPtr;
        */

        std::vector< unsigned char > buffer;

        buffer.resize(w*h*4);  // Ensure big enough via a guess!

        unsigned char *aDbufPtr = &buffer[0];

        int height_copy; // eax@1
       // unsigned short int *srcPtr; // ebp@1
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
        height_copy = ReadUint16(stream);
        if (height_copy > 0)
        {
            dstIndex = (int)aDbufPtr;
            dstPtr = aDbufPtr;
            height = height_copy;
            do
            {
                count = 0;
                while (count < width)
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
                    blackBytes = dstIndex & 0x3F;
                    control_byte_sub6 = control_byte - 6;
                    srcByte = (unsigned int)dstIndex >> 6;
                    v19 = v18 - 1;
                    bytesToWrite = blackBytes + count;
                    if ((signed int)blackBytes > 0)
                    {
                        doubleBBytes = (unsigned int)blackBytes >> 2;
                        memset(dstPtr, 0, 4 * doubleBBytes);
                        dstBlackBytesPtr = &dstPtr[4 * doubleBBytes];
                        for (i = blackBytes & 3; i; --i)
                            *dstBlackBytesPtr++ = 0;
                        dstPtr = &aDbufPtr[blackBytes];
                        aDbufPtr += blackBytes;
                    }
                    if (control_byte_sub6)
                    {
                        if (control_byte_sub6 == 0xE)
                        {
                            control_byte_sub6 = 0x1Eu;
                            srcByte |= ReadUint16(stream) << 14;
                        }
                    }
                    else
                    {
                        srcByte = ReadUint32(stream);
                        control_byte_sub6 = 0x20u;
                    }
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
                height_copy = height - 1;
            } while (height-- != 1);
        }

        return buffer;
    }
}
