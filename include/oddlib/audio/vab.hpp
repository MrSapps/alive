#pragma once

#include <vector>
#include <ostream>
#include <istream>
#include <fstream>
#include <memory>
#include <array>
#include "oddlib/stream.hpp"

struct VabHeader
{
    VabHeader() = default;

    VabHeader(Oddlib::IStream& aStream)
    {
        Read(aStream);
    }

    void Read(Oddlib::IStream& s)
    {
        s.ReadUInt32(iForm);
        s.ReadUInt32(iVersion);
        s.ReadUInt32(iId);
        s.ReadUInt32(iFileSize);
        s.ReadUInt16(iReserved0);
        s.ReadUInt16(iNumProgs);
        s.ReadUInt16(iNumTones);
        s.ReadUInt16(iNumVags);
        s.ReadUInt8(iMasterVol);
        s.ReadUInt8(iMasterPan);
        s.ReadUInt8(iAttr1);
        s.ReadUInt8(iAttr2);
        s.ReadUInt32(iReserved1);
    }

    Uint32 iForm;       // Always "VABp"
    Uint32 iVersion;    // Header version
    Uint32 iId;         // Bank Id
    Uint32 iFileSize;   // File size
    Uint16 iReserved0;  // Not used
    Uint16 iNumProgs;
    Uint16 iNumTones;
    Uint16 iNumVags;
    Uint8 iMasterVol;
    Uint8 iMasterPan;
    Uint8 iAttr1;
    Uint8 iAttr2;
    Uint32 iReserved1;
};

//  program (instrument) level for example  "piano", "drum", "guitar" and "effect sound".
struct VagAtr;
struct ProgAtr
{
    ProgAtr() = default;

    void Read(Oddlib::IStream& s)
    {
        s.ReadUInt8(iNumTones);
        s.ReadUInt8(iVol);
        s.ReadUInt8(iPriority);
        s.ReadUInt8(iMode);
        s.ReadUInt8(iPan);
        s.ReadUInt8(iReserved0);
        s.ReadUInt16(iAttr);
        s.ReadUInt32(iReserved1);
        s.ReadUInt32(iReserved2);
    }

    Uint8 iNumTones; // Number of valid entries in iTones
    Uint8 iVol;
    Uint8 iPriority;
    Uint8 iMode;
    Uint8 iPan;
    Uint8 iReserved0;
    Uint16 iAttr;
    Uint32 iReserved1;
    Uint32 iReserved2;

    // Pointers are not owned
    std::vector< VagAtr* > iTones;
};

struct VagAtr
{
    VagAtr(Oddlib::IStream& aStream )
    {
        Read( aStream );
    }

    void Read(Oddlib::IStream& s)
    {
        s.ReadUInt8(iPriority);
        s.ReadUInt8(iMode);
        s.ReadUInt8(iVol);
        s.ReadUInt8(iPan);
        s.ReadUInt8(iCenter);
        s.ReadUInt8(iShift);
        s.ReadUInt8(iMin);
        s.ReadUInt8(iMax);
        s.ReadUInt8(iVibW);
        s.ReadUInt8(iVibT);
        s.ReadUInt8(iPorW);
        s.ReadUInt8(iPorT);
        s.ReadUInt8(iPitchBendMin);
        s.ReadUInt8(iPitchBendMax);
        s.ReadUInt8(iReserved1);
        s.ReadUInt8(iReserved2);
        s.ReadUInt16(iAdsr1);
        s.ReadUInt16(iAdsr2);
        s.ReadSInt16(iProg);
        s.ReadSInt16(iVag);
        s.ReadSInt16(iReserved[0]);
        s.ReadSInt16(iReserved[1]);
        s.ReadSInt16(iReserved[2]);
        s.ReadSInt16(iReserved[3]);
    }

    Uint8 iPriority;

    // 0 = normal, 4 = reverb
    Uint8 iMode;

    // volume 0-127
    Uint8 iVol;

    // panning 0-127
    Uint8 iPan;

    // "Default" note
    Uint8 iCenter;
    Uint8 iShift;
    
    // Key range which this waveform maps to
    Uint8 iMin;
    Uint8 iMax;
    
    // Maybe these are not used?
    Uint8 iVibW;
    Uint8 iVibT;
    Uint8 iPorW;
    Uint8 iPorT;
    
    // Min/max pitch values
    Uint8 iPitchBendMin;
    Uint8 iPitchBendMax;
    
    // Not used
    Uint8 iReserved1;
    Uint8 iReserved2;
    
    // adsr1
    // 0-127 attack rate (byte)
    // 0-15 decay rate (nibble)
    // 0-127 sustain rate (byte)

    // adsr2
    // 0-31 release rate (6bits)
    // 0-15 sustain rate (nibble)
    // 1+0.5+1+0.6+0.5=3.6 bytes
    Uint16 iAdsr1;
    Uint16 iAdsr2;
    
    Sint16 iProg; // Which progAttr we live in?
    Sint16 iVag;  // Sound index in the VB
    Sint16 iReserved[4];
};

// AE VB file entry - rather than having the sound data in the VB it actually just has
// pointers into sounds.dat instead
struct AEVh
{
    AEVh( Oddlib::IStream& s )
    {
        Read( s );
    }

    void Read(Oddlib::IStream& s )
    {
        s.ReadUInt32(iLengthOrDuration);
        s.ReadUInt32(iUnusedByEngine);
        s.ReadUInt32(iFileOffset);
    }

    unsigned int iLengthOrDuration;
    unsigned int iUnusedByEngine; // khz, 44100, or 8000 seen? aka bit rate then?
    unsigned int iFileOffset;
};

class Vab
{
public:
    Vab() = default;
    void ReadVb(Oddlib::IStream& aStream, bool isPsx, bool useSoundsDat, Oddlib::IStream* soundsDatStream = nullptr);
    void ReadVh(Oddlib::IStream& stream, bool isPsx);

public:
    VabHeader mHeader;

    // We have 128 programs in a VAB
    std::array<ProgAtr, 128> mProgs;

    // Which can have 16 tones or instruments which are mapped to key ranges
    // and a sample along with how to play that sample
    std::vector< std::unique_ptr<VagAtr> > mTones;

    struct SampleData
    {
        std::vector<Uint8> mData;
    };
    std::vector<SampleData> mSamples;

private:
    // VAG Data body / (VB 254 VAG data) ("Samples" in AE?)
    // 512bytes /2 = 256 samps max
    std::vector<AEVh> iOffs;
    std::vector<Uint32> mVagOffsets;
};
