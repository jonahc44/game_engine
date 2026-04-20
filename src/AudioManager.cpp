#include "AudioManager.h"
#include "AudioHelper.h"
#include "SDL2_mixer/SDL_mixer.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::unordered_map<std::string, Mix_Chunk*> AudioManager::audio_chunks;

AudioManager::AudioManager() {
    AudioHelper::Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    AudioHelper::Mix_AllocateChannels(50);
}

void AudioManager::Play(int channel, const std::string& clip_name, bool does_loop) {
    if (audio_chunks.count(clip_name) == 0) {
        fs::path audio_path = fs::path("resources/audio") / (clip_name + ".wav");
        
        if (!fs::exists(audio_path)) {
            audio_path = fs::path("resources/audio") / (clip_name + ".ogg");
        }

        Mix_Chunk* chunk = AudioHelper::Mix_LoadWAV(audio_path.string().c_str());
        if (!chunk) {
            std::cout << "error: failed to play audio clip " << clip_name << '\n';
            return;
        }
        audio_chunks[clip_name] = chunk;
    }

    int loops = does_loop ? -1 : 0;
    AudioHelper::Mix_PlayChannel(channel, audio_chunks[clip_name], loops);
}

void AudioManager::Halt(int channel) {
    AudioHelper::Mix_HaltChannel(channel);
}

void AudioManager::SetVolume(int channel, float volume) {
    AudioHelper::Mix_Volume(channel, static_cast<int>(volume));
}

void AudioManager::LoadAudio(const std::string& audio_name, const std::string& alias, bool sfx) {
    fs::path wav_file = audio_path / (audio_name + ".wav");
    fs::path ogg_file = audio_path / (audio_name + ".ogg");
    
    if (fs::exists(wav_file)) {
        if (sfx) {
            sfx_map[alias] = AudioHelper::Mix_LoadWAV(wav_file.string().c_str());
        } else {
            bgm_chunks.emplace_back(AudioHelper::Mix_LoadWAV(wav_file.string().c_str()));
        }
    } else if (fs::exists(ogg_file)) {
        if (sfx) {
            sfx_map[alias] = AudioHelper::Mix_LoadWAV(ogg_file.string().c_str());
        } else {
            bgm_chunks.emplace_back(AudioHelper::Mix_LoadWAV(ogg_file.string().c_str()));
        }
    } else {
        std::cout << "error: failed to play audio clip " << audio_name;
        exit(0);
    }
}

void AudioManager::PlayBgm() {
    if (audio_idx < bgm_chunks.size() && Mix_Playing(0) == 0) {
        AudioHelper::Mix_PlayChannel(0, bgm_chunks[audio_idx], -1);
    }
}

void AudioManager::PlayEnd(bool bad) {
    if (bad) {
        AudioHelper::Mix_PlayChannel(0, bgm_chunks[audio_idx], 0);
    } else {
        AudioHelper::Mix_PlayChannel(0, bgm_chunks.back(), 0);
    }
}

void AudioManager::CleanupBgm() {
    if (bgm_chunks.size() > audio_idx++) {
        AudioHelper::Mix_HaltChannel(0);
    }
}

void AudioManager::PlayAudio(int channel, const std::string& audio_name, bool loop) {
    auto it = sfx_map.find(audio_name);
    if (it == sfx_map.end()) return;
    int num_loops = loop ? -1 : 0;
    Mix_Chunk* audio = sfx_map[audio_name];
    AudioHelper::Mix_PlayChannel(channel, audio, num_loops);
}