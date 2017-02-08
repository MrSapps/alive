#include "oddlib/bits_factory.hpp"
#include <vector>

namespace Oddlib
{
    class BitsFg1AePc : public IFg1
    {
    public:
        BitsFg1AePc(SDL_Surface* camera, IStream& stream, bool bBitMaskedPartialBlocks);
        virtual SDL_Surface* GetSurface() const;
    private:
        SDL_SurfacePtr mSurface;
    };
}
