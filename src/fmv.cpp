#include "fmv.hpp"
#include "imgui/imgui.h"
#include "core/audiobuffer.hpp"
#include "oddlib/cdromfilesystem.hpp"
#include "gamedata.hpp"
#include "logger.hpp"
#include "filesystem.hpp"
#include "subtitles.hpp"
#include <GL/glew.h>
#include "SDL_opengl.h"
#include "nanovg.h"
#include "stdthread.h"

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

static float Percent(float max, float percent)
{
    return (max / 100.0f) * percent;
}

static void RenderSubtitles(NVGcontext* ctx, const char* msg, int screenW, int screenH)
{
    float xpos = 0.0f;
    float ypos = static_cast<float>(screenH);

    nvgFillColor(ctx, nvgRGBA(0, 0, 0, 255));
    nvgFontSize(ctx, Percent(static_cast<float>(screenH), 6.7f));

    nvgTextAlign(ctx, NVG_ALIGN_TOP);
    
    float bounds[4];
    nvgTextBounds(ctx, xpos, ypos, msg, nullptr, bounds);

    //float fontX = bounds[0];
    //float fontY = bounds[1];
    float fontW = bounds[2] - bounds[0];
    float fontH = bounds[3] - bounds[1];

    // Move off the bottom of the screen by half the font height
    ypos -= fontH + (fontH/2);

    // Center XPos in the screenW
    xpos = (screenW / 2) - (fontW / 2);

    nvgText(ctx, xpos, ypos, msg, nullptr);

    nvgFillColor(ctx, nvgRGBA(255, 255, 255, 255));
    float adjust = Percent(static_cast<float>(screenH), 0.3f);
    nvgText(ctx, xpos - adjust, ypos - adjust, msg, nullptr);

}

class IMovie : public IAudioPlayer
{
public:
    static std::unique_ptr<IMovie> Factory(const std::string& fmvName, IAudioController& audioController, FileSystem& fs, const std::map<std::string, std::vector<GameData::FmvSection>>& allFmvs);

    IMovie(IAudioController& controller, std::unique_ptr<SubTitleParser> subtitles)
        : mAudioController(controller), mSubTitles(std::move(subtitles))
    {
        // TODO: Add to interface - must be added/removed outside of ctor/dtor due
        // to data race issues
        std::lock_guard<std::mutex> lock(mAudioBufferMutex);
        mAudioController.AddPlayer(this);
    }

    virtual ~IMovie()
    {
        // TODO: Data race fix
        std::lock_guard<std::mutex> lock(mAudioBufferMutex);
        mAudioController.RemovePlayer(this);
    }

    virtual bool EndOfStream() = 0;
    virtual bool NeedBuffer() = 0;
    virtual void FillBuffers() = 0;

    // Main thread context
    void OnRenderFrame(NVGcontext* ctx, int screenW, int screenH)
    {
        // TODO: Populate mAudioBuffer and mVideoBuffer
        // for up to N buffered frames
        std::lock_guard<std::mutex> lock(mAudioBufferMutex);

        while (NeedBuffer())
        {
            FillBuffers();
        }

        int num = -1;
        if (!mVideoBuffer.empty())
        {
            num = mVideoBuffer.begin()->mFrameNum;
        }

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
        if (mSubTitles)
        {
            // We assume the FPS is always 15, thus 1000/15=66.66 so frame number * 66 = number of msecs into the video
            const auto& subs = mSubTitles->Find((videoFrameIndex * 66));
            if (!subs.empty())
            {
                // TODO: Render all active subs, not just the first one
                const char* msg = subs[0]->Text().c_str();
                RenderSubtitles(ctx, msg, screenW, screenH);
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
                mVideoBuffer.pop_front();
                continue;
            }

            if (f.mFrameNum == videoFrameIndex)
            {
                mLast = f;
                RenderFrame(f.mW, f.mH, f.mPixels.data());
                played = true;
                break;
            }

        }

        if (!played && !mVideoBuffer.empty())
        {
            Frame& f = mVideoBuffer.front();
            RenderFrame(f.mW, f.mH, f.mPixels.data());
        }

        if (!played && mVideoBuffer.empty())
        {
            Frame& f = mLast;
            RenderFrame(f.mW, f.mH, f.mPixels.data());
        }

        while (NeedBuffer())
        {
            FillBuffers();
        }



    }

