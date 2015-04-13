#include <gmock/gmock.h>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/exceptions.hpp"

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(LvlArchive, FileNotFound)
{
    ASSERT_THROW(Oddlib::LvlArchive("not_found.lvl"), Oddlib::Exception);
}

struct SeqHeader
{
    Uint32 mMagic; // SEQp
    Uint32 mVersion; // Seems to always be 1
    Uint16 mResolutionOfQuaterNote;
    Uint8 mTempo[3];
    Uint16 mRhythm;

};

/*
70 51 45 53 magic
00 00 00 01 version
01 E0 
0E 15 C4  tempo
04 02 rhythm
00 - maybe padding?

C Program Change [F] to 16

26 55 9F 
3C 7F 2D 
3C 00 00 

FF 2F 00 end of track
00 00 00 probably an error - or padding

*/

struct SeqInfo
{
    Uint32 iLastTime = 0;
    Sint32 iNumTimesToLoop = 0;
    unsigned char running_status_q = 0;
};

static Uint32 GetTheTime() { return 0;  }

int getVarLen_q(Oddlib::Stream& stream)
{
    // TODO
    abort();

    return 0;
}

void snd_midi_skip_length(Oddlib::Stream& stream, int skip)
{
    stream.Seek(stream.Pos() + skip);
}


