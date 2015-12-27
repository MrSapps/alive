#define _STRING_H

#include <STDLIB.H>
#include <LIBGTE.H>
#include <LIBGPU.H>
#include <LIBGS.H>
#include <LIBDS.H>
#include <STDIO.H>
#include <LIBETC.H>
#include <STRINGS.H>
#include <KERNEL.H>
#include <LIBSND.H>
#include "DebugFont.c"
#include "tims/FONT.h"
#include "tims/P1.h"
#include "tims/P2.h"



#define OT_LENGTH 40 // the ordertable length
#define PACKETMAX 4000 // the maximum number of objects on the screen
#define SCREEN_WIDTH  368 // screen width
#define	SCREEN_HEIGHT 240 // screen height

#define countof(x) sizeof(x)/sizeof(x[0])
class Graphics
{
public:
    Graphics()
    {
        mActiveBuffer = 0;
    }


    void Init()
    {
        ::SetVideoMode(IsPalBios() ? MODE_PAL : MODE_NTSC);

        ::GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsINTER|GsOFSGPU, 1, 0); // set the graphics mode resolutions. You may also try using 'GsNONINTER' (read LIBOVR46.PDF in PSYQ/DOCS for detailed information)
        ::GsDefDispBuff(0, 0, 0, SCREEN_HEIGHT); // set the top left coordinates of the two buffers in video memory

        // init the ordering tables
        mOrderingTable[0].length = OT_LENGTH;
        mOrderingTable[1].length = OT_LENGTH;
        mOrderingTable[0].org = mOrderingTableTag[0];
        mOrderingTable[1].org = mOrderingTableTag[1];

        ::GsClearOt(0,0,&mOrderingTable[0]);
        ::GsClearOt(0,0,&mOrderingTable[1]);

        clearVRAM();

        ::FntLoad(960, 0); // load the font from the BIOS into VRAM/SGRAM


        ::SetDumpFnt(FntOpen(10, 10, SCREEN_WIDTH-20, SCREEN_HEIGHT-20, 0, 1024*4)); // screen X,Y | max text length X,Y | automatic background clear 0,1 | max characters

        // Overwrite the debug font with something less ugly
        ::LoadImage(&DbgFnt_ImageRect, reinterpret_cast<u_long*>(DbgFnt_ImageData));
        ::LoadImage(&DbgFnt_ClutRect,  reinterpret_cast<u_long*>(DbgFnt_ClutData));

    }

    void PreRender()
    {
        mActiveBuffer = GsGetActiveBuff(); // get the current buffer
        ::GsSetWorkBase((PACKET*)mGpuPacketBuffer[mActiveBuffer]); // setup the packet workbase
        ::GsClearOt(0, 0, &mOrderingTable[mActiveBuffer]); // clear the ordering table
    }

    void Render()
    {
        ::DrawSync(0); // wait for all drawing to finish
        ::VSync(0); // wait for v_blank interrupt
        ::GsSwapDispBuff(); // flip the double buffers
        ::GsSortClear(0, 0, 0, &mOrderingTable[mActiveBuffer]); // clear the ordering table with a background color.
        ::GsDrawOt(&mOrderingTable[mActiveBuffer]); // Draw the ordering table for the CurrentBuffer
        ::FntFlush(-1); // refresh the font
    }

    GsOT* OtPtr()
    {
        return &mOrderingTable[mActiveBuffer];
    }

private:
    bool IsPalBios()
    {
        // within the BIOS, if the address 0xBFC7FF52 equals 'E' (SCEE string), set it as PAL otherwise, set it as NTSC
        return *(const char *)0xbfc7ff52=='E';
    }

    void clearVRAM()
    {
        RECT rectTL;

        setRECT(&rectTL, 0, 0, 1024, 512);
        ClearImage2(&rectTL, 0, 0, 0);

        DrawSync(0); // ensure that the VRAM is clear before exiting
    }

private:
    GsOT mOrderingTable[2];
    GsOT_TAG mOrderingTableTag[2][PACKETMAX];
    PACKET mGpuPacketBuffer[2][PACKETMAX];
    int mActiveBuffer;
};

//u_long __ramsize   = 0x00200000; // force 2 megabytes of RAM
//u_long __stacksize = 0x00004000; // force 16 kilobytes of stack