    // Main thread context
    bool IsEnd()
    {
        std::lock_guard<std::mutex> lock(mAudioBufferMutex);
        const auto ret = EndOfStream() && mAudioBuffer.empty();
        if (ret && !mVideoBuffer.empty())
        {
            LOG_ERROR("Still " << mVideoBuffer.size() << " frames left after audio finished");
        }
        return ret;
    }

protected:
    // Audio thread context, from IAudioPlayer
    virtual void Play(Uint8* stream, Uint32 len) override
    {
        std::lock_guard<std::mutex> lock(mAudioBufferMutex);

        // Consume mAudioBuffer and update the amount of consumed bytes
        size_t have = mAudioBuffer.size()/sizeof(int16_t);
        size_t take = len/sizeof(float);
        if (take > have)
        {
            // Buffer underflow - we don't have enough data to fill the requested buffer
            // audio glitches ahoy!
            LOG_ERROR("Audio buffer underflow want " << take << " samples " << " have " << have << " samples");
            take = have;
        }

        float *floatOutStream = reinterpret_cast<float*>(stream);
        for (auto i = 0u; i < take; i++)
        {
            uint8_t low = mAudioBuffer[i*sizeof(int16_t)];
            uint8_t high = mAudioBuffer[i*sizeof(int16_t) + 1];
            int16_t fixed = (int16_t)(low | (high << 8));
            floatOutStream[i] = fixed / 32768.0f;
        }
        mAudioBuffer.erase(mAudioBuffer.begin(), mAudioBuffer.begin() + take*sizeof(int16_t));
        mConsumedAudioBytes += take*sizeof(int16_t);
    }

    void RenderFrame(int width, int height, const GLvoid *pixels)
    {
        // TODO: Optimize - should use VBO's & update 1 texture rather than creating per frame
        GLuint TextureID = 0;
        glGenTextures(1, &TextureID);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // For Ortho mode, of course
        GLfloat X = -1.0f;
        GLfloat Y = -1.0f;
        GLfloat Width = 2.0f;
        GLfloat Height = 2.0f;

        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(X, Y, 0);
        glTexCoord2f(1, 0); glVertex3f(X + Width, Y, 0);
        glTexCoord2f(1, -1); glVertex3f(X + Width, Y + Height, 0);
        glTexCoord2f(0, -1); glVertex3f(X, Y + Height, 0);
        glEnd();

        glDeleteTextures(1, &TextureID);
    }

protected:
    struct Frame
    {
        size_t mFrameNum;
        Uint32 mW;
        Uint32 mH;
        std::vector<Uint8> mPixels;
    };
    Frame mLast;
    size_t mFrameCounter = 0;
    size_t mConsumedAudioBytes = 0;
    std::mutex mAudioBufferMutex;
    std::deque<Uint8> mAudioBuffer;
    std::deque<Frame> mVideoBuffer;
    IAudioController& mAudioController;
    int mAudioBytesPerFrame = 1;
    std::unique_ptr<SubTitleParser> mSubTitles;
private:
    //AutoMouseCursorHide mHideMouseCursor;
};

// PSX MOV/STR format, all PSX game versions use this.
class MovMovie : public IMovie
{
protected:
    std::unique_ptr<Oddlib::IStream> mFmvStream;
    bool mPsx = false;
    MovMovie(IAudioController& audioController, std::unique_ptr<SubTitleParser> subtitles)
        : IMovie(audioController, std::move(subtitles))
    {

    }
public:
    MovMovie(IAudioController& audioController, std::unique_ptr<Oddlib::IStream> stream, std::unique_ptr<SubTitleParser> subtitles, Uint32 startSector, Uint32 numberOfSectors)
        : IMovie(audioController, std::move(subtitles))
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
        int numFrames = (mFmvStream->Size()/10) / 2048;
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
        std::vector<unsigned char> outPtr(kXaFrameDataSize * kNumAudioChannels * kBytesPerSample);
        
        if (mDemuxBuffer.empty())
        {
            mDemuxBuffer.resize(1024 * 1024);
        }

        std::vector<Uint8> pixelBuffer;
        for (;;)
        {

            PsxStrHeader w;
            if (mFmvStream->AtEnd())
            {
                return;
            }
            mFmvStream->ReadBytes(reinterpret_cast<Uint8*>(&w), sizeof(w));

            // PC sector must start with "MOIR" if video, else starts with "VALE"
            if (!mPsx && w.mSectorType != 0x52494f4d)
            {
                // abort();
            }

            // AKIK is 0x80010160 in PSX
            const auto kMagic = mPsx ? 0x80010160 : 0x4b494b41;
            if (w.mAkikMagic != kMagic)
            {
                uint16_t numBytes = 0;
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
                        numBytes = mAdpcm.DecodeFrameToPCM((int8_t *)outPtr.data(), &rawXa->data[0], true);
                    }
                    else
                    {
                        // Blank/empty audio frame, play silence so video stays in sync
                        numBytes = 2016 * 2 * 2;
                    }
                }
                else
                {
                    numBytes = mAdpcm.DecodeFrameToPCM((int8_t *)outPtr.data(), (uint8_t *)&w.mAkikMagic, true);
                }

