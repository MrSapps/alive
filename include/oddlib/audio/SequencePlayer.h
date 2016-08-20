#pragma once

#include "SDL.h"
#include <string>
#include "stdthread.h"
#include <vector>
#include "oddlib/stream.hpp"
#include "oddlib/audio/AliveAudio.h"
#include "stdthread.h"
#include "types.hpp"

struct SeqHeader
{
    u32 mMagic; // SEQp
    u32 mVersion; // Seems to always be 1
    u16 mResolutionOfQuaterNote;
    u8 mTempo[3];
    u8 mTimeSignatureBars;
    u8 mTimeSignatureBeats;
};

struct SeqInfo
{
    u32 iLastTime = 0;
    s32 iNumTimesToLoop = 0;
    u8 running_status = 0;
};

// The types of Midi Messages the sequencer will play.
enum AliveAudioMidiMessageType
{
    ALIVE_MIDI_NOTE_ON = 1,
    ALIVE_MIDI_NOTE_OFF = 2,
    ALIVE_MIDI_PROGRAM_CHANGE = 3,
    ALIVE_MIDI_ENDTRACK = 4,
};

// The current state of a SequencePlayer.
enum AliveAudioSequencerState
{
    ALIVE_SEQUENCER_STOPPED = 1,
    ALIVE_SEQUENCER_PLAYING = 3,
    ALIVE_SEQUENCER_FINISHED = 4,
    ALIVE_SEQUENCER_INIT_VOICES = 5,
};

struct AliveAudioMidiMessage
{
    AliveAudioMidiMessage(AliveAudioMidiMessageType type, int timeOffset, int channel, int note, char velocity, int special = 0)
    {
        Type = type;
        Channel = channel;
        Note = note;
        Velocity = velocity;
        TimeOffset = timeOffset;
        Special = special;
    }
    AliveAudioMidiMessageType Type;
    int Channel = 0;
    int Note = 0;
    char Velocity = 0;
    int TimeOffset = 0;
    int Special = 0;
};

class SequencePlayer
{
public:
    SequencePlayer(AliveAudio& aliveAudio);
    ~SequencePlayer();


    int LoadSequenceStream(Oddlib::IStream& stream);
    void PlaySequence();
    void StopSequence();

    f64 MidiTimeToSample(int time);
    u64 GetPlaybackPositionSample();

    // TODO: Shouldn't really be required anymore
    int m_TrackID = 1; // The track ID. Use this to separate SoundFX from Music.
    AliveAudioSequencerState m_PlayerState = ALIVE_SEQUENCER_STOPPED;

    // Gets called every time the play position is at 1/4 of the song.
    // Useful for changing sequences but keeping the time signature in sync.
    std::function<void()> m_QuarterCallback;

    //private:
    int m_KillThread = false; // If true, loop thread will exit.
    Uint64 m_SongFinishSample = 0; // Not relative.
    int m_SongBeginSample = 0;	// Not relative.
    int m_PrevBar = 0;
    int m_TimeSignatureBars = 0;
    f64 m_SongTempo = 1.0f;
    void PlayerThreadFunction();

private:
    std::vector<AliveAudioMidiMessage> m_MessageList;
    //std::thread * m_SequenceThread = nullptr;
    std::mutex m_MessageListMutex;
    std::mutex m_PlayerStateMutex;
    AliveAudio& mAliveAudio;


    void DoQuaterCallback()
    {
        if (m_QuarterCallback)
        {
            m_QuarterCallback();
        }
    }
};
