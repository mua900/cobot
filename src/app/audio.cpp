#include "audio.hpp"
#include "common.hpp"
#include "log.hpp"

#include <cmath>

bool AudioPlayer::create() {
    SDL_AudioDeviceID playback_device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    SDL_AudioDeviceID sdl_device = SDL_OpenAudioDevice(playback_device, nullptr);
    if (!sdl_device)
    {
        SDL_Log("Could not open audio device using SDL: %s\n", SDL_GetError());
        return false;
    }

    MIX_Mixer* mix_mixer = MIX_CreateMixerDevice(sdl_device, nullptr);
    if (!mix_mixer)
    {
        log_error("Couldn't create MIX_Mixer object: %s\n", SDL_GetError());
        return false;
    }

    this->device = device;
    this->mixer = mix_mixer;
    return true;
}

void AudioPlayer::cleanup() {
    MIX_DestroyMixer(mixer);
}

MIX_Audio* AudioPlayer::load_audio(const char* path) const
{
    return MIX_LoadAudio(mixer, path, true);
}

TrackId AudioPlayer::add_track(const char* path)
{
    MIX_Audio* audio = MIX_LoadAudio(mixer, path, true);
    if (!audio)
    {
        return NullTrackId;
    }

    return make_track(audio);
}

TrackId AudioPlayer::make_track(MIX_Audio* audio)
{
    MIX_Track* track = MIX_CreateTrack(mixer);
    if (!track)
    {
        return NullTrackId;
    }

    MIX_SetTrackAudio(track, audio);

    return tracks.add(track);
}
