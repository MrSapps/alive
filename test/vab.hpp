#pragma once

#include <vector>
#include <ostream>
#include <istream>
#include <fstream>

struct VabHeader
{
    VabHeader( std::istream& aStream )
    {
        Read( aStream );
    }

    void Read( std::istream& aStream )
    {
        aStream.read( reinterpret_cast< char* > ( &iForm ), sizeof( iForm ) );
        aStream.read( reinterpret_cast< char* > ( &iVersion ), sizeof( iVersion ) );
        aStream.read( reinterpret_cast< char* > ( &iId ), sizeof( iId ) );
        aStream.read( reinterpret_cast< char* > ( &iFileSize ), sizeof( iFileSize ) );
        aStream.read( reinterpret_cast< char* > ( &iReserved0 ), sizeof( iReserved0 ) );
        aStream.read( reinterpret_cast< char* > ( &iNumProgs ), sizeof( iNumProgs ) );
        aStream.read( reinterpret_cast< char* > ( &iNumTones ), sizeof( iNumTones ) );
        aStream.read( reinterpret_cast< char* > ( &iNumVags ), sizeof( iNumVags ) );
        aStream.read( reinterpret_cast< char* > ( &iMasterVol ), sizeof( iMasterVol ) );
        aStream.read( reinterpret_cast< char* > ( &iMasterPan ), sizeof( iMasterPan ) );
        aStream.read( reinterpret_cast< char* > ( &iAttr1 ), sizeof( iAttr1 ) );
        aStream.read( reinterpret_cast< char* > ( &iAttr2 ), sizeof( iAttr2 ) );
        aStream.read( reinterpret_cast< char* > ( &iReserved1 ), sizeof( iReserved1 ) );
    }

    unsigned int iForm;				// Always "VABp"
    unsigned int iVersion;			// Header version
    unsigned int iId;				// Bank Id
    unsigned int iFileSize;			// File size
    unsigned short int iReserved0;  // Not used
    unsigned short int iNumProgs;
    unsigned short int iNumTones;
    unsigned short int iNumVags;
    unsigned char iMasterVol;
    unsigned char iMasterPan;
    unsigned char iAttr1;
    unsigned char iAttr2;
    unsigned int iReserved1;
};

//  program (instrument) level for example  "piano", "drum", "guitar" and "effect sound".
struct VagAtr;
struct ProgAtr
{
    ProgAtr( std::istream& aStream )
    {
        Read( aStream );
    }

    void Read( std::istream& aStream )
    {
        aStream.read( reinterpret_cast< char* > ( &iNumTones ), sizeof( iNumTones ) );
        aStream.read( reinterpret_cast< char* > ( &iVol ), sizeof( iVol ) );
        aStream.read( reinterpret_cast< char* > ( &iPriority ), sizeof( iPriority ) );
        aStream.read( reinterpret_cast< char* > ( &iMode ), sizeof( iMode ) );
        aStream.read( reinterpret_cast< char* > ( &iPan ), sizeof( iPan ) );
        aStream.read( reinterpret_cast< char* > ( &iReserved0 ), sizeof( iReserved0 ) );
        aStream.read( reinterpret_cast< char* > ( &iAttr ), sizeof( iAttr ) );
        aStream.read( reinterpret_cast< char* > ( &iReserved1 ), sizeof( iReserved1 ) );
        aStream.read( reinterpret_cast< char* > ( &iReserved2 ), sizeof( iReserved2 ) );
    }

    unsigned char iNumTones; // Number of valid entries in iTones
    unsigned char iVol;
    unsigned char iPriority;
    unsigned char iMode;
    unsigned char iPan;
    unsigned char iReserved0;
    unsigned short int iAttr;
    unsigned int iReserved1;
    unsigned int iReserved2;

    std::vector< VagAtr* > iTones;
};

// vag/tone/waves that are linked to an instrument
struct VagAtr
{
    VagAtr( std::istream& aStream )
    {
        Read( aStream );
    }

