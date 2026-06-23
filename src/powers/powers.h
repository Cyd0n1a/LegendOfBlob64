#ifndef POWERS_POWERS_H
#define POWERS_POWERS_H

#include <stdint.h>

typedef enum {
    PWR_NONE,
    PWR_EMBER,
    PWR_SPRING,
    PWR_SPARK,
    PWR_VINE,
    PWR_FROST,
    PWR_WEIGHT,
    PWR_PHASE,
} power_id_t;

typedef struct {
    uint8_t r, g, b;
} color_rgb_t;

typedef struct {
    power_id_t  id;
    color_rgb_t hue;
    uint16_t    charge_max;
    uint16_t    charge_cost;
} power_def_t;

extern const power_def_t POWER_DEFS[];

#endif
