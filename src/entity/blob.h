#ifndef ENTITY_BLOB_H
#define ENTITY_BLOB_H

#include <stdint.h>
#include "../input/input.h"
#include "../powers/powers.h"

typedef struct {
    float x, z;          /* world XZ position */
    float face_angle;    /* Y-axis facing direction (radians) */
    float speed_norm;    /* 0..1, smoothed movement speed */
    float wobble;        /* idle wobble phase accumulator */
    bool  rolling;       /* R held: faster movement + stagger on contact */
    uint8_t globs;       /* current health (Glob count) */
    uint8_t globs_max;   /* maximum Globs */
    power_id_t   power;        /* currently held copied power, PWR_NONE = none */
    uint16_t     charge;       /* remaining charge units */
    uint32_t     power_color;  /* RGBA8888 of held power (for HUD) */
} blob_state_t;

void blob_init(blob_state_t *b);
void blob_update(blob_state_t *b, const input_state_t *inp, float dt,
                 float room_half_w, float room_half_z);

#endif
