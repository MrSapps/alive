#include "oddlib/bits_factory.hpp"
#include <vector>

namespace Oddlib
{
    class BitsFg1 : public IFg1
    {
    public:
        BitsFg1(SDL_Surface* camera, IStream& stream, bool bBitMaskedPartialBlocks);
        virtual SDL_Surface* GetSurface() const;
    private:
        SDL_SurfacePtr mSurface;
    };
}
