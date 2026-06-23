#ifndef PHYSICS_PHYSICS_H
#define PHYSICS_PHYSICS_H

/* Soft-body point-mass sim + swept-capsule world collision.
 * Two collision bodies: a swept capsule (authoritative, gameplay) and a
 * soft-body ring (visual + squeeze checks only). */

typedef struct {
    float x, y;         /* capsule centre */
    float vx, vy;       /* velocity */
    float radius;
    float half_height;
} capsule_t;

#endif
