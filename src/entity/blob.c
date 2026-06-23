#include "blob.h"
#include <libdragon.h>
#include <math.h>

#define BLOB_SPEED    90.f
#define ROLL_SPEED   160.f
#define BLOB_RADIUS   24.f
#define START_GLOBS    3

void blob_init(blob_state_t *b) {
    b->x = b->z = 0.f;
    b->face_angle = 0.f;
    b->speed_norm = 0.f;
    b->wobble     = 0.f;
    b->rolling    = false;
    b->globs      = START_GLOBS;
    b->globs_max  = START_GLOBS;
    b->power      = PWR_NONE;
    b->charge     = 0;
    b->power_color = 0x808080FF;
}

void blob_update(blob_state_t *b, const input_state_t *inp, float dt,
                 float room_half_w, float room_half_z) {
    b->rolling = inp->btn_roll;
    float spd  = b->rolling ? ROLL_SPEED : BLOB_SPEED;

    float nx = b->x + inp->move_x * spd * dt;
    float nz = b->z - inp->move_y * spd * dt;

    float bx = room_half_w - BLOB_RADIUS;
    float bz = room_half_z - BLOB_RADIUS;
    if (nx < -bx) nx = -bx;
    if (nx >  bx) nx =  bx;
    if (nz < -bz) nz = -bz;
    if (nz >  bz) nz =  bz;

    b->x = nx;
    b->z = nz;

    float move_mag = sqrtf(inp->move_x * inp->move_x + inp->move_y * inp->move_y);
    float k = (move_mag > b->speed_norm) ? 10.f : 5.f;
    b->speed_norm += (move_mag - b->speed_norm) * k * dt;
    if (b->speed_norm < 0.f) b->speed_norm = 0.f;
    if (b->speed_norm > 1.f) b->speed_norm = 1.f;

    if (move_mag > 0.05f)
        b->face_angle = atan2f(inp->move_x, -inp->move_y);

    b->wobble += dt * 2.4f;
}
