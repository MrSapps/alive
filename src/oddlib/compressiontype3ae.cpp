#include "oddlib/compressiontype3ae.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

template<typename T>
static void ReadNextSource(Oddlib::IStream& stream, int& control_byte, T& dstIndex)
{
    if (control_byte)
    {
        if (control_byte == 0xE) // Or 14
        {
            control_byte = 0x1Eu; // Or 30
            dstIndex |= ReadUint16(stream) << 14;
        }
    }
    else
    {
        dstIndex = ReadUint32(stream);
        control_byte = 0x20u; // 32
    }
    control_byte -= 6;
}


namespace Oddlib
{
    // Function 0x0040A6A0 in AE
    std::vector<Uint8> CompressionType3Ae::Decompress(IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 /*dataSize*/)
    {
        std::vector<Uint8> buffer(finalW*h);
        
        //const auto streamStart = stream.Pos();

        int dstPos = 0;
        int control_byte = 0;

        int width = w;
        int height = h;
        if (height > 0)
        {
            unsigned int dstIndex = 0;
            do
            {
                int columnNumber = 0;
                while (columnNumber < width)
                {
                    ReadNextSource(stream, control_byte, dstIndex);

                    const unsigned char blackBytes = dstIndex & 0x3F;
                    unsigned int srcByte = dstIndex >> 6;

                    // Here the "black" bytes would be written, since our buffers are inited to zeros we
                    // don't have to worry about getting this bit correct
                    const int bytesToWrite = blackBytes + columnNumber;
          
                    // Pretend we just blacked out an area
                    dstPos += blackBytes;
                    ReadNextSource(stream, control_byte, srcByte);

                    const unsigned char bytes = srcByte & 0x3F;
                    dstIndex = srcByte >> 6;

                    columnNumber = bytes + bytesToWrite;
                    if (bytes > 0)
                    {
                        int byteCount = bytes;
                        do
                        {
                            ReadNextSource(stream, control_byte, dstIndex);
                          
                            const char dstByte = dstIndex & 0x3F;
                            dstIndex = dstIndex >> 6;

                            buffer[dstPos] = dstByte;
                            dstPos++;
                            --byteCount;
                        } while (byteCount);
                    }
                }
                if (columnNumber & 3)
                {
                    do
                    {
                        ++dstPos;
                        ++columnNumber;
                    } while (columnNumber & 3);
                }
            } while (height-- != 1);
        }

        //const auto readSize = (stream.Pos() - streamStart);
        //assert(readSize <= dataSize);

        return buffer;
    }
}
