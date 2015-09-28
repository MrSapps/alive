#pragma once

#include <memory>
#include <vector>
#include <string>

class GameData;
class IAudioController;
class FileSystem;

class Sound
{
public:
    Sound(const Sound&) = delete;
    Sound& operator = (const Sound&) = delete;
    Sound(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    ~Sound();
    void Update();
    void Render(int w, int h);
private:
    void BarLoop();
private:
    GameData& mGameData;
    IAudioController& mAudioController;
    FileSystem& mFs;
    std::unique_ptr<class SequencePlayer> mSeqPlayer;
    int mTargetSong = -1;
    bool mLoopSong = false;
    std::vector<std::string> mThemes;
};
