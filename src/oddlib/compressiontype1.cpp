#include "oddlib/compressiontype1.hpp"
#include "oddlib/stream.hpp"

namespace Oddlib
{
    // Function 0x0040A610 in AE
    std::vector<Uint8> CompressionType1::Decompress(IStream& /*stream*/, Uint32 /*finalW*/, Uint32 /*w*/, Uint32 /*h*/, Uint32 /*dataSize*/)
    {
        abort();
        //return std::vector<Uint8>();
    }
}
