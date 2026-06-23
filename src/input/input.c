#include "input.h"
#include <libdragon.h>
#include <math.h>
#include <string.h>

static input_state_t state;

void input_init(void) {
    memset(&state, 0, sizeof(state));
}

void input_poll(void) {
    joypad_poll();

    joypad_inputs_t  in  = joypad_get_inputs(JOYPAD_PORT_1);
    joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    float ax = (float)in.stick_x / 80.f;
    float ay = (float)in.stick_y / 80.f;
    float mag = sqrtf(ax * ax + ay * ay);
    if (mag > 1.f) { ax /= mag; ay /= mag; }

    state.move_x    = ax;
    state.move_y    = ay;
    state.btn_hop    = (bool)btn.a;
    state.btn_power  = (bool)btn.b;
    state.btn_absorb = (bool)btn.z;
    state.btn_roll   = (bool)in.btn.r;
    state.btn_spit   = (bool)btn.c_up;
    state.btn_map    = (bool)btn.c_down;
    state.btn_start  = (bool)btn.start;
}

const input_state_t *input_get(void) { return &state; }

void input_rumble(bool on) {
    (void)on;
    /* Rumble Pak wired here once accessory detection is in place */
}
