#include "oddlib/compressiontype3.hpp"
#include "oddlib/stream.hpp"

#include <windows.h>

namespace Oddlib
{
    static void Next6Bits(signed int& magic, unsigned int& src_data, unsigned int& frame_data_index, unsigned short int* frame_data)
    {
        if (magic > 0) // Always zero the first time
        {
            if (magic == 14)
            {
                magic = 30;
                src_data = (frame_data[frame_data_index] << 14) | src_data; // ReadUint16
                frame_data_index++;
            }
        }
        else
        {
            magic = 32;
            src_data = frame_data[frame_data_index] | (frame_data[frame_data_index + 1] << 16); // read uint32
            frame_data_index += 2;
        }
        magic -= 6;
    }

    // Function 0x004031E0 in AO
    std::vector<Uint8> CompressionType3::Decompress(IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 dataSize)
    {
        // Params
        const unsigned int whSize = w*h;
        std::vector<Uint8> input(dataSize);
        stream.ReadBytes(input.data(), input.size());
        input.resize(input.size() * 4); // TODO: Why do we sometimes read past the end? Because reading in / sizeof(Uint32)'s?

        unsigned char* aAnimDataPtr = (unsigned char*)input.data();

        unsigned int aSize = (input.size() /2 ); // Don't exactly know what this is, just pray its big enough for now!

        std::vector<unsigned char> buffer;
        buffer.resize(finalW*h*4);

        // Locals
        int sizeInBytes; // eax@1
        unsigned short int *frame_data; // ebx@1
        //unsigned char *srcPtr; // ebp@1
        int numBytesInFrameCnt; // edi@1
        signed int bitCounter; // esi@1
        unsigned char *pDst; // edx@3
        int for_counter; // [sp+20h] [bp+Ch]@1
        
        numBytesInFrameCnt = whSize & 0xFFFFFFFC;
        for_counter = whSize & 3;
        frame_data = (unsigned short int*)aAnimDataPtr; // Convert poiner type, ok!
        bitCounter = 0;

//        const Uint32 kOtherStart = 6 * (numBytesInFrameCnt / 8);
//        srcPtr = &input[kOtherStart];
        sizeInBytes = (aSize + 3) / 4;

        pDst = &buffer[0];
        if (numBytesInFrameCnt)
        {
            unsigned int frame_data_index = 0;
            unsigned int src_data = 0;
            do
            {
                Next6Bits(bitCounter, src_data, frame_data_index, frame_data);
                unsigned char bits = src_data & 0x3F;
                src_data = src_data >> 6;
                --numBytesInFrameCnt;
                if (bits & 0x20)
                {
                    // 0x20= 0b100000
                    // 0x3F= 0b111111
                    int numberOfBytes = (bits & 0x1F) + 1;
                    if (numberOfBytes)
                    {
                        do
                        {
                            if (numBytesInFrameCnt)
                            { 
                                Next6Bits(bitCounter, src_data, frame_data_index, frame_data);
                                bits = src_data & 0x3F;
                                src_data = src_data >> 6;
                                --numBytesInFrameCnt;
                            }
                            else
                            {
                                // 6 bits from "other" source
                                bits = 0;// *srcPtr++ & 0x3F;
                                --for_counter;
                            }
                            *pDst++ = bits;
                        } while (--numberOfBytes != 0);
                    }
                }
                else
                {
                    // literal flag isn't set, so we have up to 5 bits of "black" pixels
                    pDst += bits + 1;
                }
            } while (numBytesInFrameCnt);
        }

        /*
        // Some sort of "remainder" handling using 8bits instead of 6?
        int result; // eax@23
        unsigned char curByte; // al@24
        int amount; // eax@25
        int bytes_to_copy; // ecx@26
        unsigned char byte; // al@27
        unsigned char curByteCopy; // [sp+18h] [bp+4h]@24

        if (for_counter > 0)
        {
            for (result = for_counter; for_counter; result = for_counter)
            {
                curByte = 0;// *srcPtr++ & 0x3F;
                curByteCopy = curByte;
                --for_counter;
                if (curByte & 0x20)
                {
                    amount = (curByte & 0x1F) + 1;
                    if (amount)
                    {
                        bytes_to_copy = amount;
                        do
                        {
                            byte = 0;// *srcPtr++ & 0x3F;
                            *pDst++ = byte;
                            --bytes_to_copy;
                            --for_counter;
                        } while (bytes_to_copy);
                    }
                }
                else
                {
                    // Skip black pixels
                    pDst += curByteCopy + 1;
                }
            }
        }
        */

        return buffer;
    }
}
