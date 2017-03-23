#include "fmv.hpp"
#include "gui.h"
#include "logger.hpp"
#include "proxy_nanovg.h"
#include "stdthread.h"
#include "renderer.hpp"
#include "resourcemapper.hpp"
#include "cdromfilesystem.hpp"

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

static void RenderSubtitles(Renderer& rend, const char* msg, float x, float y, float w, float h)
{
    f32 xpos = 0.0f;
    f32 ypos = y + h;

    rend.fillColor(ColourF32{ 0, 0, 0, 1 });
    rend.fontSize(Percent(h, 6.7f));
    rend.textAlign(TEXT_ALIGN_TOP);
    
    f32 bounds[4];
    rend.textBounds(static_cast<int>(xpos), static_cast<int>(ypos), msg, bounds);

    //f32 fontX = bounds[0];
    //f32 fontY = bounds[1];
    f32 fontW = bounds[2] - bounds[0];
    f32 fontH = bounds[3] - bounds[1];

    // Move off the bottom of the screen by half the font height
    ypos -= fontH + (fontH/2);

    // Center XPos in the screenW
    xpos = x + (w / 2) - (fontW / 2);

    rend.text(xpos, ypos, msg, Renderer::eDebugUi+1);
    rend.fillColor(ColourF32{ 1, 1, 1, 1 });
    f32 adjust = Percent(h, 0.3f);
    rend.text(xpos - adjust, ypos - adjust, msg, Renderer::eDebugUi+1);
}


IMovie::IMovie(const std::string& resourceName, IAudioController& controller, std::unique_ptr<SubTitleParser> subtitles)
    : mAudioController(controller), mSubTitles(std::move(subtitles)), mName(resourceName)
{
    // TODO: Add to interface - must be added/removed outside of ctor/dtor due
    // to data race issues
    std::lock_guard<std::mutex> lock(mAudioBufferMutex);
    mAudioController.AddPlayer(this);
}

IMovie:: ~IMovie()
{
    // TODO: Data race fix
    std::lock_guard<std::mutex> lock(mAudioBufferMutex);
    mAudioController.RemovePlayer(this);
}


