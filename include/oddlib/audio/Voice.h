#pragma once

#include "SDL.h"
#include "oddlib/audio/AudioInterpolation.h"
#include "types.hpp"

enum ADSR_State {
    ADSR_State_attack,
    ADSR_State_decay,
    ADSR_State_sustain,
    ADSR_State_release,
};
class AliveAudioVoice
{
public:
    AliveAudioVoice() = default;
    AliveAudioVoice(const AliveAudioVoice&) = delete;
    AliveAudioVoice& operator = (const AliveAudioVoice&) = delete;

    class AliveAudioTone * m_Tone = nullptr;
    int		i_Program = 0;
    int		i_Note = 0;
    bool	b_Dead = false;
    f64	f_SampleOffset = 0;
    bool	b_NoteOn = true;
    f64	f_Velocity = 1.0f;
    f64	f_Pitch = 0.0f;
    bool    m_DebugDisableResampling = false;
    bool mbIgnoreLoops = false;

    f64	f_TrackDelay = 0; // Used by the sequencer for perfect timing
    bool	m_UsesNoteOffDelay = false;
    f64	f_NoteOffDelay = 0;

    f32 GetSample(AudioInterpolation interpolation, bool antiAliasFilteringEnabled);

private:
    f64 m_ADSR_Level = 0; // Value of the adsr curve at current time
    ADSR_State m_ADSR_State = ADSR_State_attack;
};
