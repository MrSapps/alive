#include "fmv.hpp"
#include "logger.hpp"
#include "stdthread.h"
#include "abstractrenderer.hpp"
#include "resourcemapper.hpp"
#include "cdromfilesystem.hpp"
#include "soxr.h"

class AutoMouseCursorHide
{
public:
    AutoMouseCursorHide()
    {
        SDL_ShowCursor(0);
    }

    ~AutoMouseCursorHide()
    {
        SDL_ShowCursor(1);
    }
};


static f32 Percent(f32 max, f32 percent)
{
    return (max / 100.0f) * percent;
}

static void RenderSubtitles(AbstractRenderer& rend, const char* msg, float x, float y, float w, float h)
{
    const f32 fontSize = Percent(h, 6.7f);
    f32 bounds[4];
    rend.TextBounds(x, y, fontSize, msg, bounds);

    const f32 fontW = bounds[2] - bounds[0];
    const f32 ypos = Percent(h, 90.0f);
    const f32 xpos = x + (w / 2) - (fontW / 2);

    rend.Text(
        xpos,
        ypos,
        fontSize,
        msg,
        { 255, 255, 255, 255 },
        AbstractRenderer::eLayers::eFmv + 2,
        AbstractRenderer::eBlendModes::eNormal,
        AbstractRenderer::eCoordinateSystem::eScreen);
}

IMovie::IMovie(const std::string& resourceName, IAudioController& controller, std::unique_ptr<SubTitleParser> subtitles)
    : mAudioController(controller), mSubTitles(std::move(subtitles)), mName(resourceName)
{

}

IMovie::~IMovie()
{

}


// Main thread context
void IMovie::OnRenderFrame(AbstractRenderer& rend)
{
    // TODO: Populate mAudioBuffer and mVideoBuffer
    // for up to N buffered frames
    std::lock_guard<std::mutex> lock(mAudioBufferMutex);

    if (!mPlaying)
    {
        return;
    }

    while (NeedBuffer())
    {
        FillBuffers();
    }

    // TODO: If the buffer call back for audio is large, then this might only get called every N frames meaning
    // we can drop video frames even if not running too slowly. We should take this into account and interpolate between
    // now and the expected next call time.
    const auto videoFrameIndex = mConsumedAudioBytes / mAudioBytesPerFrame;
    const char *current_subs = nullptr;
    if (mSubTitles)
    {
        // We assume the FPS is always 15, thus 1000/15=66.66 so frame number * 66 = number of msecs into the video
        const auto& subs = mSubTitles->Find((videoFrameIndex * 66)+200);
        if (!subs.empty())
        {
            // TODO: Render all active subs, not just the first one
            current_subs = subs[0]->Text().c_str();
            if (subs.size() > 1)
            {
                LOG_WARNING("Too many active subtitles " << subs.size());
            }
        }
    }

    bool played = false;
    while (!mVideoBuffer.empty())
    {
        Frame& f = mVideoBuffer.front();
        if (f.mFrameNum < videoFrameIndex)
        {
            if (mVideoBuffer.size() == 1)
            {
                // Don't remove everything otherwise we won't have any frame to display at all
                break;
            }

            mVideoBuffer.pop_front();
            continue;
        }

        if (f.mFrameNum == videoFrameIndex)
        {
            // Don't pop frame after rendering for the case when the video ends and we are playing
            // audio but there are no more frames. In the case we just keep displaying whatever the last
            // frame was (since we didn't pop it).
            RenderFrame(rend, f.mW, f.mH, f.mPixels.data(), current_subs);
            played = true;
            break;
        }

    }

    if (!played && !mVideoBuffer.empty())
    {
        Frame& f = mVideoBuffer.front();
        RenderFrame(rend, f.mW, f.mH, f.mPixels.data(), current_subs);
    }

    while (NeedBuffer())
    {
        FillBuffers();
    }
}

// Main thread context
bool IMovie::IsEnd()
{
    std::lock_guard<std::mutex> lock(mAudioBufferMutex);
    const auto ret = EndOfStream() && mAudioBuffer.empty();
    if (ret && mVideoBuffer.size() > 1)
    {
        LOG_ERROR("Still " << mVideoBuffer.size() << " frames left after audio finished");
    }
    return ret;
}

// Main thread context
void IMovie::Start()
{
    std::lock_guard<std::mutex> lock(mAudioBufferMutex);
    mAudioController.SetExclusiveAudioPlayer(this);
    mPlaying = true;
}

// Main thread context
void IMovie::Stop()
{
    std::lock_guard<std::mutex> lock(mAudioBufferMutex);
    mAudioController.SetExclusiveAudioPlayer(nullptr);
    mPlaying = false;
}