#define kNumLvls 33
const char* kLvlFiles[kNumLvls] =
{
    "\\PM.LVL",
    "\\P2\\C1.LVL",
    "\\P2\\D1.LVL",
    "\\P2\\D2.LVL",
    "\\P2\\D7.LVL",
    "\\P2\\E2.LVL",
    "\\P2\\R2.LVL",
    "\\P2\\R6.LVL",
    "\\E1.LVL",
    "\\F1.LVL",
    "\\F2.LVL",
    "\\F4.LVL",
    "\\L1.LVL",
    "\\R1.LVL",
    "\\S1.LVL",
    "\\BA\\BA.LVL",
    "\\BM\\BM.LVL",
    "\\BR\\BR.LVL",
    "\\BW\\BW.LVL",
    "\\CR\\CR.LVL",
    "\\FD\\FD.LVL",
    "\\MI\\MI.LVL",
    "\\NE\\NE.LVL",
    "\\PV\\PV.LVL",
    "\\ST\\ST.LVL",
    "\\SV\\SV.LVL",

    "\\ABESODSE\\C1.LVL",
    "\\ABESODSE\\E1.LVL",
    "\\ABESODSE\\R1.LVL",
    "\\ABESODSE\\S1.LVL",

    "\\ABE2\\MI.LVL",
    "\\ABE2\\CR.LVL",
    "\\ABE2\\ST.LVL",
};

void my_memcpy(void* dst, void* src, int size)
{
    unsigned char* d = reinterpret_cast<unsigned char*>(dst);
    unsigned char* s = reinterpret_cast<unsigned char*>(src);
    for (int i=0; i<size; i++)
    {
        d[i] = s[i];
    }
}

class CdRom
{
public:
    void Init(bool debugOn = false)
    {
        ::DsInit();
        ::DsSetDebug(debugOn ? 1 : 0);
    }

    void DeInit()
    {
        ::DsClose();
    }

    bool Ready() const
    {
        return ::DsSystemStatus() == DslReady;
    }

    // Stop the cd spinning
    void StopCdRotation()
    {
        DslLOC pos;
        ::DsControl(DslStop, reinterpret_cast<u_char*>(&pos), mStatusBuffer);
    }

    // Check if file exists, if yes returns true and populates fileInfo, else returns false and fileInfo is undefined
    bool Find(const char* fileName, DslFILE& fileInfo)
    {
        if (!::DsSearchFile(&fileInfo, const_cast<char*>(fileName)))
        {
            printf("File %s not found\n", fileName);
            return false;
        }
        else
        {
            printf("File %s found, starts at sector %d size %ld\n", fileName, fileInfo.pos.sector, fileInfo.size);
            return true;
        }
    }

    bool ReadSectors(int sector, int numSectors, void* buffer)
    {
        DslLOC pos;
        ::DsIntToPos(sector, &pos);

        printf("Start read of [%d] sectors from sector [%d]\n", numSectors, sector);
        if (::DsRead(&pos, numSectors, reinterpret_cast<u_long*>(buffer), DslModeSpeed | DslModeSize1) == 0)
        {
            printf("Read failure\n");
            return false;
        }

        int ret = 0;
        do
        {
            ret = ::DsReadSync(mStatusBuffer);
        }
        while (ret > 0);
        printf("ReadSync [%d]\n", ret);
        return ret == 0;
    }

private:
    u_char mStatusBuffer[8];
};

typedef unsigned int Uint32;
typedef unsigned short int Uint16;
typedef signed short int Sint16;
typedef unsigned char Uint8;
typedef signed char Sint8;

const static Uint32 KMaxLvlArchiveFileNameLength = 12;
const static Uint32 kSectorSize = 2048;
const static Uint32 kMaxFiles = 500;


enum ePads
{
    ePad1 = 1,
    ePad2 = 2,
    eAny = 3
};

// Delects press, e.g is x pressed == yes when x is is down but wasn't previously down
class Controller
{
private:
    u_long GetButtonMask(ePads pad, u_long button)
    {
        switch(pad)
        {
        case ePad1:
            return _PAD(0, button);

        case ePad2:
            return _PAD(1, button);

        case eAny:
            return _PAD(0, button) | _PAD(1, button);
        }

        // Impossible - but have to shut compiler up
        return 0;
    }

    bool ButtonPressed(ePads pad, u_long button)
    {
         return (mPadState & GetButtonMask(pad, button)) && !(mPrevPadState & GetButtonMask(pad, button));
    }

public:
    Controller()
     :  mPrevPadState(0), mPadState(0)
    {

    }

    void Init()
    {
        // This API only supports the 16-button controller, which is fine
        ::PadInit(0);
    }

    bool Up(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADLup);
    }

    bool Down(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADLdown);
    }

    bool Left(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADLleft);
    }

    bool Right(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADLright);
    }

    bool Circle(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADRright);
    }

    bool Cross(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADRdown);
    }

    bool Triangle(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADRup);
    }

    bool Square(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADRleft);
    }

    bool Select(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADselect);
    }

    bool Start(ePads pad = eAny)
    {
        return ButtonPressed(pad, PADstart);
    }

    void Update()
    {
        mPrevPadState = mPadState;
        mPadState = ::PadRead(0);
    }
private:
    u_long mPrevPadState;
    u_long mPadState;
};

static Controller gInput;
static CdRom cdRom;

