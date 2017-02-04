#include "sdl_raii.hpp"
#include "lodepng/lodepng.h"

/*static*/ void SDLHelpers::SaveSurfaceAsPng(const char* fileName, SDL_Surface* surface)
{
    lodepng::State state = {};

    // input color type
    state.info_raw.colortype = LCT_RGBA;
    state.info_raw.bitdepth = 8;

    // output color type
    state.info_png.color.colortype = LCT_RGBA;
    state.info_png.color.bitdepth = 8;
    state.encoder.auto_convert = 0;

    // encode to PNG
    std::vector<unsigned char> out;
    lodepng::encode(out, static_cast<const unsigned char*>(surface->pixels), surface->w, surface->h, state);

    // write out PNG buffer
    std::ofstream fileStream;
    fileStream.open(fileName, std::ios::binary);
    if (!fileStream.is_open())
    {
        throw Oddlib::Exception("Can't open output file");
    }

    fileStream.write(reinterpret_cast<const char*>(out.data()), out.size());
}


/*static*/ SDL_SurfacePtr SDLHelpers::LoadPng(Oddlib::IStream& stream, bool hasAlpha)
{
    lodepng::State state = {};

    // input color type
    state.info_raw.colortype = hasAlpha ? LCT_RGBA : LCT_RGB;
    state.info_raw.bitdepth = 8;

    // decode PNG
    std::vector<unsigned char> out;
    std::vector<unsigned char> in = Oddlib::IStream::ReadAll(stream);

    unsigned int w = 0;
    unsigned int h = 0;

    const auto decodeRet = lodepng::decode(out, w, h, state, in);

    if (decodeRet == 0)
    {

        SDL_SurfacePtr scaled;
        scaled.reset(SDL_CreateRGBSurfaceFrom(out.data(), w, h, hasAlpha ? 32 : 24, w * (hasAlpha ? 4 : 3), 0, 0, 0, 0));

        SDL_SurfacePtr ownedBuffer(SDL_CreateRGBSurface(0, w, h, hasAlpha ? 32 : 24, 0, 0, 0, 0));
        SDL_BlitSurface(scaled.get(), NULL, ownedBuffer.get(), NULL);

        return ownedBuffer;
    }
    else
    {
        LOG_ERROR(lodepng_error_text(decodeRet));
    }
    return nullptr;
}