// Audio thread context, from IAudioPlayer
bool IMovie::Play(f32* stream, u32 len)
{
    std::lock_guard<std::mutex> lock(mAudioBufferMutex);

    // Consume mAudioBuffer and update the amount of consumed bytes
    size_t have = mAudioBuffer.size()/sizeof(int16_t);
    size_t take = len;
    if (take > have)
    {
        // Buffer underflow - we don't have enough data to fill the requested buffer
        // audio glitches ahoy!
        LOG_ERROR("Audio buffer underflow want " << take << " samples " << " have " << have << " samples");
        take = have;
    }

    for (auto i = 0u; i < take; i++)
    {
        uint8_t low = mAudioBuffer[i*sizeof(int16_t)];
        uint8_t high = mAudioBuffer[i*sizeof(int16_t) + 1];
        int16_t fixed = (int16_t)(low | (high << 8));

        // TODO: Add a proper audio mixing algorithm/API, this will clip/overflow and cause weridnes when
        // 2 streams of diff sample rates are mixed
        stream[i] += fixed / 32768.0f;
    }
    mAudioBuffer.erase(mAudioBuffer.begin(), mAudioBuffer.begin() + take*sizeof(int16_t));
    mConsumedAudioBytes += take*sizeof(int16_t);
    return false;
}

void IMovie::RenderFrame(AbstractRenderer &rend, int width, int height, const void *pixels, const char* subtitles)
{
    // TODO: Optimize - should update 1 texture rather than creating per frame
    TextureHandle texhandle = rend.CreateTexture(AbstractRenderer::eTextureFormats::eRGB, width, height, AbstractRenderer::eTextureFormats::eRGBA, pixels, true);
    
    rend.TexturedQuad(texhandle, 
        0,
        0,
        static_cast<f32>(rend.Width()),
        static_cast<f32>(rend.Height()),
        AbstractRenderer::eFmv,
        ColourU8{ 255, 255, 255, 255 },
        AbstractRenderer::eNormal,
        AbstractRenderer::eScreen);
    

    if (subtitles)
    {
        
        RenderSubtitles(rend, subtitles,
            0,
            0,
            static_cast<f32>(rend.Width()),
            static_cast<f32>(rend.Height()));
    }

    rend.DestroyTexture(texhandle);
}

// PSX MOV/STR format, all PSX game versions use this.
class MovMovie : public IMovie
{
protected:
    std::unique_ptr<Oddlib::IStream> mFmvStream;
    bool mPsx = false;
    MovMovie(const std::string& resourceName, IAudioController& audioController, std::unique_ptr<SubTitleParser> subtitles)
        : IMovie(resourceName, audioController, std::move(subtitles))
    {

    }
public:
    MovMovie(MovMovie&&) = delete;
    MovMovie& operator = (MovMovie&&) = delete;

    ~MovMovie()
    {

    }

    MovMovie(const std::string& resourceName, IAudioController& audioController, std::unique_ptr<Oddlib::IStream> stream, std::unique_ptr<SubTitleParser> subtitles, u32 startSector, u32 numberOfSectors)
        : IMovie(resourceName, audioController, std::move(subtitles))
    {
        if (numberOfSectors == 0)
        {
            mFmvStream = std::move(stream);
        }
        else
        {
            // Playing only a subset of the stream
            mFmvStream.reset(stream->Clone(startSector, numberOfSectors));
        }

        const int kSampleRate = 44100;// 37800;
        const int kFps = 15;
        const u32 kNumChannels = 2;
        mAudioBytesPerFrame = (kSampleRate / kFps) * kNumChannels * sizeof(u16);

        //mAudioController.SetAudioSpec(kSampleRate / kFps, kSampleRate);

        mPsx = true;
    }

    struct RawCDXASector
    {
        uint8_t data[2328 + 12 + 4 + 8];
    };

    static const uint8_t m_CDXA_STEREO = 3;

#pragma pack(push)
#pragma pack(1)
    struct PsxVideoFrameHeader
    {
        unsigned short int mNumMdecCodes;
        unsigned short int m3800Magic;
        unsigned short int mQuantizationLevel;
        unsigned short int mVersion;
    };

    struct PsxStrHeader
    {
        // these 2 make up the 8 byte subheader?
        unsigned int mSectorType; // AKIK
        unsigned int mSectorNumber;

        // The 4 "unknown" / 0x80010160 in psx data is replaced by "AKIK" in PC data
        unsigned int mAkikMagic;