class CdStream
{
public:
    CdStream()
        : mPos(0), mStartSector(0), mSector(0)
    {

    }

    void Reset(int startSector)
    {
        mPos = 0;
        mStartSector = startSector;
        mSector = startSector;
    }

    void ReadUInt8(Uint8& output)
    {
        ReadBytes(reinterpret_cast<Uint8*>(&output), sizeof(Uint8));
    }

    void ReadUInt32(Uint32& output)
    {
        ReadBytes(reinterpret_cast<Uint8*>(&output), sizeof(Uint32));
    }

    void ReadUInt16(Uint16& output)
    {
        ReadBytes(reinterpret_cast<Uint8*>(&output), sizeof(Uint16));
    }

    void ReadSInt16(Sint16& output)
    {
        ReadBytes(reinterpret_cast<Uint8*>(&output), sizeof(Sint16));
    }

    void ReadBytes(Sint8* pDest, size_t destSize)
    {
        ReadBytes(reinterpret_cast<Uint8*>(pDest), destSize);
    }

    bool ReadBytes(Uint8* pDest, size_t destSize)
    {
        for (;;)
        {
            const int posWithinSector = mPos % (kSectorSize);
            if (posWithinSector == 0)
            {
                //printf("Read to next sector POS zero [%d]\n", mSector);
                if (!cdRom.ReadSectors(mSector, 1, mSectorBuffer))
                {
                    return false;
                }
                mSector++;
            }

            //assert(posWithinSector <= kSectorSize);

            // If what we will read is more than the remaining data in this sector
            //printf("SECTOR POS: [%d] END [%d]\n", posWithinSector, posWithinSector + destSize);
            if (posWithinSector + destSize > kSectorSize)
            {
                //printf("Partial fit\n");

                const int spaceLeft = 2048 - static_cast<int>(posWithinSector);
                if (spaceLeft < 0 )
                {
                    printf("Error: Space left [%d]\n", spaceLeft);
                    for (;;) { }
                }

                const int dataLeftAfterRead = static_cast<int>(destSize) - spaceLeft;
                if (dataLeftAfterRead < 0 )
                {
                    printf("Error: dataLeftAfterRead [%d]\n", dataLeftAfterRead);
                    for (;;) { }
                }

                my_memcpy(pDest, mSectorBuffer+posWithinSector, spaceLeft);
                mPos += spaceLeft;

                pDest += spaceLeft;
                destSize = dataLeftAfterRead;
                continue;
            }
            else if (destSize)
            {
                //printf("Fully fits\n");
                my_memcpy(pDest, mSectorBuffer+posWithinSector, destSize);
                mPos += destSize;
            }

            break;
        }
        return true;
    }

    // Seek from 0 to file size
    bool Seek(size_t pos)
    {
        printf("Seek to [%d]\n", pos);

        // Add on the start sector position
        mSector = (pos / kSectorSize) + mStartSector;
        mPos = (mStartSector * kSectorSize) + pos;
        if ((mPos % kSectorSize) !=0 )
        {
            printf("Seek must fetch sector [%d]\n", mSector);
            // Fetch the buffer now else mSectorBuffer will have stale data
            if (!cdRom.ReadSectors(mSector, 1, mSectorBuffer))
            {
                return false;
            }
        }
        return true;
    }

    int StartSector() const
    {
        return mStartSector;
    }
private:
    Uint32 mPos;
    int mStartSector;
    int mSector;
    char mSectorBuffer[2048];
};

static Uint8 gVbBuffer[2048*400];
static Uint8 gVhBuffer[2048*100];
static Uint8 gSeqBuffer[2048*100];

class Lvl
{
public:
    static Uint32 MakeType(Uint8 b4, Uint8 b3, Uint8 b2, Uint8 b1)
    {
        return (static_cast<Uint32> (b1) << 24) +
                (static_cast<Uint32> (b2) << 16) +
                (static_cast<Uint32> (b3) << 8) +
                static_cast<Uint32> (b4);
    }

    struct ChunkHeader
    {
        Uint32 iSize; // Size inc header
        Uint32 iRefCount; // Ref count, should be zero when loaded
        Uint32 iType;
        Uint32 iId; // Id
    };

    struct LvlHeader
    {
        Uint32 mFirstFileOffset;
        Uint32 mNull1;
        Uint32 mMagic; // (Indx)
        Uint32 mNull2;
        Uint32 mNumFiles;
        Uint32 mFirstFileOffsetSectorNum;
        Uint32 mUnknown2;
        Uint32 mUnknown3;
    };

    struct FileRecord
    {
        // This array will not be NULL terminated
        char mFileNameBytes[KMaxLvlArchiveFileNameLength];

        Uint32 mStartSector;
        Uint32 mNumSectors;
        Uint32 mFileSize;
    };

    LvlHeader mHeader;
    FileRecord mFiles[kMaxFiles];
public:
    int NumFiles() const
    {
        return mHeader.mNumFiles;
    }