                for (int i = 0; i < numBytes; i++)
                {
                    mAudioBuffer.push_back(outPtr[i]);
                }

                // Must be VALE
                continue;
            }
            else
            {
                const Uint16 frameW = w.mWidth;
                const Uint16 frameH = w.mHeight;

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

                    mMdec.DecodeFrameToABGR32((uint16_t*)pixelBuffer.data(), (uint16_t*)mDemuxBuffer.data(), frameW, frameH, false);
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
    DDVMovie(IAudioController& audioController, std::unique_ptr<Oddlib::IStream> stream, std::unique_ptr<SubTitleParser> subtitles)
        : MovMovie(audioController, std::move(subtitles))
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
    MasherMovie(IAudioController& audioController, std::unique_ptr<Oddlib::IStream> stream, std::unique_ptr<SubTitleParser> subtitles)
        : IMovie(audioController, std::move(subtitles))
    {
        mMasher = std::make_unique<Oddlib::Masher>(std::move(stream));

        if (mMasher->HasAudio())
        {
            mAudioController.SetAudioSpec(static_cast<Uint16>(mMasher->SingleAudioFrameSizeSamples()), mMasher->AudioSampleRate());
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
            std::vector<Uint8> decodedAudioFrame(mMasher->SingleAudioFrameSizeSamples() * 2 * 2); // *2 if stereo
            mAtEndOfStream = !mMasher->Update((Uint32*)mFramePixels.data(), decodedAudioFrame.data());
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
    std::vector<Uint8> mFramePixels;
};

/*static*/ std::unique_ptr<IMovie> IMovie::Factory(
    const std::string& fmvName, 
    IAudioController& audioController, 
    FileSystem& fs, 
    const std::map<std::string, std::vector<GameData::FmvSection>>& allFmvs)
{
    TRACE_ENTRYEXIT;

    // TODO: PSX fmv seems to have a lot of "black" at the end - probably json data is incorrect or needs tweaking?
    std::string targetName = fmvName;
    Uint32 startSector = 0;
    Uint32 numberOfSectors = 0;

    // Check the PC name
    int pcFmvPriority = 0;
    auto resourceLocation = fs.ResourceExists(fmvName, pcFmvPriority);

    // Find the mapping of PSX -> PC fmv names from the json data
    auto fmvData = allFmvs.find(fmvName);
    if (fmvData != std::end(allFmvs))
    {
        // Check if the PSX file containing the FMV exists
        const std::vector<GameData::FmvSection>& sections = fmvData->second;
        for (const GameData::FmvSection& section : sections)
        {
            // For BR.MOV there is a slight hack to pick the biggest file because BR.MOV on CD1 isn't 0 bytes for some reason
            // and we always want the CD2 BR.MOV which is bigger.
            int psxFmvPriority = 0;
            auto tmpResourceLocation = fs.ResourceExists(section.mPsxFileName, psxFmvPriority, section.mPsxFileName == "BR\\BR.MOV");
            // Only pick PSX FMV over the PC version when either its the only thing we have
            // or when we have both but the PSX data is marked higher priority than the PC data.
            if (tmpResourceLocation && (!resourceLocation || (resourceLocation && psxFmvPriority > pcFmvPriority)))
            {
                resourceLocation = tmpResourceLocation;
                targetName = section.mPsxFileName;
                startSector = section.mStartSector;
                numberOfSectors = section.mNumberOfSectors;
                break;
            }
        }
    }
   
    if (!resourceLocation)
    {
        throw Oddlib::Exception("FMV not found");
    }

    auto stream = resourceLocation->Open(targetName);

    // Try to open any corresponding subtitle file
    const std::string subTitleFileName = "data/subtitles/" + fmvName + ".SRT";
    std::unique_ptr<SubTitleParser> subTitles;
    if (fs.Exists(subTitleFileName))
    {
        subTitles = std::make_unique<SubTitleParser>(fs.Open(subTitleFileName));
    }

    char idBuffer[4] = {};
    stream->ReadBytes(reinterpret_cast<Sint8*>(idBuffer), sizeof(idBuffer));
    stream->Seek(0);
    std::string idStr(idBuffer, 3);
    if (idStr == "DDV")
    {
        return std::make_unique<MasherMovie>(audioController, std::move(stream), std::move(subTitles));
    }
    else if (idStr == "MOI")
    {
        return std::make_unique<DDVMovie>(audioController, std::move(stream), std::move(subTitles));
    }
    else
    {
        // Only PSX FMV's have many in a single file
        return std::make_unique<MovMovie>(audioController, std::move(stream), std::move(subTitles), startSector, numberOfSectors);
    }
}

class FmvUi
{
private:
    ImGuiTextFilter mFilter;
    int listbox_item_current = 0;
    std::vector<const char*> listbox_items;
    std::unique_ptr<class IMovie>& mFmv;
public:
    FmvUi(const FmvUi&) = delete;
    FmvUi& operator = (const FmvUi&) = delete;
    FmvUi(std::unique_ptr<class IMovie>& fmv, IAudioController& audioController, FileSystem& fs)
        : mFmv(fmv), mAudioController(audioController), mFileSystem(fs)
    {
    }

    void DrawVideoSelectionUi(const std::map<std::string, std::vector<GameData::FmvSection>>& allFmvs)
    {
        std::string name = "Video player";
        static bool bSet = false;
        if (!bSet)
        {
            ImGui::SetNextWindowPos(ImVec2(720, 40));
            bSet = true;
        }
        ImGui::Begin(name.c_str(), nullptr, ImVec2(550, 580), 1.0f, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

        mFilter.Draw();


        listbox_items.clear();
        listbox_items.reserve(allFmvs.size());

        for (const auto& fmv : allFmvs)
        {
            if (mFilter.PassFilter(fmv.first.c_str()))
            {
                listbox_items.emplace_back(fmv.first.c_str());
            }
        }

        if (ImGui::ListBoxHeader("##", ImVec2(ImGui::GetWindowWidth() - 15, ImGui::GetWindowSize().y - 95)))
        {
            if (listbox_item_current >= static_cast<int>(listbox_items.size()))
            {
                listbox_item_current = 0;
            }

            for (size_t i = 0; i < listbox_items.size(); i++)
            {
                if (ImGui::Selectable(listbox_items[i], static_cast<int>(i) == listbox_item_current))
                {
                    listbox_item_current = i;
                }
            }
            ImGui::ListBoxFooter();
        }


        if (ImGui::Button("Play", ImVec2(ImGui::GetWindowWidth(), 20)) && !listbox_items.empty())
        {
            try
            {
                const std::string fmvName = listbox_items[listbox_item_current];
                mFmv = IMovie::Factory(fmvName, mAudioController, mFileSystem, allFmvs);
            }
            catch (const Oddlib::Exception& ex)
            {
                LOG_ERROR("Exception: " << ex.what());
            }
        }

        ImGui::End();
    }
private:
    IAudioController& mAudioController;
    FileSystem& mFileSystem;
};


Fmv::Fmv(GameData& gameData, IAudioController& audioController, FileSystem& fs)
    : mGameData(gameData), mAudioController(audioController), mFileSystem(fs)
{
    // TODO: Should probably be handled by something else
    glEnable(GL_TEXTURE_2D);
}

Fmv::~Fmv()
{

}

void Fmv::Play(const std::string& name)
{
    if (!mFmv)
    {
        try
        {
            mFmv = IMovie::Factory(name, mAudioController, mFileSystem, mGameData.Fmvs());
        }
        catch (const Oddlib::Exception& ex)
        {
            LOG_ERROR("Exception: " << ex.what());
        }
    }
}

bool Fmv::IsPlaying() const
{
    return mFmv != nullptr;
}

void Fmv::Stop()
{
    mFmv = nullptr;
}

void Fmv::Update()
{
    if (mFmv)
    {
        if (mFmv->IsEnd())
        {
            mFmv = nullptr;
        }
    }
}

void Fmv::Render(NVGcontext* ctx, int screenW, int screenH)
{
    if (mFmv)
    {
        mFmv->OnRenderFrame(ctx, screenW, screenH);
    }
}

DebugFmv::DebugFmv(GameData& gameData, IAudioController& audioController, FileSystem& fs) 
    : Fmv(gameData, audioController, fs)
{

}

DebugFmv::~DebugFmv()
{

}

void DebugFmv::Render(struct NVGcontext* ctx, int screenW, int screenH)
{
    Fmv::Render(ctx, screenW, screenH);

    if (!mFmv)
    {
        RenderVideoUi();
        mFileSystem.DebugUi();
    }
}

void DebugFmv::RenderVideoUi()
{
    if (!mFmv)
    {
        if (!mFmvUi)
        {
            mFmvUi = std::make_unique<FmvUi>(mFmv, mAudioController, mFileSystem);
        }
        mFmvUi->DrawVideoSelectionUi(mGameData.Fmvs());
    }
}
