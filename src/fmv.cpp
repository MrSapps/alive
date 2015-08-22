#include "fmv.hpp"
#include "imgui/imgui.h"
#include "core/audiobuffer.hpp"
#include "oddlib/cdromfilesystem.hpp"
#include "gamedata.hpp"
#include "logger.hpp"
#include "SDL_opengl.h"
#include "oddlib/audio/SequencePlayer.h"

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
    virtual ~IMovie() = default;
    virtual void OnRenderFrame() = 0;
    virtual bool IsEnd() = 0;
protected:
    void RenderFrame(int width, int height, const GLvoid *pixels)
    {
        // TODO: Optimize - should use VBO's & update 1 texture rather than creating per frame
        GLuint TextureID = 0;
        glGenTextures(1, &TextureID);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        int Mode = GL_RGB;

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
    std::mutex mAudioLock;

private:
    //AutoMouseCursorHide mHideMouseCursor;
};

int gFakeFrames = 0;

// PSX MOV/STR format, all PSX game versions use this.
class MovMovie : public IMovie
{
private:
    std::unique_ptr<Oddlib::Stream> gStream;
    std::unique_ptr<RawCdImage> gCd;
    std::vector<Uint32> mPixelBuffer;
protected:
    std::unique_ptr<Oddlib::IStream> mFmvStream;

    bool mPsx = false;
    MovMovie(IAudioController& audioController)
        : mAudioController(audioController)
    {

    }
public:
    MovMovie(const std::string& fullPath, IAudioController& audioController)
        : mAudioController(audioController)
    {
        gStream = std::make_unique<Oddlib::Stream>("C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 2) [SLES-11480].bin");
        gCd = std::make_unique<RawCdImage>(*gStream);
        gCd->LogTree();
        mFmvStream = gCd->ReadFile("BR\\BR.MOV", true);
        
        //mAudioController.SetAudioSpec(8064 / 4, 37800);
        mAudioController.SetAudioSpec(37800/15, 37800);

        auto fileSize = mFmvStream->Size();
        auto numFrames = fileSize / 2048;

        mPsx = true;

        // TODO: Add to interface - must be added/removed outside of ctor/dtor due
        // to data race issues
        mAudioController.AddPlayer(this);
    }

    ~MovMovie()
    {
        std::lock_guard<std::mutex> lock(mAudioBufferMutex);
        mAudioController.RemovePlayer(this);
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

    struct MasherVideoHeaderWrapper
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

    // Audio thread context
    virtual void Play(Uint8* stream, Sint32 len) override
    {
        std::lock_guard<std::mutex> lock(mAudioBufferMutex);

        // TODO: Consume mAudioBuffer and update the matching video frame number
        size_t have = mAudioBuffer.size();
        size_t take = len;
        if (len > have)
        {
            // Buffer underflow - we don't have enough data to fill the requested buffer
            // audio glitches ahoy!
            take = have;
        }

        for (int i = 0; i < take; i++)
        {
            stream[i] = mAudioBuffer[i];
        }
        mAudioBuffer.erase(mAudioBuffer.begin(), mAudioBuffer.begin() + take);
        mConsumedAudioBytes += take;
        mVideoFrameIndex = mConsumedAudioBytes / 10063;
    }

    // 64608768, 125
    // 64608768 / 10080 = 6409
    // 6409 + 11 = 6420
    // 64608768 / 6420 = 10063

    // (67 * 5) + 127 + (67*5)+  127+431+533+508+665+2387+972
    // = 6420 * 8064
    // 

    bool NeedBuffer()
    {
        return (mVideoBuffer.size() == 0 || mAudioBuffer.size() < (10080 * 2)) && !mFmvStream->AtEnd();
    }

    // Main thread context
    virtual void OnRenderFrame() override
    {
        // TODO: Populate mAudioBuffer and mVideoBuffer
        // for up to N buffered frames
        std::lock_guard<std::mutex> lock(mAudioBufferMutex);

        while (NeedBuffer())
        {
            BufferFrame();
        }

        // 10080 or 10320 ??


        int num = -1;
        if (!mVideoBuffer.empty())
        {
            num = mVideoBuffer.begin()->mFrameNum;
        }

//        std::cout << "Playing frame num " << mVideoFrameIndex << " first buffered frame is " << num << " samples played " << (size_t)mConsumedAudioBytes << std::endl;

        bool played = false;
        while (!mVideoBuffer.empty())
        {  
            Frame& f = mVideoBuffer.front();
            if (f.mFrameNum < mVideoFrameIndex)
            {
                mVideoBuffer.pop_front();
                continue;
            }

            if (f.mFrameNum == mVideoFrameIndex)
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
            BufferFrame();
        }
    }

    void BufferFrame()
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

            MasherVideoHeaderWrapper w;
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
                        gFakeFrames++;
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
                const int frameW = w.mWidth;
                const int frameH = w.mHeight;

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

    virtual bool IsEnd() override
    {
        // TODO: Don't have mVideoBuffer.empty() here - this will cope with if there
        // is 1 frame left over from audio to video rounding errors
        return mFmvStream->AtEnd() && mAudioBuffer.empty() && mVideoBuffer.empty();
    }

private:

    struct Frame
    {
        size_t mFrameNum;
        int mW;
        int mH;
        std::vector<Uint8> mPixels;
    };
    Frame mLast;


    size_t mFrameCounter = 0;
    std::atomic<size_t> mVideoFrameIndex;
    std::atomic<size_t> mConsumedAudioBytes;
    std::vector<unsigned char> mDemuxBuffer;
    std::mutex mAudioBufferMutex;
    std::deque<Uint8> mAudioBuffer;
    std::deque<Frame> mVideoBuffer;
    PSXMDECDecoder mMdec;
    PSXADPCMDecoder mAdpcm;
    IAudioController& mAudioController;
};

// Same as MOV/STR format but with modified magic in the video frames
// and XA audio raw cd sector information stripped. Only used in AO PC.
class DDVMovie : public MovMovie
{
public:
    DDVMovie(const std::string& fullPath, IAudioController& audioController)
        : MovMovie(audioController)
    {
        mPsx = false;
        mFmvStream = std::make_unique<Oddlib::Stream>(fullPath);
        // TODO: Update
        //AudioBuffer::ChangeAudioSpec(8064 / 4, 37800);
    }
};

// Also called DDV on disk confusingly. Not the same as DDVMovie used in AO PC, this
// type can be found in AE PC.
class MasherMovie : public IMovie
{
public:
    MasherMovie(const std::string& fileName)
    {
        LOG_INFO("Playing movie " << fileName);

        mMasher = std::make_unique<Oddlib::Masher>(fileName);
        
        if (mMasher->HasAudio())
        {
            // TODO: Update
           // AudioBuffer::ChangeAudioSpec(mMasher->SingleAudioFrameSizeBytes(), mMasher->AudioSampleRate());
        }

        if (mMasher->HasVideo())
        {
            mFrameSurface = SDL_CreateRGBSurface(0, mMasher->Width(), mMasher->Height(), 32, 0, 0, 0, 0);
            mFramePixels.resize(mMasher->Width() * mMasher->Height());
        }
    }