static int SND_seq_play_q(Oddlib::Stream& stream)
{
    // Read the header
    SeqHeader header;
    stream.ReadUInt32(header.mMagic);
    stream.ReadUInt32(header.mVersion);
    stream.ReadUInt16(header.mResolutionOfQuaterNote);
    stream.ReadBytes(header.mTempo, sizeof(header.mTempo));
    stream.ReadUInt16(header.mRhythm);

    unsigned char padding = 0; // might be wrong
    stream.ReadUInt8(padding);

    const size_t midiDataStart = stream.Pos();


    // Context state
    SeqInfo gSeqInfo = {};

    // compare saved vs current time?
    if (gSeqInfo.iLastTime > GetTheTime()) 
    {
        return 1;
    }

    
    //while (1)
    {
        unsigned char prevSndPtr = 0;
        stream.ReadUInt8(prevSndPtr);

        unsigned char midi_byte = prevSndPtr;

        if (prevSndPtr >= 0xF0u) // Is this a Sysex or meta event?
        {
            if (prevSndPtr == 0xF0 || prevSndPtr == 0xF7)
            {
                // System Exclusive (Sysex) Events (skip them)
                int sysExEventLength = getVarLen_q(stream);
                snd_midi_skip_length(stream, sysExEventLength);
            }
            else if (prevSndPtr == 0xFF)// Is it a meta event?
            {
                unsigned char meta_event_type = 0;
                stream.ReadUInt8(meta_event_type);

                if (meta_event_type == 0x2F)        // End of track?
                {
                    int loopCount = gSeqInfo.iNumTimesToLoop;// v1 some hard coded data?? or just a local static?
                    if (loopCount) // If zero then loop forever
                    {
                        --loopCount;

                        //char buf[256];
                        //sprintf(buf, "EOT: %d loops left\n", loopCount);
                       // OutputDebugString(buf);

                        gSeqInfo.iNumTimesToLoop = loopCount; //v1
                        if (loopCount <= 0)
                        {
                            //getNext_q(aSeqIndex); // Done playing? Ptr not reset to start
                            return 1;
                        }
                    }

                    //OutputDebugString("EOT: Loop forever\n");
                    // Must be a loop back to the start?
                    stream.Seek(midiDataStart);
                }
                else
                {
                    // Some other meta event
                    /*
                    int meta_event_length = getVarLen_q(gSeqInfo->iSeqPtr);
                    if (meta_event_length)
                    {
                        if (meta_event_type == 0x51)    // Tempo in microseconds per quarter note (24-bit value)
                        {
                            incPtr(gSeqInfo->iSeqPtr);
                            incPtr(gSeqInfo->iSeqPtr);
                            unsigned short int tempo = (unsigned char)incPtr(gSeqInfo->iSeqPtr);
                            snd_midi_set_tempo(aSeqIndex, 0, tempo & 0x7FFF);
                        }
                        else
                        {
                            // unknown meta event? skip it
                            snd_midi_skip_length(gSeqInfo->iSeqPtr, meta_event_length);
                        }
                    }*/
                }
            }
            //goto end_of_func;
        }

     
        char param_1 = 0;
        if (prevSndPtr >= 0x80u) // note on or off/valid running  status
        {
            stream.ReadUInt8((Uint8 &)param_1);
            gSeqInfo.running_status_q = midi_byte; // v1 save running status?

        }
        else
        {
            if (!gSeqInfo.running_status_q) // v1
            {
                return 0; // error if no running status?
            }

            param_1 = midi_byte; // 1st param is normally the note number?



            midi_byte = gSeqInfo.running_status_q; //v1
        }



        int shifted_param_1 = 0;
        shifted_param_1 |= (Uint16)(param_1 << 8); // param 1, such as the new prog num

        int param_1_and_2 = midi_byte | shifted_param_1; // param + first byte (normally the command/channel byte)
        int status_byte = midi_byte & 0xF0; // is it note on, prog change etc.
        unsigned int key_velocity = param_1_and_2;

        int param_1_and_3_and_maybe_3 = param_1_and_2; // command/running status & param1

        if (status_byte != 0xC0)                  // not Program/Instrument Change
        {
            if (status_byte != 0xD0)                // not Channel Key Pressure
            {
                // must be key on/off??
                // TODO FIX ME
                //param_1_and_3_and_maybe_3 = ((unsigned char)incPtr(gSeqInfo->iSeqPtr) << 16) | param_1_and_2;
                key_velocity = param_1_and_3_and_maybe_3; // command/running status & param1 + key pressure
            }
        }

        status_byte = status_byte & 0xF0;

        
        switch (status_byte)
        {
            // Note on, key & vol are params
        case 0x90:
        {
            /*
            int channel = key_velocity & 0xF; // Nibble 1 is the channel
            seq_data* v16 = get_data(channel);

            unsigned char program_number_copy = v16->program_number;


            unsigned int base_volume = v16->base_volume;

            // base volume?
            int adjusted_volume = (signed __int16)(base_volume * (DWORD)gSeqInfo->volume_adjust_q >> 7); //v1

            if (key_velocity >> 16) // do we have a key pressure?
            {
                //OutputDebugString("Note on [on]\n");
                signed int current_ref_count = 0;
                psx_sound_channel* sPtr = unk_C1409C;

                // Get the ref count for this channel(?), if any
                signed int num_keys = 24;
                do
                {
                    if (sPtr->field_3)
                    {
                        if (sPtr->field_0 == gSeqInfo->w28) // v1
                        {
                            if (sPtr->program_number == v16->program_number)
                            {
                                if (sPtr->tone_id == channel + (16 * aSeqIndex))
                                {
                                    v16->program_number = program_number_copy;
                                    unsigned char curKey = ((key_velocity & 0xFF00) >> 8);
                                    if (sPtr->key_number == curKey) // param 1 & 2 key number & key pressure
                                    {
                                        if (sPtr->m_ref_count_q > current_ref_count)
                                        {
                                            current_ref_count = sPtr->m_ref_count_q;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    ++sPtr;
                } while (num_keys-- != 1);

                // Update the ref count
                int updated_ref_count = current_ref_count + 1;


                int note_to_play = key_velocity & 0xFF00;
                int volR = key_velocity >> 16;

                unsigned char note8Bits = (unsigned char)((int)(note_to_play >> 8));

                // note8Bits++;
                // note8Bits--;
                // note8Bits--;


                note_to_play = note_to_play & 0xFF;



                note_to_play = note_to_play | (((unsigned short int)note8Bits) << 8);



                // Play the tone - returns which channels it is playing on?
                unsigned int play_ret = 0;  (unsigned int)calls_play_snd_seq(
                                           gSeqInfo->w28,			// must be zero? OK v1
                                           v16->program_number,	// program number (can be set by seq byte)
                                           note_to_play,	// sample to play  (from seq byte)
                                           adjusted_volume,		// vol left?
                                           v16->centre_vol_q,		// centre vol? // 3rd byte = max data??
                                           volR);	// vol right? aka key pressure byte? (from seq byte)

                std::string h = "NOTE:" + std::to_string(note_to_play) + "\n";
                OutputDebugString(h.c_str());


                // unk_C140 9C = 156
                // unk_C140 AA = 170 9c is 14 bytes before here, or rather +14 bytes on 9c
                unsigned int key_item_counter = 0;
                unsigned char* itemPtr = (unsigned char *)unk_C140AA;
                unsigned int cntr = 24;
                do
                {
                    if ((1 << key_item_counter) & play_ret)
                    {
                        // ptr - 1 means take offset 14 - 2, thus offset is 12
                        // member at off 12 is volume_adjust_q ??
                        *((WORD *)itemPtr - 1) = channel + (16 * aSeqIndex);
                        *itemPtr = updated_ref_count;
                    }
                    ++key_item_counter;
                    itemPtr += 44;                      // move to next item?
                    --cntr;
                } while (cntr);

            }
            else
            {
                //OutputDebugString("Note on [off]\n");
                midi_note_off(channel, v16, param_1_and_3_and_maybe_3, key_velocity, aSeqIndex);
            }*/
        }
            break;
            // Note off, key & vol are params
        case 0x80:
        {
            /*
            //OutputDebugString("Note off\n");
            int note_off_channel = key_velocity & 0xF;

            seq_data* v28 = get_data(note_off_channel);

            midi_note_off(note_off_channel, v28, param_1_and_3_and_maybe_3, key_velocity, aSeqIndex);*/
        }
            break;
            // Controller Change
        case 0xB0:
        {
            /*
            OutputDebugString("Controller change\n");
            int controller_number = ((unsigned int)status_byte >> 8) & 0x7F;
            if (controller_number != 6 && controller_number != 0x26)
            {
                if (controller_number == 0x63)
                {
                    //*static_byte_BD1CFC = (unsigned int)status_byte >> 16;
                }
                break;
            }
            */

            /*
            switch (*static_byte_BD1CFC)
            {
            case 20:                              // set loop point
                OutputDebugString("Set loop\n");
                gSeqInfo->iLoopPtr = gSeqInfo->iSeqPtr; // v1
                gSeqInfo->num_loops_to_play = (unsigned int)status_byte >> 16; // v1
                break;
            case 30:								// go to loop point
            {
                OutputDebugString("Do loop\n");
                if (gSeqInfo->iLoopPtr)
                {
                    if (gSeqInfo->num_loops_to_play > 0) // v1
                    {
                        gSeqInfo->iSeqPtr = gSeqInfo->iLoopPtr; // Load saved loop point into the seq data
                        if (gSeqInfo->num_loops_to_play < 0x7f) // 127
                        {
                            *static_byte_BD1CFC = 0;
                            --gSeqInfo->num_loops_to_play; // v1
                            goto end_of_func;
                        }
                    }
                }
            }
                break;
            case 40:
                OutputDebugString("FuncPtr\n");
                if (gSeqInfo->iFuncPtr)
                {
                    BYTE arg3 = (status_byte << 8) & 0xFF;
                    gSeqInfo->iFuncPtr(aSeqIndex, 0, arg3);
                    *static_byte_BD1CFC = 0;
                    goto end_of_func;
                }
                break;
            }
            *static_byte_BD1CFC = 0;*/
        }
            break;
            // Program change aka channel number change
        case 0xC0:
        {
            /*
            BYTE param = (key_velocity >> 8) & 0xFF;
            const BYTE channel = key_velocity & 0xF;
            seq_data* data = get_data(channel);
            data->program_number = param;

            std::stringstream s;
            s << "Prog change to : " << (int)param << "\n";
            OutputDebugString(s.str().c_str());*/
        }
            break;
            // Pitch Bend - crashes?
        case 0xE0:
        {
            /*
            OutputDebugString("Pitch bend\n");
            int bend_by = ((key_velocity >> 8) - 16384) >> 4;
            snd_midi_pitch_bend(
                get_data(key_velocity & 0xF)->program_number, // v1
                bend_by);*/
        }
            break;
        default:
            break;
        }
    
   // end_of_func:

        /*
        int time_stamp = getVarLen_q(gSeqInfo->iSeqPtr);
        if (time_stamp)
        {
            // Convert next event ticks to milliseconds?
            // (60 * clocks) / (tempo * division) = seconds

            // d14, iLastTime = v1
            unsigned int next_event_time = (time_stamp * gSeqInfo->d14 / 1000) + gSeqInfo->iLastTime;
            gSeqInfo->iLastTime = next_event_time; // v1
            if (next_event_time > *g_time_dword_BD1CEC)
            {
                //goto start;
                return 1;
            }
        }*/

    }
    

    return 0;
}

TEST(Seq, DISABLED_Integration)
{
    Oddlib::LvlArchive lvl("MI.LVL");

    const auto seq = lvl.FileByName("MISEQ.BSQ");
    auto seqChunk = seq->ChunkById(0x0000009e);
    Oddlib::Stream seqStream(seqChunk->ReadData());
    SND_seq_play_q(seqStream);

    //seq->SaveChunks();
}


TEST(LvlArchive, DISABLED_Integration)
{
    // Load AE lvl
    Oddlib::LvlArchive lvl("MI.LVL");

    const auto file = lvl.FileByName("FLYSLIG.BND");
    ASSERT_NE(nullptr, file);


    const auto chunk = file->ChunkById(450);
    ASSERT_NE(nullptr, chunk);

    ASSERT_EQ(450, chunk->Id());

    const auto data = chunk->ReadData();
    ASSERT_EQ(false, data.empty());

    Oddlib::LvlArchive lvl2("R1.LVL");

    std::vector<std::unique_ptr<Oddlib::Animation>> animations = Oddlib::AnimationFactory::Create(lvl2, "ROPES.BAN", 1000);

}