    const FileRecord& FileAt(int index)
    {
        return mFiles[index];
    }

    Lvl()
    {
        printf("lvl ctor\n");
    }

    ~Lvl()
    {
        printf("Lvl dtor\n");
    }

    bool Load(const char* fileName)
    {
        char buffer[256] = {};
        sprintf(buffer, "%s;1", fileName);

        printf("Searching for [%s]...\n", fileName);
        if (cdRom.Find(buffer, mFileInfo))
        {
            // Load the first sector so we can grab the header
            const int sector = ::DsPosToInt(&mFileInfo.pos);
            mStream.Reset(sector);

            if (!mStream.ReadBytes(reinterpret_cast<Uint8*>(&mHeader), sizeof(LvlHeader)))
            {
                return false;
            }

            Print(mHeader);

            if (mHeader.mNumFiles > countof(mFiles))
            {
                printf("More files in LVL than hard coded array size\n");
                return false;
            }

            for (Uint32 i=0; i<mHeader.mNumFiles; i++)
            {
                //printf("Reading record [%d] of [%d]\n", i+1, mHeader.mNumFiles);
                if (!mStream.ReadBytes(reinterpret_cast<Uint8*>(&mFiles[i]), sizeof(FileRecord)))
                {
                    printf("Failed to read file record\n");
                    return false;
                }
                //Print(mFiles[i]);
            }

            return true;
        }
        else
        {
            printf("Lvl not found\n");
            return false;
        }
    }

    bool LoadFile(const char* name, unsigned char* buffer, unsigned int bufferLen, unsigned int& fileSize)
    {
        for (unsigned int i=0; i<mHeader.mNumFiles; i++)
        {
            if (strncmp(name, mFiles[i].mFileNameBytes, KMaxLvlArchiveFileNameLength) == 0)
            {
                printf("Loading file [%s]\n", name);
                fileSize = mFiles[i].mFileSize;
                return LoadFile(mFiles[i], buffer, bufferLen);
            }
        }
        printf("File [%s] not found\n", name);
        return false;
    }

    bool LoadFile(const FileRecord& rec, unsigned char* buffer, unsigned int bufferLen)
    {
        if (rec.mFileSize > bufferLen)
        {
            printf("File is too big for buffer [%d] vs [%d]\n", rec.mFileSize, bufferLen);
            return false;
        }

        if (!cdRom.ReadSectors(mStream.StartSector() + rec.mStartSector, rec.mNumSectors, buffer))
        {
            printf("Reading data failure\n");
            return false;
        }

        printf("File read completed\n");
        return true;
    }

    static void Print(const LvlHeader& h)
    {
        printf("mFirstFileOffset=%d\n", h.mFirstFileOffset);
        printf("mNull1=%d\n", h.mNull1);
        printf("mMagic=%d\n", h.mMagic);
        printf("mNull2=%d\n", h.mNull2);
        printf("mNumFiles=%d\n", h.mNumFiles);
        printf("mFirstFileOffsetSectorNum=%d\n", h.mFirstFileOffsetSectorNum);
        printf("mUnknown2=%d\n", h.mUnknown2);
        printf("mUnknown3=%d\n", h.mUnknown3);
    }

    static void Print(const FileRecord& f)
    {
        printf("mStartSector=%d\n", f.mStartSector);
        printf("mNumSectors=%d\n", f.mNumSectors);
        printf("mFileSize=%d\n", f.mFileSize);
        char tmpBuffer[KMaxLvlArchiveFileNameLength+1] = {};
        strncpy(tmpBuffer, f.mFileNameBytes, KMaxLvlArchiveFileNameLength);
        printf("mFileNameBytes=%s\n", tmpBuffer);
    }

private:
    CdStream mStream;
    DslFILE mFileInfo;
};

enum eMainStates
{
    eInitScreen,
    eLvlSelection,
    eCdNotReady,
    eLvlLoading,
    eErrorLoadingLvl,
    eLoadedLvl,
    eErrorLoadingTracks,
    eTrackSelection,
};

static Lvl gLvl;
static eMainStates gMainState = eInitScreen;

class TextMenu
{
public:
    TextMenu()
    {

    }

    void Init(const char** strings, int numStrings, const char* title = NULL)
    {
        mMenuTitle = title;

        mStrings = strings;
        mNumStrings = numStrings;

        mYSize = mNumStrings / 2;
        mLastRowOdd = mNumStrings % 2;
        if (mLastRowOdd)
        {
            mYSize++;
        }

        mSelectedX = 0;
        mSelectedY = 0;
    }

