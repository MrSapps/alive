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

class IMovie
{
public:
    virtual ~IMovie() = default;
    virtual void OnRenderAudio(Uint8 *stream, Sint32 len) = 0;
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

private:
    AutoMouseCursorHide mHideMouseCursor;
};


// PSX MOV/STR format, all PSX game versions use this.
class MovMovie : public IMovie
{
private:
    std::unique_ptr<Oddlib::Stream> gStream;
    std::unique_ptr<RawCdImage> gCd;
    std::vector<Uint32> pixels;
protected:
    std::unique_ptr<Oddlib::IStream> mFmvStream;

    bool mPsx = false;
    MovMovie() = default;
public:
    MovMovie(const std::string& fullPath)
    {
        gStream = std::make_unique<Oddlib::Stream>("C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (E) [SLES-00664].bin");
        gCd = std::make_unique<RawCdImage>(*gStream);
        gCd->LogTree();
        bool exists = gCd->FileExists("S1.MOV");
        if (exists)
        {
            std::cout << "found" << std::endl;
        }
        mFmvStream = gCd->ReadFile("P2\\R2.MOV");
        AudioBuffer::ChangeAudioSpec(8064 / 4, 37800);
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

    struct MasherVideoHeaderWrapper
    {
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

    virtual void OnRenderAudio(Uint8 *stream, Sint32 len) override
    {

    }

    virtual void OnRenderFrame() override
    {
        std::vector<unsigned char> r(32768);
        std::vector<unsigned char> outPtr(32678);

        unsigned int numSectorsToRead = 0;
        unsigned int sectorNumber = 0;
        const int kDataSize = 2016;

        int frameW = 0;
        int frameH = 0;

        for (;;)
        {
            MasherVideoHeaderWrapper w;

            if (mPsx)
            {
                uint8_t sync[12];
                mFmvStream->ReadBytes(reinterpret_cast<Uint8*>(&sync), sizeof(sync));

                uint8_t header[4];
                mFmvStream->ReadBytes(reinterpret_cast<Uint8*>(&header), sizeof(header));
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
                    numBytes = mAdpcm.DecodeFrameToPCM((int8_t *)outPtr.data(), &rawXa->data[0], true);
                }
                else
                {
                    numBytes = mAdpcm.DecodeFrameToPCM((int8_t *)outPtr.data(), (uint8_t *)&w.mAkikMagic, true);
                }

                AudioBuffer::mPlayedSamples = 0;
                AudioBuffer::SendSamples((char*)outPtr.data(), numBytes);

                // 8064
                while (AudioBuffer::mPlayedSamples < numBytes / 4)
                {

                }

                // Must be VALE
                continue;
            }
            else
            {
                frameW = w.mWidth;
                frameH = w.mHeight;

                //std::cout << "sector: " << w.mSectorNumber << std::endl;
                //std::cout << "data len: " << w.mFrameDataLen << std::endl;
                //std::cout << "frame number: " << w.mFrameNum << std::endl;
                //std::cout << "num sectors in frame: " << w.mNumSectorsInFrame << std::endl;
                //std::cout << "frame sector number: " << w.mSectorNumberInFrame << std::endl;
                // SetSurfaceSize(w.mWidth, w.mHeight);

                uint32_t bytes_to_copy = w.mFrameDataLen - w.mSectorNumberInFrame *kDataSize;
                if (bytes_to_copy > 0)
                {
                    if (bytes_to_copy > kDataSize)
                    {
                        bytes_to_copy = kDataSize;
                    }

                    memcpy(r.data() + w.mSectorNumberInFrame * kDataSize, w.frame, bytes_to_copy);
                }

                if (w.mSectorNumberInFrame == w.mNumSectorsInFrame - 1)
                {
                    break;
                }
            }
        }

        
        if (pixels.empty())
        {
            pixels.resize(frameW * frameH);
        }

        mMdec.DecodeFrameToABGR32((uint16_t*)pixels.data(), (uint16_t*)r.data(), frameW, frameH, false);
        RenderFrame(frameW, frameH, pixels.data());
    }

    virtual bool IsEnd() override
    {
        return false;
    }

private:
    PSXMDECDecoder mMdec;
    PSXADPCMDecoder mAdpcm;
};

// Same as MOV/STR format but with modified magic in the video frames
// and XA audio raw cd sector information stripped. Only used in AO PC.
class DDVMovie : public MovMovie
{
public:
    DDVMovie(const std::string& fullPath)
    {
        mPsx = false;
        mFmvStream = std::make_unique<Oddlib::Stream>(fullPath);
        AudioBuffer::ChangeAudioSpec(8064 / 4, 37800);
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
            AudioBuffer::ChangeAudioSpec(mMasher->SingleAudioFrameSizeBytes(), mMasher->AudioSampleRate());
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

    virtual void OnRenderAudio(Uint8 *stream, Sint32 len) override
    {

    }

    virtual void OnRenderFrame() override
    {
        std::vector<Uint16> decodedFrame(mMasher->SingleAudioFrameSizeBytes() * 2); // *2 if stereo
        mEnd = !mMasher->Update(mFramePixels.data(), (Uint8*)decodedFrame.data());
        if (!mEnd)
        {
            RenderFrame(mMasher->Width(), mMasher->Height(), mFramePixels.data());
            AudioBuffer::SendSamples((char*)decodedFrame.data(), decodedFrame.size() * 2);
            while (AudioBuffer::mPlayedSamples < mMasher->FrameNumber() * mMasher->SingleAudioFrameSizeBytes())
            {

            }
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
    FmvUi(std::unique_ptr<class IMovie>& fmv)
        : mFmv(fmv)
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
                mFmv = std::make_unique<MovMovie>(fullPath); 
            }
            catch (const Oddlib::Exception& ex)
            {

            }
        }

        ImGui::End();
    }
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
                mFmvUis.emplace_back(std::make_unique<FmvUi>(mFmv));
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

Fmv::Fmv(GameData& gameData)
    : mGameData(gameData)
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