// Main thread context
void IMovie::OnRenderFrame(Renderer& rend, GuiContext &gui)
{
    // TODO: Populate mAudioBuffer and mVideoBuffer
    // for up to N buffered frames
    std::lock_guard<std::mutex> lock(mAudioBufferMutex);

    while (NeedBuffer())
    {
        FillBuffers();
    }

    /*
    int num = -1;
    if (!mVideoBuffer.empty())
    {
        num = mVideoBuffer.begin()->mFrameNum;
    }*/

    //        std::cout << "Playing frame num " << mVideoFrameIndex << " first buffered frame is " << num << " samples played " << (size_t)mConsumedAudioBytes << std::endl;

    // 10063 is audio freq of 37800/15fps = 2520 * num frames(6420) = 16178400

    // 37800/15
    //  37800/15fps = 2520 samples per frame, 5040 bytes
    //  6420 * 5040 = 32356800*2=64713600
    // 10063 is 64608768(total audio bytes) / 6420(num frames) = 10063 bytes per frame
    // Total audio bytes is 64607760
    // Total audio bytes is 64607760
    // IMovie::Play[E] Audio buffer underflow want 5040 bytes  have 1008 bytes
    // Total audio bytes is 64608768

    // 2016 samples every 4 sectors, num sectors = file size / 2048?
    
    const auto videoFrameIndex = mConsumedAudioBytes / mAudioBytesPerFrame;// 10063;
    //std::cout << "Total audio bytes is " << mConsumedAudioBytes << std::endl;
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
            RenderFrame(rend, gui, f.mW, f.mH, f.mPixels.data(), current_subs);
            played = true;
            break;
        }

    }

    if (!played && !mVideoBuffer.empty())
    {
        Frame& f = mVideoBuffer.front();
        RenderFrame(rend, gui, f.mW, f.mH, f.mPixels.data(), current_subs);
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

// Audio thread context, from IAudioPlayer
void IMovie::Play(u8* stream, u32 len)
{
    std::lock_guard<std::mutex> lock(mAudioBufferMutex);

    // Consume mAudioBuffer and update the amount of consumed bytes
    size_t have = mAudioBuffer.size()/sizeof(int16_t);
    size_t take = len/sizeof(f32);
    if (take > have)
    {
        // Buffer underflow - we don't have enough data to fill the requested buffer
        // audio glitches ahoy!
        LOG_ERROR("Audio buffer underflow want " << take << " samples " << " have " << have << " samples");
        take = have;
    }

    f32 *floatOutStream = reinterpret_cast<f32*>(stream);
    for (auto i = 0u; i < take; i++)
    {
        uint8_t low = mAudioBuffer[i*sizeof(int16_t)];
        uint8_t high = mAudioBuffer[i*sizeof(int16_t) + 1];
        int16_t fixed = (int16_t)(low | (high << 8));

        // TODO: Add a proper audio mixing alogrithm/API, this will clip/overflow and cause weridnes when
        // 2 streams of diff sample rates are mixed
        floatOutStream[i] += fixed / 32768.0f;
    }
    mAudioBuffer.erase(mAudioBuffer.begin(), mAudioBuffer.begin() + take*sizeof(int16_t));
    mConsumedAudioBytes += take*sizeof(int16_t);
}

void IMovie::RenderFrame(Renderer &rend, GuiContext& /*gui*/, int width, int height, const GLvoid *pixels, const char* subtitles)
{
    // TODO: Optimize - should update 1 texture rather than creating per frame
    int texhandle = rend.createTexture(GL_RGB, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels, true);
    
    rend.drawQuad(texhandle, 
        rend.CameraPosition().x - (rend.ScreenSize().x / 2), 
        rend.CameraPosition().y - (rend.ScreenSize().y / 2), 
        rend.ScreenSize().x, 
        rend.ScreenSize().y,
        Renderer::eDebugUi);
    
    rend.SetActiveLayer(Renderer::eDebugUi + 1);

    if (subtitles)
    {
        RenderSubtitles(rend, subtitles,
            0,
            0,
            static_cast<f32>(rend.Width()),
            static_cast<f32>(rend.Height()));
    }

    rend.destroyTexture(texhandle);
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

        const int kSampleRate = 37800;
        const int kFps = 15;

        mAudioController.SetAudioSpec(kSampleRate / kFps, kSampleRate);

        // TODO: Check the correctness of this
        int numFrames = static_cast<int>((mFmvStream->Size()/10) / 2048);
        mAudioBytesPerFrame = (4 * kSampleRate)*(numFrames / kFps) / numFrames;

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
        // TODO Calculate constant
        return (mVideoBuffer.size() == 0 || mAudioBuffer.size() < (10080 * 2)) && !mFmvStream->AtEnd();
    }

    virtual void FillBuffers() override
    {

        const int kXaFrameDataSize = 2016;
        const int kNumAudioChannels = 2;
        const int kBytesPerSample = 2;
        std::vector<s16> outPtr((kXaFrameDataSize * kNumAudioChannels * kBytesPerSample) / 2);
        
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
                    }
                }
                else
                {
                     mAdpcm.DecodeFrameToPCM(outPtr, (uint8_t *)&w.mAkikMagic);
                }
              
                for (auto i = 0u; i < outPtr.size(); i++)
                {
                    mAudioBuffer.push_back(outPtr[i] & 0xFF);
                    mAudioBuffer.push_back(static_cast<unsigned char>(outPtr[i] >> 8));
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
                    pixelBuffer.resize(frameW * frameH* 4); // 4 bytes per pixel

                    mMdec.DecodeFrameToABGR32((uint16_t*)pixelBuffer.data(), (uint16_t*)mDemuxBuffer.data(), frameW, frameH);
                    mVideoBuffer.push_back(Frame{ mFrameCounter++, frameW, frameH, pixelBuffer });

                    return;
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
        mAudioBytesPerFrame = 10063; // TODO: Calculate
        mAudioController.SetAudioSpec(37800/15, 37800);
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
            mAudioController.SetAudioSpec(static_cast<u16>(mMasher->SingleAudioFrameSizeSamples()), mMasher->AudioSampleRate());
        }

        if (mMasher->HasVideo())
        {
            mFramePixels.resize(mMasher->Width() * mMasher->Height() * 4);
        }

        // 18827760 - 18821880 = 5880/4 = 1470 extra samples (half the SingleAudioFrameSizeSamples).

        mAudioBytesPerFrame = (4 * mMasher->AudioSampleRate())*(mMasher->NumberOfFrames() / mMasher->FrameRate()) / mMasher->NumberOfFrames();

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