    ~MasherMovie()
    {
        if (mFrameSurface)
        {
            SDL_FreeSurface(mFrameSurface);
        }
    }

    virtual void OnRenderFrame() override
    {
        std::vector<Uint16> decodedFrame(mMasher->SingleAudioFrameSizeBytes() * 2); // *2 if stereo
        mEnd = !mMasher->Update(mFramePixels.data(), (Uint8*)decodedFrame.data());
        if (!mEnd)
        {
            RenderFrame(mMasher->Width(), mMasher->Height(), mFramePixels.data());
            /* TODO: Update
            AudioBuffer::SendSamples((char*)decodedFrame.data(), decodedFrame.size() * 2);
            while (AudioBuffer::mPlayedSamples < mMasher->FrameNumber() * mMasher->SingleAudioFrameSizeBytes())
            {

            }*/
        }
    }

    virtual bool IsEnd() override
    {
        return mEnd;
    }

private:
    bool mEnd = false;
    SDL_Surface* mFrameSurface = nullptr;
    std::unique_ptr<Oddlib::Masher> mMasher;
    std::vector<Uint32> mFramePixels;
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
        if (player->LoadSequenceData(AliveAudio::m_LoadedSeqData[targetSong]) == 0);
        player->PlaySequence();

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
    char buf[4096];
    ImGuiTextFilter mFilter;
    int listbox_item_current = 1;
    std::vector<const char*> listbox_items;
    std::unique_ptr<class IMovie>& mFmv;
public:
    FmvUi(std::unique_ptr<class IMovie>& fmv, IAudioController& audioController)
        : mFmv(fmv), mAudioController(audioController)
    {
#ifdef _WIN32
        strcpy(buf, "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Oddysee\\");
#else
        strcpy(buf, "/media/paul/FF7DISC3/Program Files (x86)/Steam/SteamApps/common/Oddworld Abes Oddysee/");
#endif
    }

    void DrawVideoSelectionUi(const std::string& setName, const std::vector<std::string>& allFmvs)
    {
        std::string name = "Video player (" + setName + ")";
        ImGui::Begin(name.c_str(), nullptr, ImVec2(550, 580), 1.0f, ImGuiWindowFlags_NoCollapse);


        ImGui::InputText("Video path", buf, sizeof(buf));

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
            std::string fullPath = std::string(buf) + listbox_items[listbox_item_current];
            std::cout << "Play " << listbox_items[listbox_item_current] << std::endl;
            try
            {
                //mFmv = std::make_unique<MasherMovie>(fullPath);
                //mFmv = std::make_unique<DDVMovie>(fullPath);
                mFmv = std::make_unique<MovMovie>(fullPath, mAudioController); 
            }
            catch (const Oddlib::Exception& ex)
            {

            }
        }

        ImGui::End();
    }
private:
    IAudioController& mAudioController;
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
                mFmvUis.emplace_back(std::make_unique<FmvUi>(mFmv, mAudioController));
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

Fmv::Fmv(GameData& gameData, IAudioController& audioController)
    : mGameData(gameData), mAudioController(audioController)
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
