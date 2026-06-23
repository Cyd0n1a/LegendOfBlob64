#include "hud.h"
#include <libdragon.h>

/* ---- Layout (320x240 screen) ---- */
#define GLOB_X0      8    /* left edge of first glob square */
#define GLOB_Y0    224    /* top edge of glob row */
#define GLOB_SIZE   10    /* square size in pixels */
#define GLOB_STEP   14    /* x step between globs */

#define PWR_X0     252   /* power icon top-left */
#define PWR_Y0     223
#define PWR_SIZE    14   /* power icon square size */

#define BAR_X0     270   /* charge bar left */
#define BAR_Y0     228
#define BAR_W       42   /* max charge bar width */
#define BAR_H        6   /* charge bar height */

static inline color_t rgba32(uint32_t c) {
    return RGBA32((c>>24)&0xFF, (c>>16)&0xFF, (c>>8)&0xFF, c&0xFF);
}

void hud_draw(const blob_state_t *blob) {
    rdpq_mode_push();
    rdpq_set_mode_fill(RGBA32(0, 0, 0, 0));  /* enters fill mode */

    /* ---- Glob meter ---- */
    for (int i = 0; i < blob->globs_max; i++) {
        color_t col = (i < blob->globs)
            ? RGBA32(64, 255, 128, 255)   /* full glob: bright green */
            : RGBA32(26,  48,  32, 255);  /* empty glob: dark green */
        rdpq_set_fill_color(col);
        int x = GLOB_X0 + i * GLOB_STEP;
        rdpq_fill_rectangle(x, GLOB_Y0, x + GLOB_SIZE, GLOB_Y0 + GLOB_SIZE);
    }

    /* ---- Power icon ---- */
    if (blob->power != PWR_NONE) {
        rdpq_set_fill_color(rgba32(blob->power_color));
    } else {
        rdpq_set_fill_color(RGBA32(64, 64, 64, 255));
    }
    rdpq_fill_rectangle(PWR_X0, PWR_Y0, PWR_X0 + PWR_SIZE, PWR_Y0 + PWR_SIZE);

    /* ---- Charge bar background ---- */
    rdpq_set_fill_color(RGBA32(32, 32, 32, 255));
    rdpq_fill_rectangle(BAR_X0, BAR_Y0, BAR_X0 + BAR_W, BAR_Y0 + BAR_H);

    /* ---- Charge bar fill ---- */
    if (blob->power != PWR_NONE && blob->charge > 0) {
        uint16_t max_c = POWER_DEFS[blob->power].charge_max;
        int fill_w = (int)((float)blob->charge / (float)max_c * BAR_W);
        if (fill_w > 0) {
            rdpq_set_fill_color(rgba32(blob->power_color));
            rdpq_fill_rectangle(BAR_X0, BAR_Y0, BAR_X0 + fill_w, BAR_Y0 + BAR_H);
        }
    }

    rdpq_mode_pop();
}