    void Update()
    {
        if (gInput.Up())
        {
            mSelectedY--;
            if (mSelectedY < 0)
            {
                if (mLastRowOdd && mSelectedX == kXSize - 1)
                {
                    mSelectedY = mYSize - 2;
                }
                else
                {
                    mSelectedY = mYSize - 1;
                }
            }
        }
        else if (gInput.Down())
        {
            mSelectedY++;
            if (mSelectedY > mYSize - 1 || (mLastRowOdd && mSelectedY > mYSize - 2 && mSelectedX == kXSize - 1))
            {
                mSelectedY = 0;
            }
        }
        else if (gInput.Left())
        {
            mSelectedX--;
            if (mSelectedX < 0)
            {
                if (mLastRowOdd && mSelectedY == mYSize - 1)
                {
                    mSelectedX = kXSize - 2;
                }
                else
                {
                    mSelectedX = kXSize - 1;
                }
            }
        }
        else if (gInput.Right())
        {
            mSelectedX++;
            if (mSelectedX > kXSize - 1 || (mLastRowOdd && mSelectedY == mYSize - 1))
            {
                mSelectedX = 0;
            }
        }
    }

    void Render()
    {
        if (mMenuTitle)
        {
            ::FntPrint("%s\n\n", mMenuTitle);
        }

        for (int i = 0; i <mNumStrings; i++)
        {
            if (i == mNumStrings - 1 && mLastRowOdd)
            {
                ::FntPrint("[%s] %s\n", SelectedString(i), mStrings[i]);
            }
            else
            {
                char buffer[40] = {};
                strcpy(buffer, mStrings[i]);

                int len = 0;
                do
                {
                    len = strlen(buffer);
                    if (len < 16)
                    {
                        buffer[len] = ' ';
                        len++;
                    }
                } while (len < 16);

                ::FntPrint("[%s] %s [%s] %s\n", SelectedString(i), buffer, SelectedString(i + 1), mStrings[i + 1]);
                i++;
            }
        }
    }

    int SelectedIndex() const
    {
        const int r = (mSelectedY * kXSize) + mSelectedX;
        return r;
    }

private:
    const char* SelectedString(int pos)
    {
        const int y = pos / kXSize;
        const int x = pos % kXSize;
        if (x == mSelectedX && y == mSelectedY)
        {
            return "X";
        }
        return " ";
    }

private:
    const char* mMenuTitle;
    const char** mStrings;
    int mNumStrings;
    const static int kXSize;
    int mYSize;
    int mSelectedY;
    int mSelectedX;
    bool mLastRowOdd;
};

const int TextMenu::kXSize = 2;

static TextMenu gLevelSelectMenu;

int DisplayLevelSelection()
{
    gLevelSelectMenu.Update();
    gLevelSelectMenu.Render();

    if (gInput.Cross())
    {
        return gLevelSelectMenu.SelectedIndex();
    }
    else if (gInput.Start())
    {
        cdRom.StopCdRotation();
    }
    return -1;
}

#define MVOL		127
#define SVOL		64

class Sound
{
public:
    Sound()
    {
        mOpenVabId = -1;
        mOpenSeqId = -1;
    }

    void Init()
    {
        ::SsInit();
        ::SsSetTableSize(mSeqTable, 4, 5);
        // Every 1/240th of a second is the highest res possible
        ::SsSetTickMode(SS_TICK60);
        ::SsStart();
        ::SsSetMVol(MVOL, MVOL);
    }

    bool LoadVab(unsigned char* vhBuffer, unsigned char* vbBuffer)
    {
        CloseVab();

        mOpenVabId = ::SsVabTransfer(vhBuffer, vbBuffer, -1, SS_WAIT_COMPLETED);
        if (mOpenVabId < 0)
        {
            if (mOpenVabId == -1)
            {
                printf("ERROR: Invalid VH\n");
                return false;
            }
            else if (mOpenVabId == -2)
            {
                printf("ERROR: Invalid VB\n");
                return false;
            }
            else
            {
                printf("ERROR: Copying to SPU memory\n");
                return false;
            }
        }
        if (::SsVabTransCompleted(SS_WAIT_COMPLETED) == 0)
        {
            printf("ERROR: Didn't finish copy to SPU ram\n");
            return false;
        }

        return true;
    }

    bool PlaySEQ(unsigned char* seqBuffer)
    {
        if (mOpenVabId==-1)
        {
            printf("ERROR: No vab loaded\n");
            return false;
        }

        CloseSeq();

        mOpenSeqId = ::SsSeqOpen(reinterpret_cast<unsigned long*>(seqBuffer), mOpenVabId);
        if (mOpenSeqId == -1)
        {
            printf("Open SEQ failed\n");
            return false;
        }

        printf("Playing seq [%d]\n", mOpenSeqId);
        ::SsSeqSetVol(mOpenSeqId, SVOL, SVOL);
        ::SsSeqPlay(mOpenSeqId, SSPLAY_PLAY, 1);

        return true;
    }

