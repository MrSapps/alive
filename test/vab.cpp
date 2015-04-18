#include "vab.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <stdlib.h>
#include <string.h>

Vab::Vab()
{
    iSoundsDat.open("sounds.dat", std::ios::binary);
    if (!iSoundsDat.is_open())
    {
        abort();
    }
}

Vab::Vab( std::string aVhFile, std::string aVbFile )
{
    LoadVhFile( aVhFile );
    LoadVbFile( aVbFile );
}

void Vab::LoadVbFile( std::string aFileName )
{
    // Read the file.
    std::ifstream file;
    file.open( aFileName.c_str(), std::ios::binary );
    if (!file.is_open())
    {
        abort();
    }
    ReadVb( file );
}

void Vab::ReadVb( std::istream& aStream )
{

    //    aStream.setByteOrder( QDataStream::LittleEndian );

    /*
    for ( quint32 i=0; i<iHeader->iNumVags; ++i )
    {
        AoVag* vag = new AoVag();
        aStream >> vag->iSize;
        aStream >> vag->iSampleRate;
        for ( quint32 k=0; k<vag->iSize; ++k )
        {
            quint8 byte = 0;
            aStream >> byte;
            vag->iSampleData.append( byte );
        }
        iAoVags.push_back( vag );
    }

    qDebug("Eof = %d", aStream.atEnd() );

    for ( quint32 i=0; i<iAoVags.size(); ++i )
    {
        quint32 rate = iAoVags[i]->iSampleRate;
        QByteArray data = iAoVags[i]->iSampleData;

        WaveFile file;

        // Sometimes rate is negative, no idea why!
        qint32 signedRate = static_cast< qint32 > ( rate );
        quint32 positiveRate = abs( signedRate );

        file.SetSampleData( data, positiveRate );

        QString fileName = "test_sound_" + QString::number( i ) + ".wav";
        file.Save( fileName );
    }
    */


    for ( unsigned int i=0; i<iHeader->iNumVags; i++ )
    {
        AEVh* vh = new AEVh( aStream );
        iOffs.push_back( vh );
    }

}


void Vab::LoadVhFile( std::string aFileName )
{
    // Read the file.
    std::ifstream file;
    file.open( aFileName.c_str(), std::ios::binary );
    if (!file.is_open())
    {
        abort();
    }
    ReadVh( file );
}

void Vab::ReadVh( std::istream& aStream )
{
    iHeader = new VabHeader( aStream );
    int tones = 0;
    for ( unsigned int i=0; i<128; i++ ) // 128 = max progs
    {
        ProgAtr* progAttr = new ProgAtr( aStream );
        iProgs.push_back( progAttr );
        tones += progAttr->iNumTones;
    }

    for ( unsigned int i=0; i<iHeader->iNumProgs; i++ )
    {
        for ( unsigned int j=0; j<16; j++ ) // 16 = max tones
        {
            VagAtr* vagAttr = new VagAtr( aStream );
            /* Remove these to get num tones to match header, but messes up index of tones*/
            /*
            if ( vagAttr->iVag == 0 )
            {
                delete vagAttr;
                continue;
            }
            */

            iTones.push_back( vagAttr );

            iProgs[vagAttr->iProg]->iTones.push_back( vagAttr );
        }
    }

    /* Remove these to get num progs to match header*/
    /*
    for ( size_t i=0; i<iProgs.size(); i++ )
    {
        if ( iProgs[i]->iTones.empty() )
        {
            iProgs.erase( iProgs.begin() + i , iProgs.begin() + i + 1 );
            i = 0;
        }
    }
    */


    // No 512 bytes at EOF!?
    // aStream.device()->seek( aStream.device()->pos() - 1 );

    if ( aStream.bad() )
    {
        //qDebug("Past EOF!!");
    }

    /*
    int abe = -1;
    int slig = -1;

    for ( unsigned int i=0; i<iTones.size(); i++ )
    {

        unsigned int index = iTones[i]->iProg;
        if ( iTones[i]->iVag == 15 )
        {
            if ( abe != -1 && abe != index )
            {
 //               qDebug("Abe error!");
            }
            abe = index;
        }

        if ( iTones[i]->iVag == 40 )
        {
            if ( slig != -1 && slig != index )
            {
//                qDebug("Slig error!");
            }
            slig = index;
        }

        if ( index > 0 && index < iProgs.size())
        {
            iProgs[index]->iTones.push_back( iTones[i] );
        }
    }

   // iProgs[abe] = iProgs[slig];

    //iProgs[abe]->iAttr = 90;
*/

}