        unsigned short int mSectorNumberInFrame;
        unsigned short int mNumSectorsInFrame;
        unsigned int mFrameNum;
        unsigned int mFrameDataLen;
        unsigned short int mWidth;
        unsigned short int mHeight;

        PsxVideoFrameHeader mVideoFrameHeader;
        unsigned int mNulls;

        unsigned char frame[2296];
    };
#pragma pack(pop)

    virtual bool EndOfStream() override
    {
        return mFmvStream->AtEnd();
    }

    virtual bool NeedBuffer() override
    {
        const u32 kNumChans = 2;
        const u32 requiredSize = mAudioController.SampleRate() * sizeof(u16) * kNumChans;

        return (mVideoBuffer.size() == 0 || mAudioBuffer.size() < (requiredSize*2)) && !mFmvStream->AtEnd();
    }

    virtual void FillBuffers() override
    {
        while (NeedBuffer())
        {
            const int kXaFrameDataSize = 2016;
            const int kNumAudioChannels = 2;
            const int kBytesPerSample = 2;
            std::array<s16, (kXaFrameDataSize * kNumAudioChannels * kBytesPerSample) / 2> outPtr;

            if (mDemuxBuffer.empty())
            {
                mDemuxBuffer.resize(1024 * 1024);
            }

            std::vector<u8> pixelBuffer;
            for (;;)
            {

                PsxStrHeader w;
                if (mFmvStream->AtEnd())
                {
                    return;
                }
                mFmvStream->ReadBytes(reinterpret_cast<u8*>(&w), sizeof(w));

                // PC sector must start with "MOIR" if video, else starts with "VALE"
                if (!mPsx && w.mSectorType != 0x52494f4d)
                {
                    // abort();
                }

                // AKIK is 0x80010160 in PSX
                const auto kMagic = mPsx ? 0x80010160 : 0x4b494b41;
                bool noAudio = false;
                if (w.mAkikMagic != kMagic)
                {
                    if (mPsx)
                    {
                        /*
                        std::cout <<
                        (CHECK_BIT(xa->subheader.coding_info, 0) ? "Mono " : "Stereo ") <<
                        (CHECK_BIT(xa->subheader.coding_info, 2) ? "37800Hz " : "18900Hz ") <<
                        (CHECK_BIT(xa->subheader.coding_info, 4) ? "4bit " : "8bit ")
                        */

                        RawCdImage::CDXASector* rawXa = (RawCdImage::CDXASector*)&w;
                        if (rawXa->subheader.coding_info != 0)
                        {
                            mAdpcm.DecodeFrameToPCM(outPtr, &rawXa->data[0]);
                        }
                        else
                        {
                            // Blank/empty audio frame, play silence so video stays in sync
                            //numBytes = 2016 * 2 * 2;
                            mAudioBuffer.resize(mAudioBuffer.size() + (2352*4));
                            noAudio = true;
                        }
                    }
                    else
                    {
                        mAdpcm.DecodeFrameToPCM(outPtr, (uint8_t *)&w.mAkikMagic);
                    }

                    if (!noAudio)
                    {
                        size_t consumedSrc = 0;
                        size_t wroteSamples = 0;
                        size_t inLenSampsPerChan = kXaFrameDataSize;
                        size_t outLenSampsPerChan = inLenSampsPerChan * 2;
                        std::vector<u8> tmp2(2352 * 4);

                        soxr_io_spec_t ioSpec = soxr_io_spec(
                            SOXR_INT16_I,   // In type
                            SOXR_INT16_I);  // Out type

                        soxr_oneshot(
                            37800,       // Input rate
                            44100,      // Output rate
                            2,          // Num channels
                            outPtr.data(),
                            inLenSampsPerChan,
                            &consumedSrc,
                            tmp2.data(),
                            outLenSampsPerChan,
                            &wroteSamples,
                            &ioSpec,    // IO spec
                            nullptr,    // Quality spec
                            nullptr     // Runtime spec
                        );


                        for (auto i = 0u; i < wroteSamples * 4; i++)
                        {
                            mAudioBuffer.push_back(tmp2[i]);
                        }
                    }

                    // Must be VALE
                    continue;
                }
                else
                {
                    const u16 frameW = w.mWidth;
                    const u16 frameH = w.mHeight;

                    uint32_t bytes_to_copy = w.mFrameDataLen - w.mSectorNumberInFrame *kXaFrameDataSize;
                    if (bytes_to_copy > 0)
                    {
                        if (bytes_to_copy > kXaFrameDataSize)
                        {
                            bytes_to_copy = kXaFrameDataSize;
                        }

                        memcpy(mDemuxBuffer.data() + w.mSectorNumberInFrame * kXaFrameDataSize, w.frame, bytes_to_copy);
                    }

                    if (w.mSectorNumberInFrame == w.mNumSectorsInFrame - 1)
                    {
                        // Always resize as its possible for a stream to change its frame size to be smaller or larger
                        // this happens in the AE PSX MI.MOV streams
                        pixelBuffer.resize(frameW * frameH * 4); // 4 bytes per pixel

                        mMdec.DecodeFrameToABGR32((uint16_t*)pixelBuffer.data(), (uint16_t*)mDemuxBuffer.data(), frameW, frameH);
                        mVideoBuffer.push_back(Frame{ mFrameCounter++, frameW, frameH, pixelBuffer });

                        return;
                    }
                }
            }
        }
    }

private:
    std::vector<unsigned char> mDemuxBuffer;
    PSXMDECDecoder mMdec;
    PSXADPCMDecoder mAdpcm;
};

