#ifndef AUDIO_H
#define AUDIO_H

/* Tocador de som bem simples, só com SDL2 (sem SDL_mixer): carrega .wav
 * da pasta sounds/ e toca vários ao mesmo tempo, misturando no próprio
 * callback de áudio. */

int audio_init(void);
void audio_shutdown(void);

/* Carrega um .wav e devolve um handle (>=0), ou -1 em erro. */
int audio_load(const char *path);

/* Toca um som já carregado. volume vai de 0.0 a 1.0. Pode tocar o mesmo
 * handle várias vezes ao mesmo tempo (ex.: vários pássaros). */
void audio_play(int handle, float volume);

#endif
