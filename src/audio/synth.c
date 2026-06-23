#include "synth.h"
#include <libdragon.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE  44100
#define MAX_VOICES   8

typedef struct {
    int    active;
    int    pos;
    int    len;
    float  phase;
    float  freq_start;
    float  freq_end;
    int    wave;        /* 0=sine 1=square 2=saw 3=noise */
    float  volume;
    float  pan_l;
    float  pan_r;
} voice_t;

static voice_t voices[MAX_VOICES];

static void voice_start(int slot, const voice_t *cfg) {
    voices[slot]        = *cfg;
    voices[slot].active = 1;
    voices[slot].pos    = 0;
    voices[slot].phase  = 0.f;
}

void synth_play(sfx_id_t id) {
    voice_t v;
    memset(&v, 0, sizeof(v));
    v.pan_l = v.pan_r = 0.707f;

    switch (id) {
        case SFX_ABSORB:
            v.len = (int)(SAMPLE_RATE * 0.20f);
            v.freq_start = 400.f; v.freq_end = 1200.f;
            v.wave = 0; v.volume = 0.50f;
            voice_start(0, &v);
            break;
        case SFX_SPIT:
            v.len = (int)(SAMPLE_RATE * 0.18f);
            v.freq_start = 1000.f; v.freq_end = 200.f;
            v.wave = 0; v.volume = 0.45f;
            voice_start(1, &v);
            break;
        case SFX_ROLL_CHARGE:
            v.len = (int)(SAMPLE_RATE * 0.30f);
            v.freq_start = 80.f; v.freq_end = 200.f;
            v.wave = 3; v.volume = 0.30f;
            voice_start(2, &v);
            break;
        case SFX_HOP:
            v.len = (int)(SAMPLE_RATE * 0.12f);
            v.freq_start = 600.f; v.freq_end = 900.f;
            v.wave = 0; v.volume = 0.40f;
            voice_start(3, &v);
            break;
        case SFX_BONK:
            v.len = (int)(SAMPLE_RATE * 0.10f);
            v.freq_start = 200.f; v.freq_end = 50.f;
            v.wave = 3; v.volume = 0.35f;
            voice_start(4, &v);
            break;
        case SFX_POWER_USE:
            v.len = (int)(SAMPLE_RATE * 0.22f);
            v.freq_start = 800.f; v.freq_end = 1600.f;
            v.wave = 1; v.volume = 0.40f;
            voice_start(5, &v);
            break;
    }
}

static void synth_mix_into(short *buf, int n_frames) {
    for (int i = 0; i < n_frames; i++) {
        float sample_L = 0.f, sample_R = 0.f;

        for (int v = 0; v < MAX_VOICES; v++) {
            voice_t *vp = &voices[v];
            if (!vp->active) continue;

            float t    = (float)vp->pos / (float)vp->len;
            float freq = vp->freq_start + (vp->freq_end - vp->freq_start) * t;

            vp->phase += freq / (float)SAMPLE_RATE;
            if (vp->phase >= 1.f) vp->phase -= 1.f;

            float osc;
            switch (vp->wave) {
                case 0:  osc = sinf(vp->phase * 6.28318f);                           break;
                case 1:  osc = (vp->phase < 0.5f) ? 1.f : -1.f;                     break;
                case 2:  osc = vp->phase * 2.f - 1.f;                               break;
                default: osc = ((float)rand() / (float)RAND_MAX) * 2.f - 1.f;       break;
            }

            float env  = 1.f - t * t;
            float mono = osc * env * vp->volume;
            sample_L  += mono * vp->pan_l;
            sample_R  += mono * vp->pan_r;

            vp->pos++;
            if (vp->pos >= vp->len) vp->active = 0;
        }

        if (sample_L >  1.f) sample_L =  1.f;
        if (sample_L < -1.f) sample_L = -1.f;
        if (sample_R >  1.f) sample_R =  1.f;
        if (sample_R < -1.f) sample_R = -1.f;

        int L = (int)buf[i * 2]     + (short)(sample_L * 28000.f);
        int R = (int)buf[i * 2 + 1] + (short)(sample_R * 28000.f);
        buf[i * 2]     = (short)(L >  32767 ?  32767 : L < -32768 ? -32768 : L);
        buf[i * 2 + 1] = (short)(R >  32767 ?  32767 : R < -32768 ? -32768 : R);
    }
}

void synth_init(void) {
    memset(voices, 0, sizeof(voices));
    audio_init(SAMPLE_RATE, 4);
    mixer_init(32);
}

void synth_poll(void) {
    while (audio_can_write()) {
        short *buf = audio_write_begin();
        int n = audio_get_buffer_length();
        mixer_poll(buf, n);
        synth_mix_into(buf, n);
        audio_write_end();
    }
}