static bool guiStringFilter(const char *haystack, const char *needle)
{
    if (needle[0] == '\0')
        return true;

    // Case-insensitive substring search
    size_t haystack_len = strlen(haystack);
    size_t needle_len = strlen(needle);
    bool matched = false;
    for (size_t i = 0; i + needle_len < haystack_len + 1; ++i)
    {
        matched = true;
        for (size_t k = 0; k < needle_len; ++k)
        {
            assert(k < needle_len);
            assert(i + k < haystack_len);
            if (tolower(needle[k]) != tolower(haystack[i + k]))
            {
                matched = false;
                break;
            }
        }
        if (matched)
            break;
    }
    return matched;
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

}

void Fmv::Play(const std::string& name)
{
    if (mFmvName != name)
    {
        mFmvName = name;
        mFmv = mResourceLocator.LocateFmv(mAudioController, name.c_str());
        if (!mFmv)
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
    mFmv = nullptr;
    mFmvName.clear();
}

void Fmv::Update()
{
    if (!IsPlaying())
    {
        Stop();
    }
}

void Fmv::Render(Renderer& rend, GuiContext& gui)
{
    if (mFmv)
    {
        mFmv->OnRenderFrame(rend, gui);
    }

    if (Debugging().mBrowserUi.fmvBrowserOpen)
    {
        DebugUi(gui);
    }
}

void Fmv::DebugUi(GuiContext& gui)
{
    static char mFilterString[64] = {};
    static int mListBoxSelectedItem = -1;
    static std::vector<const char*> mListBoxItems;

    gui_begin_window(&gui, "Video player");

    bool rebuild = false;
    if (gui_textfield(&gui, "Filter", mFilterString, sizeof(mFilterString)))
    {
        rebuild = true;
    }


    if (rebuild || mListBoxItems.empty())
    {
        mListBoxItems.clear();
        mListBoxItems.reserve(mResourceLocator.mResMapper.mFmvMaps.size());

        for (const auto& fmv : mResourceLocator.mResMapper.mFmvMaps)
        {
            if (guiStringFilter(fmv.first.c_str(), mFilterString))
            {
                mListBoxItems.emplace_back(fmv.first.c_str());
            }
        }
    }

    for (size_t i = 0; i < mListBoxItems.size(); i++)
    {
        if (gui_selectable(&gui, mListBoxItems[i], static_cast<int>(i) == mListBoxSelectedItem))
        {
            mListBoxSelectedItem = static_cast<int>(i);
        }
    }

    if (mListBoxSelectedItem >= 0 && mListBoxSelectedItem < static_cast<int>(mListBoxItems.size()))
    {
        const std::string fmvName = mListBoxItems[mListBoxSelectedItem];
        mFmv = mResourceLocator.LocateFmv(mAudioController, fmvName.c_str());
        mListBoxSelectedItem = -1;
    }

    gui_end_window(&gui);
}