    void Read( std::istream& aStream )
    {
        aStream.read( reinterpret_cast< char* > ( &iPriority ), sizeof( iPriority ) );
        aStream.read( reinterpret_cast< char* > ( &iMode ), sizeof( iMode ) );
        aStream.read( reinterpret_cast< char* > ( &iVol ), sizeof( iVol ) );
        aStream.read( reinterpret_cast< char* > ( &iPan ), sizeof( iPan ) );
        aStream.read( reinterpret_cast< char* > ( &iCenter ), sizeof( iCenter ) );
        aStream.read( reinterpret_cast< char* > ( &iShift ), sizeof( iShift ) );
        aStream.read( reinterpret_cast< char* > ( &iMin ), sizeof( iMin ) );
        aStream.read( reinterpret_cast< char* > ( &iMax ), sizeof( iMax ) );
        aStream.read( reinterpret_cast< char* > ( &iVibW ), sizeof( iVibW ) );
        aStream.read( reinterpret_cast< char* > ( &iVibT ), sizeof( iVibT ) );
        aStream.read( reinterpret_cast< char* > ( &iPorW ), sizeof( iPorW ) );
        aStream.read( reinterpret_cast< char* > ( &iPorT ), sizeof( iPorT ) );
        aStream.read( reinterpret_cast< char* > ( &iPitchBendMin ), sizeof( iPitchBendMin ) );
        aStream.read( reinterpret_cast< char* > ( &iPitchBendMax ), sizeof( iPitchBendMax ) );
        aStream.read( reinterpret_cast< char* > ( &iReserved1 ), sizeof( iReserved1 ) );
        aStream.read( reinterpret_cast< char* > ( &iReserved2 ), sizeof( iReserved2 ) );
        aStream.read( reinterpret_cast< char* > ( &iAdsr1 ), sizeof( iAdsr1 ) );
        aStream.read( reinterpret_cast< char* > ( &iAdsr2 ), sizeof( iAdsr2 ) );
        aStream.read( reinterpret_cast< char* > ( &iProg ), sizeof( iProg ) );
        aStream.read( reinterpret_cast< char* > ( &iVag ), sizeof( iVag ) );
        aStream.read( reinterpret_cast< char* > ( &iReserved[0] ), sizeof( iReserved ) );
    }

    unsigned char iPriority;

    // 0 = normal, 4 = reverb
    unsigned char iMode;

    // volume 0-127
    unsigned char iVol;

    // panning 0-127
    unsigned char iPan;

    // "Default" note
    unsigned char iCenter;
    unsigned char iShift;
    
    // Key range which this waveform maps to
    unsigned char iMin;
    unsigned char iMax;
    
    // Maybe these are not used?
    unsigned char iVibW;
    unsigned char iVibT;
    unsigned char iPorW;
    unsigned char iPorT;
    
    // Min/max pitch values
    unsigned char iPitchBendMin;
    unsigned char iPitchBendMax;
    
    // Not used
    unsigned char iReserved1;
    unsigned char iReserved2;
    
    // adsr1
    // 0-127 attack rate (byte)
    // 0-15 decay rate (nibble)
    // 0-127 sustain rate (byte)

    // adsr2
    // 0-31 release rate (6bits)
    // 0-15 sustain rate (nibble)
    // 1+0.5+1+0.6+0.5=3.6 bytes
    unsigned short int iAdsr1;
    unsigned short int iAdsr2;
    
    short int iProg; // Which progAttr we live in?
    short int iVag;  // Sound index in the VB
    short int iReserved[4];
};

// AE VAB header points into sounds.dat!

// A record in the VB!! file not the VH!!
struct AEVh
{
    AEVh( std::istream& aStream )
    {
        Read( aStream );
    }

    void Read( std::istream& aStream )
    {
        aStream.read( reinterpret_cast< char* > ( &iLengthOrDuration ), sizeof( iLengthOrDuration ) );
        aStream.read( reinterpret_cast< char* > ( &iUnusedByEngine ), sizeof( iUnusedByEngine ) );
        aStream.read( reinterpret_cast< char* > ( &iFileOffset ), sizeof( iFileOffset ) );
    }

    unsigned int iLengthOrDuration;
    unsigned int iUnusedByEngine; // khz, 44100, or 8000 seen? aka bit rate then?
    unsigned int iFileOffset;
};

struct AoVag
{
    unsigned int iSize;
    unsigned int iSampleRate;
    std::vector< unsigned char > iSampleData;
};

class Vab
{
public:
    Vab();
    Vab( std::string aVhFile, std::string aVbFile );
    void LoadVbFile( std::string aFileName );
    void ReadVb( std::istream& aStream );
    void LoadVhFile( std::string aFileName );
    void ReadVh( std::istream& aStream );

public:
    VabHeader* iHeader;  // File header
    std::vector< ProgAtr* > iProgs; // Always 128 of these - must be what the midi data is indexing into?
    std::vector< VagAtr* > iTones;	// always 16 * num progs? Also called a tone??
    // VAG Data body / (VB 254 VAG data) ("Samples" in AE?)
    // 512bytes /2 = 256 samps max
    std::vector< AEVh* > iOffs;

    std::vector< AoVag* > iAoVags;

    std::ifstream iSoundsDat;

    // What about AO? Seems the same as sounds.dat but in each VB file
};
