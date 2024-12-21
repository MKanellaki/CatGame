#include "audio.h"
#include "common.h"

Mix_Chunk* ng_audio_load(const char *file)
{
#ifndef NO_AUDIO
    Mix_Chunk *audio = Mix_LoadWAV(file);

    // Making sure that the audio file was successfully loaded
    if (!audio)
        ng_die("Something went wrong, couldn't load audio file %s!", file);

    return audio;
#endif
}

Mix_Music* ng_music_load(const char *file)
{
#ifndef NO_AUDIO
    Mix_Music *audio = Mix_LoadMUS(file);

    // Making sure that the audio file was successfully loaded
    if (!audio)
        ng_die("Something went wrong, couldn't load audio file %s!", file);

    return audio;
#endif
}


void ng_audio_play(Mix_Chunk *audio)
{
#ifndef NO_AUDIO
    Mix_PlayChannel(-1, audio, 0);
#endif
}

int ng_return_channel(Mix_Chunk *audio, int dur)
{
#ifndef NO_AUDIO
    return Mix_PlayChannel(-1, audio, dur);
#endif
}

void ng_music_play(Mix_Music *audio)
{
#ifndef NO_AUDIO
    Mix_PlayMusic(audio, -1);
#endif
}