    void CloseSeq()
    {
        if (mOpenSeqId != -1)
        {
            printf("Closing seq [%d]\n", mOpenSeqId);
            ::SsSeqClose(mOpenSeqId);
            mOpenSeqId = -1;
        }
    }
private:
    void CloseVab()
    {
        if (mOpenVabId != -1)
        {
            printf("Closing vab [%d]\n", mOpenVabId);
            ::SsVabClose(mOpenVabId);
            mOpenVabId = -1;
        }
    }

    char mSeqTable[SS_SEQ_TABSIZ * 4 * 5];
    short mOpenVabId;
    short mOpenSeqId;
};

static Sound gSound;

class SeqChunk
{
public:
    SeqChunk()
    {
        for (int i = 0; i < 200; i++)
        {
            sprintf(mNumbers[i], "SEQ %d", i+1);
            mNumbersPtrs[i] = &mNumbers[i][0];
        }
    }

    bool Parse(unsigned char* seqChunk)
    {
        mData = seqChunk;
        bool ok = true;
        mNumSeqs = 0;
        unsigned char* ptr = mData;
        for (;;)
        {
            Lvl::ChunkHeader* chunk = reinterpret_cast<Lvl::ChunkHeader*>(ptr);
            if (chunk->iType == Lvl::MakeType('E', 'n', 'd', '!'))
            {
                printf("At end\n");
                break;
            }
            else if (chunk->iType == Lvl::MakeType('S', 'e', 'q', ' '))
            {
                printf("Found SEQ\n");
                mNumSeqs++;
            }
            else
            {
                printf("Unknown chunk type\n");
            }
            ptr += chunk->iSize;
        }

        mMenu.Init(mNumbersPtrs, mNumSeqs);

        return ok;
    }

    unsigned char* SeqPtr(unsigned int index)
    {
        if (index > mNumSeqs)
        {
            return NULL;
        }

        unsigned char* ptr = gSeqBuffer;
        unsigned int count = 0;
        for (;;)
        {
            Lvl::ChunkHeader* chunk = reinterpret_cast<Lvl::ChunkHeader*>(ptr);
            if (chunk->iType == Lvl::MakeType('E', 'n', 'd', '!'))
            {
                return NULL;
            }
            else if (chunk->iType == Lvl::MakeType('S', 'e', 'q', ' '))
            {
                if (count == index)
                {
                    printf("Found SEQ at index %d\n", index);
                    return reinterpret_cast<unsigned char*>(chunk);
                }
                count++;
            }
            ptr += chunk->iSize;
        }
        return NULL;
    }

    int DisplayMenu()
    {
        mMenu.Update();
        mMenu.Render();
        return mMenu.SelectedIndex();
    }
private:
    unsigned char* mData;
    unsigned int mNumSeqs;
    char mNumbers[200][10];
    const char* mNumbersPtrs[200];
    TextMenu mMenu;
};
static SeqChunk gSeqChunk;

#define MAX_VH_FILES 20

class VhMenu
{
public:
    VhMenu()
    {
        mNumVhs = 0;
        for (int i = 0; i < MAX_VH_FILES; i++)
        {
            mStrsPtrs[i] = &mStrs[i][0];
        }
    }

    void Init(Lvl& lvl)
    {
        mNumVhs = 0;
        for (int i = 0; i < lvl.NumFiles(); i++)
        {
            const Lvl::FileRecord& rec = lvl.FileAt(i);
            char tmp[13] = {};
            strncpy(tmp, rec.mFileNameBytes, 12);
            if (strstr(tmp, ".VH"))
            {
                printf("name: %s\n", tmp);
                memcpy(mStrs[mNumVhs], tmp, 13);
                mNumVhs++;
            }
        }

        mMenu.Init((const char**)mStrsPtrs, mNumVhs, "Press X to load soundbank\nPress O to go back\n\n");
    }

    int DisplayMenu()
    {
        mMenu.Update();
        mMenu.Render();
        return mMenu.SelectedIndex();
    }

    char* SelectedName()
    {
        if (mMenu.SelectedIndex() == -1)
        {
            return NULL;
        }
        return mStrsPtrs[mMenu.SelectedIndex()];
    }

private:
    unsigned int mNumVhs;
    char mStrs[MAX_VH_FILES][13];
    char* mStrsPtrs[MAX_VH_FILES];
    TextMenu mMenu;
};
static VhMenu gVhMenu;

