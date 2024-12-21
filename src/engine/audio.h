#ifndef _NG_AUDIO_H
#define _NG_AUDIO_H

#include <SDL2/SDL_mixer.h>

Mix_Chunk* ng_audio_load(const char *file);
void ng_audio_play(Mix_Chunk *audio);
Mix_Music* ng_music_load(const char *file);
void ng_music_play(Mix_Music *audio);
int ng_return_channel(Mix_Chunk *audio, int dur);

#endif
