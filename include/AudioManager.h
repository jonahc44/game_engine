#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include "SDL2_mixer/SDL_mixer.h"
#include <filesystem>
#include <vector>
#include <unordered_map>

class AudioManager {
    private:
        const std::filesystem::path audio_path = "resources/audio/";
        std::vector<Mix_Chunk*> bgm_chunks;
        int audio_idx = 0;
        std::unordered_map<std::string, Mix_Chunk*> sfx_map;
        static std::unordered_map<std::string, Mix_Chunk*> audio_chunks;

    public:
        AudioManager();
        void LoadAudio(const std::string& audio_name, const std::string& alias, bool sfx);
        void PlayBgm();
        void PlayAudio(int channel, const std::string& audio_name, bool loop);
        void PlayEnd(bool);
        void CleanupBgm();
        static void Play(int channel, const std::string& clip_name, bool does_loop);
        static void Halt(int channel);
        static void SetVolume(int channel, float volume);
};

#endif