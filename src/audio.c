#include "audio.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>

#define MAX_SOUNDS 16
#define MAX_VOICES 16

typedef struct {
    Uint8 *data;
    Uint32 len;
} Sound;

typedef struct {
    int active;
    int soundHandle;
    Uint32 pos;
    float volume;
} Voice;

static Sound g_sounds[MAX_SOUNDS];
static int g_soundCount = 0;
static Voice g_voices[MAX_VOICES];
static SDL_AudioDeviceID g_dev = 0;
static SDL_AudioSpec g_spec;

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    SDL_memset(stream, 0, (size_t)len);
    for (int i = 0; i < MAX_VOICES; i++) {
        Voice *v = &g_voices[i];
        if (!v->active) continue;
        Sound *s = &g_sounds[v->soundHandle];
        Uint32 remaining = s->len - v->pos;
        Uint32 chunk = (Uint32)len < remaining ? (Uint32)len : remaining;
        if (chunk > 0) {
            SDL_MixAudioFormat(stream, s->data + v->pos, g_spec.format, chunk,
                                (int)(v->volume * SDL_MIX_MAXVOLUME));
            v->pos += chunk;
        }
        if (v->pos >= s->len) v->active = 0;
    }
}

int audio_init(void) {
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            fprintf(stderr, "[audio] SDL_InitSubSystem falhou: %s\n", SDL_GetError());
            return -1;
        }
    }
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16LSB;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;

    g_dev = SDL_OpenAudioDevice(NULL, 0, &want, &g_spec, 0);
    if (g_dev == 0) {
        fprintf(stderr, "[audio] SDL_OpenAudioDevice falhou: %s\n", SDL_GetError());
        return -1;
    }
    SDL_PauseAudioDevice(g_dev, 0);
    return 0;
}

void audio_shutdown(void) {
    if (g_dev) { SDL_CloseAudioDevice(g_dev); g_dev = 0; }
    for (int i = 0; i < g_soundCount; i++)
        if (g_sounds[i].data) SDL_FreeWAV(g_sounds[i].data);
    g_soundCount = 0;
}

int audio_load(const char *path) {
    if (g_soundCount >= MAX_SOUNDS) return -1;

    SDL_AudioSpec fileSpec;
    Uint8 *fileData = NULL;
    Uint32 fileLen = 0;
    if (!SDL_LoadWAV(path, &fileSpec, &fileData, &fileLen)) {
        fprintf(stderr, "[audio] não consegui carregar %s: %s\n", path, SDL_GetError());
        return -1;
    }

    Uint8 *finalData = fileData;
    Uint32 finalLen = fileLen;
    int needFree = 0;

    if (fileSpec.format != g_spec.format || fileSpec.channels != g_spec.channels || fileSpec.freq != g_spec.freq) {
        SDL_AudioCVT cvt;
        int rc = SDL_BuildAudioCVT(&cvt, fileSpec.format, fileSpec.channels, fileSpec.freq,
                                    g_spec.format, g_spec.channels, g_spec.freq);
        if (rc < 0) {
            fprintf(stderr, "[audio] não consegui converter %s: %s\n", path, SDL_GetError());
            SDL_FreeWAV(fileData);
            return -1;
        }
        if (rc == 0) {
            /* já está no formato certo, nada a fazer */
        } else {
            cvt.len = (int)fileLen;
            cvt.buf = (Uint8 *)SDL_malloc((size_t)(cvt.len * cvt.len_mult));
            SDL_memcpy(cvt.buf, fileData, fileLen);
            SDL_ConvertAudio(&cvt);
            finalData = cvt.buf;
            finalLen = (Uint32)cvt.len_cvt;
            needFree = 1;
            SDL_FreeWAV(fileData);
        }
    }

    int handle = g_soundCount++;
    g_sounds[handle].data = finalData;
    g_sounds[handle].len = finalLen;
    (void)needFree; /* finalData é liberado em audio_shutdown independente da origem */
    return handle;
}

void audio_play(int handle, float volume) {
    if (handle < 0 || handle >= g_soundCount || g_dev == 0) return;
    SDL_LockAudioDevice(g_dev);
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!g_voices[i].active) {
            g_voices[i].active = 1;
            g_voices[i].soundHandle = handle;
            g_voices[i].pos = 0;
            g_voices[i].volume = volume;
            break;
        }
    }
    SDL_UnlockAudioDevice(g_dev);
}
