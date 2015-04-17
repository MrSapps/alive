#include <gmock/gmock.h>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/exceptions.hpp"
#include <fluidsynth.h>
#include "SDL.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(LvlArchive, DISABLED_FileNotFound)
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

struct SeqInfo
{
    Uint32 iLastTime = 0;
    Sint32 iNumTimesToLoop = 0;
    Uint8 running_status = 0;
};


void snd_midi_skip_length(Oddlib::Stream& stream, int skip)
{
    stream.Seek(stream.Pos() + skip);
}

static Uint32 midi_read_var_len(Oddlib::Stream& stream)
{
    Uint32 ret = 0;
    Uint8 byte = 0;
    for (int i = 0; i < 4; ++i)
    {
        stream.ReadUInt8(byte);
        ret = (ret << 7) | (byte & 0x7f);
        if (!(byte & 0x80))
        {
            break;
        }
    }
    return ret;
}

void sendnoteon(int chan, short key, short vel, unsigned int date);
static unsigned int deltaTime = 0;

static int SND_seq_play_q(fluid_synth_t* synth, Oddlib::Stream& stream)
{
    // Read the header
    SeqHeader header;
    stream.ReadUInt32(header.mMagic);
    stream.ReadUInt32(header.mVersion);
    stream.ReadUInt16(header.mResolutionOfQuaterNote);
    stream.ReadBytes(header.mTempo, sizeof(header.mTempo));
    stream.ReadUInt16(header.mRhythm);

    const size_t midiDataStart = stream.Pos();

    // Context state
    SeqInfo gSeqInfo = {};

    for (;;)
    {
        // Read event delta time
        Uint32 delta = midi_read_var_len(stream);
        deltaTime += delta;
        std::cout << "Delta: " << delta << " over all " << deltaTime << std::endl;

        // Obtain the event/status byte
        Uint8 eventByte = 0;
        stream.ReadUInt8(eventByte);
        if (eventByte < 0x80)
        {
            // Use running status
            if (!gSeqInfo.running_status) // v1
            {
                return 0; // error if no running status?
            }
            eventByte = gSeqInfo.running_status;

            // Go back one byte as the status byte isn't a status
            stream.Seek(stream.Pos() -1);
        }
        else
        {
            // Update running status
            gSeqInfo.running_status = eventByte;
        }

        if (eventByte == 0xff)
        {
            // Meta event
            Uint8 metaCommand = 0;
            stream.ReadUInt8(metaCommand);

            Uint8 metaCommandLength = 0;
            stream.ReadUInt8(metaCommandLength);

            switch (metaCommand)
            {
            case 0x2f:
            {
                std::cout << "end of track" << std::endl;

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
                return 0;

            case 0x51:    // Tempo in microseconds per quarter note (24-bit value)
            {
                std::cout << "Temp change" << std::endl;
                // TODO: Not sure if this is correct
                Uint8 tempoByte = 0;
                for (int i = 0; i < 3; i++)
                {
                    stream.ReadUInt8(tempoByte);
                }
            }
                break;

            default:
            {
                std::cout << "Unknown meta event " << Uint32(metaCommand) << std::endl;
                // Skip unknown events
                // TODO Might be wrong
                snd_midi_skip_length(stream, metaCommandLength);
            }

            }
        }
        else if (eventByte < 0x80)
        {
            // Error
            throw std::runtime_error("Unknown midi event");
        }
        else
        {
            const Uint8 channel = eventByte & 0xf;
            switch (eventByte >> 4)
            {
            case 0x9:
            {
                Uint8 note = 0;
                stream.ReadUInt8(note);

                Uint8 velocity = 0;
                stream.ReadUInt8(velocity);

                std::cout << Uint32(channel) << " NOTE ON " << Uint32(note) << " vel " << Uint32(velocity) << std::endl;

                if (velocity ==0)
                {

                    //fluid_synth_noteoff(synth, channel, note);
                }
                else
                {
                    sendnoteon(channel, note, velocity, deltaTime);
                    //fluid_synth_noteon(synth, channel, note, velocity);
                }
            }
            break;
            case 0x8:
            {
                Uint8 note = 0;
                stream.ReadUInt8(note);

                Uint8 velocity = 0;
                stream.ReadUInt8(velocity);

                std::cout << Uint32(channel) << " NOTE OFF " << Uint32(note) << " vel " << Uint32(velocity) << std::endl;

                //fluid_synth_noteoff(synth, channel, note);
            }
            break;
            case 0xc:
            {
                Uint8 prog = 0;
                stream.ReadUInt8(prog);
                std::cout << Uint32(channel) << " program change " << Uint32(prog) << std::endl;
                fluid_synth_program_change(synth, channel, prog);
            }
            break;
            case 0xa:
            {
                Uint8 note = 0;
                Uint8 pressure = 0;

                stream.ReadUInt8(note);
                stream.ReadUInt8(pressure);
                std::cout << Uint32(channel) << " polyphonic key pressure (after touch)" << Uint32(note) << " " << Uint32(pressure) << std::endl;
            }
                break;
            case 0xb:
            {
                Uint8 controller = 0;
                Uint8 value = 0;
                stream.ReadUInt8(controller);
                stream.ReadUInt8(value);
                std::cout << Uint32(channel) << " controller change " << Uint32(controller) << " value " << Uint32(value);

                /* addditonal special logic that only applies to oddworld seq music
                OutputDebugString("Controller change\n");
                int controller_number = ((unsigned int)status_byte >> 8) & 0x7F;
                if (controller_number != 6 && controller_number != 0x26)
                {
                    if (controller_number == 0x63)
                    {
                        // *static_byte_BD1CFC = (unsigned int)status_byte >> 16;
                    }
                    break;
                }

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
                *static_byte_BD1CFC = 0;
                */
            }
                break;
            case 0xd:
            {
                Uint8 value = 0;
                stream.ReadUInt8(value);
                std::cout << Uint32(channel) << " after touch " << Uint32(value) << std::endl;
            }
                break;
            case 0xe:
            {
                Uint16 bend = 0;
                stream.ReadUInt16(bend);
                std::cout << Uint32(channel) << " pitch bend " << Uint32(bend) << std::endl;
            }
                break;
            case 0xf:
            {
                const Uint32 length = midi_read_var_len(stream);
                snd_midi_skip_length(stream, length);
                std::cout << Uint32(channel) << " sysex len: " << length << std::endl;
            }
                break;
            default:
                throw std::runtime_error("Unknown MIDI command");
            }
        }
       // SDL_Delay(100);
    }
}

TEST(Seq, midi_read_var_len_1_byte)
{
    std::vector<Uint8> data = { 0x7F };
    Oddlib::Stream stream(std::move(data));
    ASSERT_EQ(127u, midi_read_var_len(stream));
}

TEST(Seq, midi_read_var_len_2_byte)
{
    std::vector<Uint8> data = { 0x81, 0x00 };
    Oddlib::Stream stream(std::move(data));
    ASSERT_EQ(128u, midi_read_var_len(stream));
}

TEST(Seq, midi_read_var_len_3_byte)
{
    std::vector<Uint8> data = { 0x81, 0x7f, 0x00 };
    Oddlib::Stream stream(std::move(data));
    ASSERT_EQ(255u, midi_read_var_len(stream));
}

TEST(Seq, midi_read_var_len_4_byte)
{
    std::vector<Uint8> data = { 0xFF, 0xFF, 0xFF, 0x7F };
    Oddlib::Stream stream(std::move(data));
    ASSERT_EQ(268435455u, midi_read_var_len(stream));
}

short synthSeqID = 0;
short mySeqID = 0;
fluid_sequencer_t* sequencer = nullptr;
unsigned int now = 0;
unsigned int seqduration = 0;

void sendnoteon(int chan, short key, short vel, unsigned int date)
{

   // key = 60;
   // vel = 127;

    int fluid_res;
    fluid_event_t *evt = new_fluid_event();
    fluid_event_set_source(evt, -1);
    fluid_event_set_dest(evt, synthSeqID);
    fluid_event_noteon(evt, chan, key, vel);
    fluid_res = fluid_sequencer_send_at(sequencer, evt, date/2, 1);
    delete_fluid_event(evt);
}

void schedule_next_callback()
{
    int fluid_res;
    // I want to be called back before the end of the next sequence
    unsigned int callbackdate = now + seqduration/2;
    fluid_event_t *evt = new_fluid_event();
    fluid_event_set_source(evt, -1);
    fluid_event_set_dest(evt, mySeqID);
    fluid_event_timer(evt, NULL);
    fluid_res = fluid_sequencer_send_at(sequencer, evt, callbackdate, 1);
    delete_fluid_event(evt);
}

void schedule_next_sequence() {
    // Called more or less before each sequence start
    // the next sequence start date
    now = now + seqduration;

    // the sequence to play

    // the beat : 2 beats per sequence
    sendnoteon(0, 60, 127, now + seqduration/2);
    sendnoteon(0, 60, 127, now + seqduration);

    // melody
    sendnoteon(1, 45, 127, now + seqduration/10);
    sendnoteon(1, 50, 127, now + 4*seqduration/10);
    sendnoteon(1, 55, 127, now + 8*seqduration/10);

    // so that we are called back early enough to schedule the next sequence
    schedule_next_callback();
}

/* sequencer callback */
void seq_callback(unsigned int time, fluid_event_t* event, fluid_sequencer_t* seq, void* data)
{
    schedule_next_sequence();
}

TEST(Seq, ParseSeq)
{
    std::vector<Uint8> seqData = { 
        0x70, 0x51, 0x45, 0x53, 0x00, 0x00, 0x00, 0x01, 0x01, 0xE0, 0x04, 0x11, 
        0xAA, 0x04, 0x02, 0x00, 0xC6, 0x11, 0x00, 0xC7, 0x1A, 0x02, 0x96, 0x3C,
        0x70, 0x04, 0x97, 0x3D, 0x79, 0x06, 0x3C, 0x73, 0x68, 0x3D, 0x00, 0x06, 
        0x3C, 0x00, 0x81, 0x34, 0x96, 0x3C, 0x00, 0x76, 0x97, 0x36, 0x79, 0x0A, 
        0x35, 0x76, 0x0E, 0x96, 0x30, 0x73, 0x56, 0x97, 0x36, 0x00, 0x0A, 0x35, 
        0x00, 0x81, 0x4C, 0x96, 0x30, 0x00, 0x2E, 0x97, 0x48, 0x67, 0x00, 0x30,
        0x67, 0x00, 0x24, 0x67, 0x42, 0x96, 0x24, 0x76, 0x36, 0x97, 0x48, 0x00, 
        0x00, 0x30, 0x00, 0x00, 0x24, 0x00, 0x81, 0x76, 0x96, 0x24, 0x00, 0x84,
        0x0A, 0xC6, 0x11, 0x00, 0xC7, 0x1A, 0x04, 0x96, 0x30, 0x54, 0x04, 0x97,
        0x31, 0x5A, 0x06, 0x30, 0x56, 0x68, 0x31, 0x00, 0x06, 0x30, 0x00, 0x84,
        0x58, 0x96, 0x30, 0x00, 0x44, 0x97, 0x18, 0x33, 0x78, 0x18, 0x00, 0x0E,
        0x96, 0x24, 0x56, 0x24, 0x97, 0x2A, 0x5A, 0x02, 0x29, 0x58, 0x6C, 0x2A, 
        0x00, 0x02, 0x29, 0x00, 0x0C, 0x24, 0x67, 0x78, 0x24, 0x00, 0x83, 0x38, 
        0x96, 0x24, 0x00, 0x81, 0x2C, 0x18, 0x58, 0x68, 0x97, 0x24, 0x4D, 0x02,
        0x2B, 0x4D, 0x7C, 0x24, 0x00, 0x02, 0x2B, 0x00, 0x83, 0x68, 0x96, 0x18,
        0x00, 0x00, 0xFF, 0x2F, 0x00, 0x00, 0x00, 0x00 };

    Oddlib::Stream seqStream(std::move(seqData));


    fluid_settings_t* settings = new_fluid_settings();

    // audio.driver alsa, oss, pulseaudio
    fluid_settings_setstr(settings, "audio.driver", "alsa");
    //fluid_settings_setnum(settings, "synth.polyphony", 64);
    //fluid_settings_setnum(settings, "audio.period-size", 1649);

    fluid_synth_t* synth = new_fluid_synth(settings);

   // sequencer = new_fluid_sequencer2(0);
    sequencer = new_fluid_sequencer();

    synthSeqID = fluid_sequencer_register_fluidsynth(sequencer, synth);
    mySeqID = fluid_sequencer_register_client(sequencer, "me", seq_callback, NULL);

    // the sequence duration, in ms
    seqduration = 100;

    if (fluid_is_soundfont("/home/paul/Desktop/Abe2MidiPlayer/Oddworld.sf2"))
    {
        fluid_synth_sfload(synth, "/home/paul/Desktop/Abe2MidiPlayer/Oddworld.sf2", 1);
    }
    else
    {
        abort();
    }


    fluid_player_t* player = new_fluid_player(synth);

    fluid_audio_driver_t* adriver = new_fluid_audio_driver(settings, synth);

   // fluid_synth_program_change(synth, 0, 17); // 17 or 26
   // fluid_synth_program_change(synth, 1, 26); // 17 or 26


    //schedule_next_sequence();

    SND_seq_play_q(synth, seqStream);

    //fluid_player_play(player);
    //fluid_player_join(player);

    SDL_Delay(3000);

    delete_fluid_sequencer(sequencer);

    delete_fluid_audio_driver(adriver);
    delete_fluid_player(player);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);

}


TEST(LvlArchive, DISABLED_Integration)
{
    // Load AE lvl
    Oddlib::LvlArchive lvl("MI.LVL");

    const auto file = lvl.FileByName("FLYSLIG.BND");
    ASSERT_NE(nullptr, file);


    const auto chunk = file->ChunkById(450);
    ASSERT_NE(nullptr, chunk);

    ASSERT_EQ(450u, chunk->Id());

    const auto data = chunk->ReadData();
    ASSERT_FALSE(data.empty());

    Oddlib::LvlArchive lvl2("R1.LVL");

    std::vector<std::unique_ptr<Oddlib::Animation>> animations = Oddlib::AnimationFactory::Create(lvl2, "ROPES.BAN", 1000);

}
