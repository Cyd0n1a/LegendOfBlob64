#include "powers.h"

const power_def_t POWER_DEFS[] = {
    [PWR_NONE]   = { PWR_NONE,   {128, 128, 128},   0,   0 },
    [PWR_EMBER]  = { PWR_EMBER,  {255,  48,  16}, 100,  10 },
    [PWR_SPRING] = { PWR_SPRING, {255, 150,  20},  80,  15 },
    [PWR_SPARK]  = { PWR_SPARK,  {240, 220,  30}, 120,  12 },
    [PWR_VINE]   = { PWR_VINE,   { 40, 200,  60},  90,  10 },
    [PWR_FROST]  = { PWR_FROST,  { 80, 180, 255}, 100,  10 },
    [PWR_WEIGHT] = { PWR_WEIGHT, { 80,  60, 200},  70,  20 },
    [PWR_PHASE]  = { PWR_PHASE,  {200,  60, 255}, 110,  15 },
};
