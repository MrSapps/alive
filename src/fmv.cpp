#include "fmv.hpp"
#include "imgui/imgui.h"
#include "core/audiobuffer.hpp"
#include "oddlib/cdromfilesystem.hpp"
#include "gamedata.hpp"
#include "logger.hpp"
#include "SDL_opengl.h"
#include "oddlib/audio/SequencePlayer.h"
#include "filesystem.hpp"

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

class IMovie : public IAudioPlayer
{
public:
    IMovie(IAudioController& controller) 
        : mAudioController(controller)
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
    void OnRenderFrame()
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
        std::cout << "Total audio bytes is " << mConsumedAudioBytes << std::endl;

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
    virtual void Play(Uint8* stream, Sint32 len) override
    {
        std::lock_guard<std::mutex> lock(mAudioBufferMutex);

        // Consume mAudioBuffer and update the amount of consumed bytes
        size_t have = mAudioBuffer.size();
        size_t take = len;
        if (len > have)
        {
            // Buffer underflow - we don't have enough data to fill the requested buffer
            // audio glitches ahoy!
            LOG_ERROR("Audio buffer underflow want " << len << " bytes " << " have " << have << " bytes");
            take = have;
        }

        for (int i = 0; i < take; i++)
        {
            stream[i] = mAudioBuffer[i];
        }
        mAudioBuffer.erase(mAudioBuffer.begin(), mAudioBuffer.begin() + take);
        mConsumedAudioBytes += take;
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
        int X = -1;
        int Y = -1;
        int Width = 2;
        int Height = 2;

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
private:
    //AutoMouseCursorHide mHideMouseCursor;
};

// PSX MOV/STR format, all PSX game versions use this.
class MovMovie : public IMovie
{
protected:
    std::unique_ptr<Oddlib::IStream> mFmvStream;
    bool mPsx = false;
    MovMovie(IAudioController& audioController)
        : IMovie(audioController)
    {

    }
public:
    MovMovie(IAudioController& audioController, std::unique_ptr<Oddlib::IStream> stream)
        : IMovie(audioController)
    {
        mFmvStream = std::move(stream);

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
        unsigned int numSectorsToRead = 0;
        unsigned int sectorNumber = 0;

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
                const Uint32 frameW = w.mWidth;
                const Uint32 frameH = w.mHeight;

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
    DDVMovie(IAudioController& audioController, std::unique_ptr<Oddlib::IStream> stream)
        : MovMovie(audioController)
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
    MasherMovie(IAudioController& audioController, std::unique_ptr<Oddlib::IStream> stream)
        : IMovie(audioController)
    {
        mMasher = std::make_unique<Oddlib::Masher>(std::move(stream));

        if (mMasher->HasAudio())
        {
            mAudioController.SetAudioSpec(mMasher->SingleAudioFrameSizeSamples(), mMasher->AudioSampleRate());
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

std::unique_ptr<SequencePlayer> player;
bool firstChange = true;
int targetSong = -1;
bool loopSong = false;

void BarLoop()
{
    printf("Bar Loop!");

    if (targetSong != -1)
    {
        if (player->LoadSequenceData(AliveAudio::m_LoadedSeqData[targetSong]) == 0)
        {
            player->PlaySequence();
        }
        targetSong = -1;
    }
}


void ChangeTheme(void *clientData)
{
    //player->StopSequence();

    jsonxx::Object theme = AliveAudio::m_Config.get<jsonxx::Array>("themes").get<jsonxx::Object>(14);

    Oddlib::LvlArchive archive(theme.get<jsonxx::String>("lvl", "null") + ".LVL");
    AliveAudio::LoadAllFromLvl(archive, theme.get<jsonxx::String>("vab", "null"), theme.get<jsonxx::String>("seq", "null"));

    //TwRemoveAllVars(m_GUIFileList);
    for (int i = 0; i < archive.FileCount(); i++)
    {
        char labelTest[100];
//        sprintf(labelTest, "group='Files' label='%s'", archive.mFiles[i].get()->FileName().c_str());
        //TwAddButton(m_GUIFileList, nullptr, nullptr, nullptr, labelTest);
    }

   // TwRemoveAllVars(m_GUITones);
    for (int e = 0; e < 128; e++)
    {
        for (int i = 0; i < AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones.size(); i++)
        {
            char labelTest[100];
            sprintf(labelTest, "group='Program %i' label='%i - Min:%i Max:%i'", e, i, AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones[i]->Min, AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones[i]->Max);
           // TwAddButton(m_GUITones, nullptr, PlaySound, (void*)new int[2]{ e, i }, labelTest);
        }
    }

   // TwRemoveAllVars(m_GUISequences);
    for (int i = 0; i < AliveAudio::m_LoadedSeqData.size(); i++)
    {
        char labelTest[100];
        sprintf(labelTest, "group='Seq Files' label='Play Seq %i'", i);

       // TwAddButton(m_GUISequences, nullptr, PlaySong, (char*)i, labelTest);
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
    FmvUi(std::unique_ptr<class IMovie>& fmv, IAudioController& audioController, FileSystem& fs)
        : mFmv(fmv), mAudioController(audioController), mFileSystem(fs)
    {
    }

    void DrawVideoSelectionUi(const std::string& setName, const std::vector<std::string>& allFmvs)
    {
        std::string name = "Video player (" + setName + ")";
        ImGui::Begin(name.c_str(), nullptr, ImVec2(550, 580), 1.0f, ImGuiWindowFlags_NoCollapse);

        mFilter.Draw();



        listbox_items.resize(allFmvs.size());

        int matchingFilter = 0;
        for (size_t i = 0; i < allFmvs.size(); i++)
        {
            if (mFilter.PassFilter(allFmvs[i].c_str()))
            {
                listbox_items[matchingFilter] = allFmvs[i].c_str();
                matchingFilter++;
            }
        }
        ImGui::PushItemWidth(-1);
        ImGui::ListBox("##", &listbox_item_current, listbox_items.data(), matchingFilter, 27);

        if (ImGui::Button("Music test"))
        {
            int id = 24; // death sound
            player.reset(new SequencePlayer());
            player->m_QuarterCallback = BarLoop;
            ChangeTheme(0);
            if (firstChange || player->m_PlayerState == ALIVE_SEQUENCER_FINISHED || player->m_PlayerState == ALIVE_SEQUENCER_STOPPED)
            {
                if (player->LoadSequenceData(AliveAudio::m_LoadedSeqData[id]) == 0)
                {
                    player->PlaySequence();
                    firstChange = false;
                }
            }
            else
            {
                targetSong = (int)id;
            }
        }

        if (ImGui::Button("Play", ImVec2(ImGui::GetWindowWidth(), 20)))
        {
            try
            {
                // TODO: Clean up
                const std::string fmvName = listbox_items[listbox_item_current];
                auto stream = mFileSystem.OpenResource(fmvName);
                char idBuffer[4] = {};
                stream->ReadBytes(reinterpret_cast<Sint8*>(idBuffer), sizeof(idBuffer));
                stream->Seek(0);
                std::string idStr(idBuffer, 3);
                if (idStr == "DDV")
                {
                    mFmv = std::make_unique<MasherMovie>(mAudioController, std::move(stream));
                }
                else if (idStr == "MOI")
                {
                    mFmv = std::make_unique<DDVMovie>(mAudioController, std::move(stream));
                }
                else
                {
                    mFmv = std::make_unique<MovMovie>(mAudioController, std::move(stream));
                }

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

void Fmv::RenderVideoUi()
{
    if (!mFmv)
    {
        if (mFmvUis.empty())
        {
            auto fmvs = mGameData.Fmvs();
            for (auto fmvSet : fmvs)
            {
                mFmvUis.emplace_back(std::make_unique<FmvUi>(mFmv, mAudioController, mFileSystem));
            }
        }

        if (!mFmvUis.empty())
        {
            int i = 0;
            auto fmvs = mGameData.Fmvs();
            for (auto fmvSet : fmvs)
            {
                mFmvUis[i]->DrawVideoSelectionUi(fmvSet.first, fmvSet.second);
                i++;
            }
        }
    }
}

Fmv::Fmv(GameData& gameData, IAudioController& audioController, FileSystem& fs)
    : mGameData(gameData), mAudioController(audioController), mFileSystem(fs)
{

}

Fmv::~Fmv()
{

}

void Fmv::Play()
{

}

void Fmv::Stop()
{
    mFmv = nullptr;
}

void Fmv::Update()
{

}

void Fmv::Render()
{
    glEnable(GL_TEXTURE_2D);

    RenderVideoUi();

    if (mFmv)
    {
        mFmv->OnRenderFrame();
        if (mFmv->IsEnd())
        {
            mFmv = nullptr;
        }
    }
}