static void HandleLvlLoaded()
{
    const int selected = gVhMenu.DisplayMenu();
    if (gInput.Cross() && selected != -1)
    {
        char* vhName = gVhMenu.SelectedName();

        unsigned int vhSize = 0;
        if (!gLvl.LoadFile(vhName, gVhBuffer, sizeof(gVhBuffer), vhSize))
        {
            gMainState = eErrorLoadingTracks;
            return;
        }

        unsigned int vbSize = 0;
        vhName[strlen(vhName)-1] = 'B';
        if (!gLvl.LoadFile(vhName, gVbBuffer, sizeof(gVbBuffer), vbSize))
        {
            vhName[strlen(vhName)-1] = 'H';
            gMainState = eErrorLoadingTracks;
            return;
        }
        vhName[strlen(vhName)-1] = 'H';

        if (!gSound.LoadVab(gVhBuffer, gVbBuffer))
        {
            printf("Failed to load VAB data\n");
            gMainState = eErrorLoadingTracks;
            return;
        }

        unsigned int seqsSize = 0;

        for (int i = 0; i < gLvl.NumFiles(); i++)
        {
            const Lvl::FileRecord& rec = gLvl.FileAt(i);
            char tmp[13] = {};
            strncpy(tmp, rec.mFileNameBytes, 12);
            if (strstr(tmp, ".BSQ"))
            {
                if (!gLvl.LoadFile(tmp, gSeqBuffer, sizeof(gSeqBuffer), seqsSize))
                {
                    gMainState = eErrorLoadingTracks;
                    return;
                }

                if (!gSeqChunk.Parse(gSeqBuffer))
                {
                    gMainState = eErrorLoadingTracks;
                    return;
                }

                gMainState = eTrackSelection;
                return;
            }
        }

        gMainState = eErrorLoadingTracks;
    }
    else if (gInput.Circle())
    {
        gMainState = eLvlSelection;
    }
}

static Graphics graphics;

#define MAX_STARS	150
#define star_w		8000			//determines the width of the area that stars 'appear' in

// Seems "brainwlkr" originally came up with this effect
class StarField
{
public:
    StarField()
    {
        speed = 2;
    }

    void Init()
    {
        //inits starfield
        srand(1);					//init the randomiser (for creating random star allocations)

        for (int i = 0; i < MAX_STARS; i++) {			//this bit sets up each 'star' (MAX_STARS number of them)
            myStars[i].x = star_w - (rand()*(2 * star_w)) / (32767 + 1);	//set stars x-coordinate
            myStars[i].y = star_w - (rand()*(2 * star_w)) / (32767 + 1);	//set stars y-coordinate
            myStars[i].z = (rand()*(80)) / (32767 + 1) + 1;			//set stars z-coordinate 1-81 (cant be zero since +1)
            myStars[i].line.r = myStars[i].line.g = myStars[i].line.b = 255;
        }//for
    }

    void Render()
    {
        //this is what makes the stars 'move', it also adds the stars to the OT
        GsOT* otPtr = graphics.OtPtr();

        for (int i = 0; i < MAX_STARS; i++) 
        {

            if (myStars[i].z < 2)
            {
                myStars[i].z = 100;	//if the star has reached you then put it back to come toward you again
            }

            //divide by c so that as star gets closer it seems to move faster past you
            const int x = (myStars[i].x) / (myStars[i].z);		//sets x value of stars
            const int y = (myStars[i].y) / (myStars[i].z);		//sets y value of stars
            myStars[i].z -= speed;				//moves stars toward you
            myStars[i].line.x1 = myStars[i].line.x0 = x + SCREEN_WIDTH / 2;			//centres starfields x value onscreen
            myStars[i].line.y1 = myStars[i].line.y0 = y + SCREEN_HEIGHT / 2;		//centres starfields y value onscreen

            GsSortLine(&myStars[i].line, otPtr, 0);	//sorts stars into OT)
        }
    }
private:
    typedef struct{
        int x;					//star structure - has and x, y and z coordinate
        int y;
        int z;
        GsLINE line;				//GsLINE strcuture that make a star pixel
    }Star;


    Star myStars[MAX_STARS];
    int speed;
};

static StarField gStarField;

class TimImage
{
public:
    explicit TimImage(const unsigned char* timBuffer)
        : mTimBuffer(timBuffer)
    {

    }

    void UploadToVram()
    {
        // +4 to skip the DWORD header magic flag
        ::GsGetTimInfo((u_long*)(mTimBuffer+4), &mGsImage);
        LoadImage(mGsImage.px, mGsImage.py, mGsImage.pw, mGsImage.ph, mGsImage.pixel);
        // Only 4 and 8 bit TIMs have CLUTs

        LoadImage(mGsImage.cx, mGsImage.cy, mGsImage.cw, mGsImage.ch, mGsImage.clut);
    }

    int ClutX() const  { return mGsImage.cx; }
    int ClutY() const  { return mGsImage.cy; }
    int Width() const  { return mGsImage.pw; }
    int Height() const { return mGsImage.ph; }
    int PMode() const { return mGsImage.pmode; }
    int VramX() const { return mGsImage.px; }
    int VramY() const { return mGsImage.py; }
private:
    void LoadImage(u_short x, u_short y, u_short w, u_short h, u_long* data)
    {
        RECT rect = { x, y, w, h };
        printf("Upload RECT: [%d,%d,%d,%d]\n", w, y, w, h);
        ::LoadImage(&rect, data);
    }

