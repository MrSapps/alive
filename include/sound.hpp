#pragma once

class GameData;
class IAudioController;
class FileSystem;

class Sound
{
public:
    Sound(const Sound&) = delete;
    Sound& operator = (const Sound&) = delete;
    Sound(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    void Update();
    void Render(int w, int h);
private:
    GameData& mGameData;
    IAudioController& mAudioController;
    FileSystem& mFs;
};
