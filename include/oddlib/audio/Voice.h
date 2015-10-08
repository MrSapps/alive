#pragma once

#include "SDL.h"
#include "oddlib/audio/AudioInterpolation.h"

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
    double	f_SampleOffset = 0;
    bool	b_NoteOn = true;
    double	f_Velocity = 1.0f;
    double	f_Pitch = 0.0f;
    bool    m_DebugDisableResampling = false;

    int		i_TrackID = 0; // This is used to distinguish between sounds fx and music
    double	f_TrackDelay = 0; // Used by the sequencer for perfect timing
    bool	m_UsesNoteOffDelay = false;
    double	f_NoteOffDelay = 0;

    float GetSample(AudioInterpolation interpolation, bool antiAliasFilteringEnabled);

private:
    double m_ADSR_Level = 0; // Value of the adsr curve at current time
    ADSR_State m_ADSR_State = ADSR_State_attack;
    float m_LastSample = 0.0f;
};
