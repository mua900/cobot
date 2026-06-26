#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "common.hpp"
#include "template.hpp"

using TrackId = u32;
static constexpr TrackId NullTrackId = -1;

struct AudioPlayer {
    SDL_AudioDeviceID device = {};
    MIX_Mixer* mixer = {};
    // each track owns a single audio to play and we can query from SDL_mixer the audio object so we don't need to store it separately
    BucketList<MIX_Track*> tracks;

    bool create();
    void cleanup();
    MIX_Audio* load_audio(const char* path) const;
    TrackId add_track(const char* path);
    TrackId make_track(MIX_Audio* audio);
    void unload_track(TrackId& track);
    void tag_track(TrackId trakc, const char* tag);
    int get_track_count() const { return tracks.count(); }
    void remove_track(int index);

    void set_master_gain(float gain);

    void play_track(TrackId track);
    void pause_track(TrackId track);
    void set_track_gain(float gain);

    void play_tag(const char* track_tag);
    void pause_tag(const char* track_tag);
    void set_tag_gain(float gain);
};
