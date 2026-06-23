#ifndef WORLD_ROOM_H
#define WORLD_ROOM_H

#include <stdint.h>
#include <stdbool.h>
#include "../powers/powers.h"

#define ROOM_HALF_W  192.f
#define ROOM_HALF_Z  192.f

#define MAX_ENEMIES  4

#define STAGGER_DURATION  2.0f   /* seconds an enemy stays staggered */
#define ENEMY_RADIUS      16.f   /* collision radius (matches render scale) */

typedef struct {
    float      x, z;
    uint32_t   color;          /* RGBA8888 prim-color tint */
    bool       alive;
    float      stagger_timer;  /* > 0 = vulnerable, counts down */
    power_id_t power;          /* power granted on absorb */
} enemy_t;

typedef struct {
    enemy_t enemies[MAX_ENEMIES];
} room_t;

void room_init(room_t *r);

#endif