// Same as MOV/STR format but with modified magic in the video frames
// and XA audio raw cd sector information stripped. Only used in AO PC.
class DDVMovie : public MovMovie
{
public:
    DDVMovie(DDVMovie&&) = delete;
    DDVMovie& operator = (DDVMovie&&) = delete;
    DDVMovie(const std::string& resourceName, IAudioController& audioController, std::unique_ptr<Oddlib::IStream> stream, std::unique_ptr<SubTitleParser> subtitles)
        : MovMovie(resourceName, audioController, std::move(subtitles))
    {
        mPsx = false;
        const int kSampleRate = 44100;// 37800;
       // const u32 kAudioFreq = 37800;
        const u32 kNumChannels = 2;
        const u32 kFps = 15;
        mAudioBytesPerFrame = (kSampleRate / kFps) * kNumChannels * sizeof(u16);
        //mAudioController.SetAudioSpec(kAudioFreq / kFps, kAudioFreq);
        mFmvStream = std::move(stream);
    }
};

// Also called DDV on disk confusingly. Not the same as DDVMovie used in AO PC, this
// type can be found in AE PC.
class MasherMovie : public IMovie
{
public:
    MasherMovie(const std::string& resourceName, IAudioController& audioController, std::unique_ptr<Oddlib::IStream> stream, std::unique_ptr<SubTitleParser> subtitles)
        : IMovie(resourceName, audioController, std::move(subtitles))
    {
        mMasher = std::make_unique<Oddlib::Masher>(std::move(stream));

        if (mMasher->HasAudio())
        {
            //mAudioController.SetAudioSpec(static_cast<u16>(mMasher->SingleAudioFrameSizeSamples()), mMasher->AudioSampleRate());
        }

        if (mMasher->HasVideo())
        {
            mFramePixels.resize(mMasher->Width() * mMasher->Height() * sizeof(u32));
        }

        const u32 kNumChannels = 2;
        mAudioBytesPerFrame = sizeof(u16) * kNumChannels * mMasher->SingleAudioFrameSizeSamples();
    }

    ~MasherMovie()
    {

    }

    virtual bool EndOfStream() override
    {
        return mAtEndOfStream;
    }

    virtual bool NeedBuffer() override
    {
        return (mVideoBuffer.size() == 0 || mAudioBuffer.size() < (mMasher->SingleAudioFrameSizeSamples()*2*2*4)) && !mAtEndOfStream;
    }

    virtual void FillBuffers() override
    {
        while (NeedBuffer())
        {
            const s32 kNumChannels = 2;
            std::vector<u8> decodedAudioFrame(mMasher->SingleAudioFrameSizeSamples() * kNumChannels * sizeof(s16));
            mAtEndOfStream = !mMasher->Update((u32*)mFramePixels.data(), decodedAudioFrame.data());
            if (!mAtEndOfStream)
            {
                // Copy to audio threads buffer
                for (size_t i = 0; i < decodedAudioFrame.size(); i++)
                {
                    mAudioBuffer.push_back(decodedAudioFrame[i]);
                }

                mVideoBuffer.push_back(Frame{ mFrameCounter++, mMasher->Width(), mMasher->Height(), mFramePixels });
            }
            else
            {
                break;
            }
        }
    }

private:
    bool mAtEndOfStream = false;
    std::unique_ptr<Oddlib::Masher> mMasher;
    std::vector<u8> mFramePixels;
};


