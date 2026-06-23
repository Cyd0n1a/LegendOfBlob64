#ifndef AUDIO_SYNTH_H
#define AUDIO_SYNTH_H

typedef enum {
    SFX_ABSORB,
    SFX_SPIT,
    SFX_ROLL_CHARGE,
    SFX_HOP,
    SFX_BONK,
    SFX_POWER_USE,
} sfx_id_t;

void synth_init(void);
void synth_play(sfx_id_t id);
void synth_poll(void);

#endif
