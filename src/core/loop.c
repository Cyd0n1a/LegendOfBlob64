#include "loop.h"
#include <libdragon.h>
#include "../input/input.h"
#include "../audio/synth.h"
#include "../render/render.h"
#include "../entity/blob.h"
#include "../world/room.h"
#include "../powers/powers.h"

#define STEP_US        16667   /* 60 Hz fixed logic timestep */
#define ABSORB_RANGE   50.f   /* Z-absorb range (world units) */

/* Touch radius for roll-stagger: blob body + enemy body */
#define STAGGER_TOUCH  (24.f + ENEMY_RADIUS)

static blob_state_t blob;
static room_t       room;

static void update(float dt) {
    input_poll();
    const input_state_t *inp = input_get();

    blob_update(&blob, inp, dt, ROOM_HALF_W, ROOM_HALF_Z);

    /* ---- Enemy stagger timers ---- */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!room.enemies[i].alive) continue;
        if (room.enemies[i].stagger_timer > 0.f) {
            room.enemies[i].stagger_timer -= dt;
            if (room.enemies[i].stagger_timer < 0.f)
                room.enemies[i].stagger_timer = 0.f;
        }
    }

    /* ---- Roll stagger: touching an unstaggered enemy while rolling ---- */
    if (blob.rolling) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!room.enemies[i].alive) continue;
            if (room.enemies[i].stagger_timer > 0.f) continue;
            float dx = room.enemies[i].x - blob.x;
            float dz = room.enemies[i].z - blob.z;
            if (dx*dx + dz*dz < STAGGER_TOUCH * STAGGER_TOUCH) {
                room.enemies[i].stagger_timer = STAGGER_DURATION;
                synth_play(SFX_BONK);
            }
        }
    }

    /* ---- Absorb: Z on a staggered enemy within range ---- */
    if (inp->btn_absorb) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!room.enemies[i].alive) continue;
            if (room.enemies[i].stagger_timer <= 0.f) continue;
            float dx = room.enemies[i].x - blob.x;
            float dz = room.enemies[i].z - blob.z;
            if (dx*dx + dz*dz < ABSORB_RANGE * ABSORB_RANGE) {
                room.enemies[i].alive = false;

                /* Grant copied power from enemy */
                power_id_t pid = room.enemies[i].power;
                blob.power       = pid;
                blob.charge      = POWER_DEFS[pid].charge_max;
                color_rgb_t hue  = POWER_DEFS[pid].hue;
                blob.power_color = ((uint32_t)hue.r << 24) |
                                   ((uint32_t)hue.g << 16) |
                                   ((uint32_t)hue.b <<  8) | 0xFF;

                synth_play(SFX_ABSORB);
                break;
            }
        }
    }

    /* ---- Use power: B button ---- */
    if (inp->btn_power && blob.power != PWR_NONE) {
        uint16_t cost = POWER_DEFS[blob.power].charge_cost;
        if (blob.charge >= cost) {
            blob.charge -= cost;
            synth_play(SFX_POWER_USE);
        }
        if (blob.charge == 0) {
            blob.power       = PWR_NONE;
            blob.power_color = 0x808080FF;
        }
    }

    /* ---- Spit: C-up discards current power ---- */
    if (inp->btn_spit && blob.power != PWR_NONE) {
        blob.power       = PWR_NONE;
        blob.charge      = 0;
        blob.power_color = 0x808080FF;
        synth_play(SFX_SPIT);
    }
}

void loop_run(void) {
    blob_init(&blob);
    room_init(&room);

    long long prev  = timer_ticks();
    long long accum = 0;
    float total_time = 0.f;

    while (1) {
        long long now  = timer_ticks();
        long long diff = TIMER_MICROS_LL(now - prev);
        prev = now;
        if (diff > 100000) diff = 100000;
        accum      += diff;
        total_time += (float)diff / 1000000.f;

        while (accum >= STEP_US) {
            update(STEP_US / 1000000.f);
            accum -= STEP_US;
        }

        synth_poll();

        surface_t *disp = display_get();
        render_frame(disp, &blob, &room, total_time);
    }
}
