#pragma once

#include <memory>
#include <vector>
#include <string>
#include "oddlib/audio/AliveAudio.h"

class GameData;
class IAudioController;
class FileSystem;
class GuiContext;

class Sound
{
public:
    Sound(const Sound&) = delete;
    Sound& operator = (const Sound&) = delete;
    Sound(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    ~Sound();
    void Update();
    void Render(GuiContext *gui, int w, int h);
private:
    void BarLoop();
    void ChangeTheme(FileSystem& fs, const std::deque<std::string>& parts);
private:
    GameData& mGameData;
    IAudioController& mAudioController;
    FileSystem& mFs;
    std::unique_ptr<class SequencePlayer> mSeqPlayer;
    int mTargetSong = -1;
    bool mLoopSong = false;
    std::vector<std::string> mThemes;
    AliveAudio mAliveAudio;
};