/*static*/ std::unique_ptr<IMovie> IMovie::Factory(
    const std::string& resourceName,
    IAudioController& audioController,
    std::unique_ptr<Oddlib::IStream> stream,
    std::unique_ptr<SubTitleParser> subTitles,
    u32 startSector, u32 endSector)
{

    char idBuffer[4] = {};
    stream->Read(idBuffer);
    stream->Seek(0);
    std::string idStr(idBuffer, 3);
    if (idStr == "DDV")
    {
        return std::make_unique<MasherMovie>(resourceName, audioController, std::move(stream), std::move(subTitles));
    }
    else if (idStr == "MOI")
    {
        return std::make_unique<DDVMovie>(resourceName, audioController, std::move(stream), std::move(subTitles));
    }
    else
    {
        const auto numberofSectors = endSector - startSector;
        return std::make_unique<MovMovie>(resourceName, audioController, std::move(stream), std::move(subTitles), startSector, numberofSectors);
    }
}

/*static*/ void Fmv::RegisterScriptBindings()
{
    Sqrat::Class<Fmv, Sqrat::NoConstructor<Fmv>> c(Sqrat::DefaultVM::Get(), "Fmv");
    c.Func("Play", &Fmv::Play);
    c.Func("IsPlaying", &Fmv::IsPlaying);
    c.Func("Stop", &Fmv::Stop);
    Sqrat::RootTable().Bind("Fmv", c);
}

Fmv::Fmv(IAudioController& audioController, ResourceLocator& resourceLocator)
    : mResourceLocator(resourceLocator), mAudioController(audioController), mScriptInstance("gMovie", this)
{
}

Fmv::~Fmv()
{
    Stop();
}

void Fmv::Play(const std::string& name)
{
    if (mFmvName != name)
    {
        mFmvName = name;
        if (mFmv)
        {
            mFmv->Stop();
        }

        mFmv = mResourceLocator.LocateFmv(mAudioController, name.c_str());
        
        if (mFmv)
        {
            mFmv->Start();
        }
        else
        {
            LOG_WARNING("Video: " + name + " was not found");
        }
        
    }
}

bool Fmv::IsPlaying() const
{
    return mFmv && !mFmv->IsEnd();
}

void Fmv::Stop()
{
    if (mFmv)
    {
        mFmv->Stop();
    }
    mFmv = nullptr;
    mFmvName.clear();
}

void Fmv::Update()
{
    if (!IsPlaying())
    {
        Stop();
    }

    if (Debugging().mBrowserUi.fmvBrowserOpen)
    {
        DebugUi();
    }
}

void Fmv::Render(AbstractRenderer& rend)
{
    if (Debugging().mSubtitleTestRegular)
    {
        RenderSubtitles(rend, "Regular subtitle test", 0.0f, 0.0f, static_cast<f32>(rend.Width()), static_cast<f32>(rend.Height()));
    }

    if (Debugging().mSubtitleTestItalic)
    {
        RenderSubtitles(rend, "<i>Italic subtitle test</i>", 0.0f, 0.0f, static_cast<f32>(rend.Width()), static_cast<f32>(rend.Height()));
    }

    if (Debugging().mSubtitleTestBold)
    {
        RenderSubtitles(rend, "<b>Bold subtitle test</b>", 0.0f, 0.0f, static_cast<f32>(rend.Width()), static_cast<f32>(rend.Height()));
    }

    if (Debugging().mDrawFontAtlas)
    {
        rend.FontStashTextureDebug(10.0f, 10.0f);
    }

    if (mFmv)
    {
        mFmv->OnRenderFrame(rend);
    }
}

void Fmv::DebugUi()
{
    static char mFilterString[64] = {};
    static int mListBoxSelectedItem = -1;
    static std::vector<const char*> mListBoxItems;

    if (ImGui::Begin("Video player"))
    {
        bool rebuild = false;
        if (ImGui::InputText("Filter", mFilterString, sizeof(mFilterString)))
        {
            rebuild = true;
        }

        if (rebuild || mListBoxItems.empty())
        {
            mListBoxItems.clear();
            mListBoxItems.reserve(mResourceLocator.mResMapper.mFmvMaps.size());

            for (const auto& fmv : mResourceLocator.mResMapper.mFmvMaps)
            {
                if (string_util::StringFilter(fmv.first.c_str(), mFilterString))
                {
                    mListBoxItems.emplace_back(fmv.first.c_str());
                }
            }
        }

        for (size_t i = 0; i < mListBoxItems.size(); i++)
        {
            if (ImGui::Selectable(mListBoxItems[i], static_cast<int>(i) == mListBoxSelectedItem))
            {
                mListBoxSelectedItem = static_cast<int>(i);
            }
        }

        if (mListBoxSelectedItem >= 0 && mListBoxSelectedItem < static_cast<int>(mListBoxItems.size()))
        {
            const std::string fmvName = mListBoxItems[mListBoxSelectedItem];
            Play(fmvName);
            mListBoxSelectedItem = -1;
        }
    }
    ImGui::End();
}