    const unsigned char* mTimBuffer;
    GsIMAGE mGsImage;
};

static void InitSprite(GsSPRITE& sprite, const TimImage& tim)
{
    memset(&sprite, 0, sizeof(GsSPRITE));
    
    // 16 bit CLUT, all options off (0x1 = 8-bit, 0x2 = 16-bit)
    sprite.attribute = 0x1000000;

    // 100% scale
    sprite.scalex = ONE;
    sprite.scaley = ONE;

    // Colour
    sprite.r = 128;
    sprite.g = 128;
    sprite.b = 128;

    // CLUT
    sprite.cx = tim.ClutX();
    sprite.cy = tim.ClutY();

    // TPage
    sprite.tpage = GetTPage(1, 2, tim.VramX(), tim.VramY());

    // Size
    sprite.w = 256;// tim.Width() * 2;
    sprite.h = tim.Height();
}

int main()
{
    ::ResetCallback();

    printf("main enter\n");


    graphics.Init();
    gStarField.Init();

    cdRom.Init(true);

    gInput.Init();

    gSound.Init();

    gLevelSelectMenu.Init(kLvlFiles, kNumLvls, "Select a level to load\nPress start to stop/swap CD");

    TimImage copyrightLeft(P1);
    copyrightLeft.UploadToVram();

    TimImage copyrightRight(P2);
    copyrightRight.UploadToVram();

    GsSPRITE copyRight[2];
    InitSprite(copyRight[0], copyrightLeft);
    InitSprite(copyRight[1], copyrightRight);
    copyRight[0].y = -3;
    copyRight[0].x = 0+25;
    copyRight[1].y = -3;
    copyRight[1].x = 128+25;

    //TestFindAllFiles(cdRom);
    int lvlIndex = -1;
    int timeToShowloading = 0;
    int timeToShowInit = 0;
    for (;;)
    {
        gInput.Update();
        graphics.PreRender();

        if (gMainState != eInitScreen)
        {
            gStarField.Render();
        }

        switch (gMainState)
        {
        case eInitScreen:
            GsSortSprite(&copyRight[0], graphics.OtPtr(), 0);
            GsSortSprite(&copyRight[1], graphics.OtPtr(), 0);
            timeToShowInit++;
            if (timeToShowInit > 300)
            {
                gMainState = eLvlSelection;
            }
            break;

        case eLvlSelection:
        {
            lvlIndex = DisplayLevelSelection();
            if (lvlIndex != -1)
            {
                // TODO: Check CD properly, if CD is switched we never
                // seem to get a new dir listing
                if (!cdRom.Ready())
                {
                    gMainState = eCdNotReady;
                }
                else
                {
                    timeToShowloading = 0;
                    gMainState = eLvlLoading;
                }
            }
        }
        break;

        case eCdNotReady:
            ::FntPrint("CD ROM not ready\n\nPress X go to back");
            if (gInput.Cross())
            {
                gMainState = eLvlSelection;
            }
            break;

        case eLvlLoading:
        {
            timeToShowloading++;
            ::FntPrint("Loading [%s]\n", kLvlFiles[lvlIndex]);
            if (timeToShowloading >= 30)
            {
                printf("Loading %s\n", kLvlFiles[lvlIndex]);
                if (gLvl.Load(kLvlFiles[lvlIndex]))
                {
                    gMainState = eLoadedLvl;
                    gVhMenu.Init(gLvl);
                }
                else
                {
                    gMainState = eErrorLoadingLvl;
                }
                lvlIndex = -1;
            }
        }
        break;

        case eErrorLoadingLvl:
            ::FntPrint("Error loading LVL\n\nPress X go to back");
            if (gInput.Cross())
            {
                gMainState = eLvlSelection;
            }
            break;

        case eLoadedLvl:
            HandleLvlLoaded();
            break;

        case eErrorLoadingTracks:
            ::FntPrint("Error loading tracks\n\nPress X go to back");
            if (gInput.Cross())
            {
                gMainState = eLvlSelection;
            }
            break;

        case eTrackSelection:
            ::FntPrint("Track selection");
            ::FntPrint("\nPress X to play and O to go back\n\n");

            const int seqNum = gSeqChunk.DisplayMenu();
            if (gInput.Cross() && seqNum != -1)
            {
                unsigned char* seqPtr = gSeqChunk.SeqPtr(seqNum);
                if (seqPtr)
                {
                    if (!gSound.PlaySEQ(seqPtr + sizeof(Lvl::ChunkHeader)))
                    {
                        printf("SEQ play failed for idx [%d]\n", seqNum);
                    }
                }
            }
            else if (gInput.Circle())
            {
                gSound.CloseSeq();
                gMainState = eLvlSelection;
            }
            break;
        }

        graphics.Render();
    }

    return 0;
}
