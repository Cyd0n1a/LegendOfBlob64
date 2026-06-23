#include "render.h"
#include "../gen/mesh.h"
#include "../ui/hud.h"
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <libdragon.h>
#include <math.h>

/* ---- Sphere geometry config ---- */
#define BLOB_LAT   12
#define BLOB_LON   12
#define BLOB_R     24.f
#define BLOB_PKCD  SPHERE_PACKED_COUNT(BLOB_LAT, BLOB_LON)  /* 288 packed = 576 verts */
#define BATCH_SZ   32    /* packed entries per batch; pass BATCH_SZ*2 individual verts to t3d_vert_load */
#define BATCH_N    (BLOB_PKCD / BATCH_SZ)                   /* 9 batches */
#define QPB        (BATCH_SZ / 2)                           /* quads per batch = 16 */

/* ---- Floor geometry config ---- */
#define FL_TW      12
#define FL_TH      12
#define FL_TILE    32.f
#define FL_PKCD    FLOOR_PACKED_COUNT(FL_TW, FL_TH)         /* 288 packed */
#define FL_BSZ     32
#define FL_BN      (FL_PKCD / FL_BSZ)                       /* 9 batches */
#define FL_QPB     (FL_BSZ / 2)                             /* 16 tiles per batch */

/* ---- Static resources (allocated once in render_init) ---- */
static T3DViewport    viewport;
static surface_t      zbuf;

static T3DVertPacked *blob_verts;       /* purple, for the player blob */
static T3DVertPacked *enemy_verts;      /* white, tinted via prim color per-enemy */
static T3DMat4FP     *blob_mat_fp;      /* updated each frame (uncached) */
static rspq_block_t  *blob_dpl;

static T3DVertPacked *floor_verts;
static T3DMat4FP     *floor_mat_fp;    /* identity, never changes */
static rspq_block_t  *floor_dpl;

static T3DMat4FP     *enemy_mat_fp[MAX_ENEMIES]; /* updated each frame */

/* ---- Lighting palettes ---- */
static const uint8_t AMB_MAIN[4]  = {  60,  60,  80, 0xFF };
static const uint8_t AMB_FLOOR[4] = { 160, 160, 160, 0xFF };
static const uint8_t LIGHT_COL[4] = { 255, 240, 220, 0xFF };

/* ---- Helpers ---- */
static inline color_t unpack_rgba(uint32_t c) {
    return RGBA32((c>>24)&0xFF, (c>>16)&0xFF, (c>>8)&0xFF, c&0xFF);
}

/* Record a sphere draw block using verts from `src` and matrix from `mat`. */
static rspq_block_t *record_sphere_dpl(const T3DVertPacked *src, T3DMat4FP *mat) {
    rspq_block_t *dpl;
    rspq_block_begin();
        t3d_matrix_push(mat);
        for (int b = 0; b < BATCH_N; b++) {
            t3d_vert_load(src + b * BATCH_SZ, 0, BATCH_SZ * 2);
            for (int q = 0; q < QPB; q++) {
                t3d_tri_draw(4*q,   4*q+1, 4*q+2);
                t3d_tri_draw(4*q+1, 4*q+3, 4*q+2);
            }
            t3d_tri_sync();
        }
        t3d_matrix_pop(1);
    dpl = rspq_block_end();
    return dpl;
}

void render_init(void) {
    viewport    = t3d_viewport_create();
    zbuf        = surface_alloc(FMT_RGBA16, 320, 240);

    /* Blob: purple sphere with lighting */
    blob_verts  = malloc_uncached(sizeof(T3DVertPacked) * BLOB_PKCD);
    blob_mat_fp = malloc_uncached(sizeof(T3DMat4FP));
    mesh_gen_sphere(blob_verts, BLOB_LAT, BLOB_LON, BLOB_R, 0xA040FFFF);
    t3d_mat4fp_identity(blob_mat_fp);
    blob_dpl = record_sphere_dpl(blob_verts, blob_mat_fp);

    /* Enemies: white sphere — prim color provides per-enemy tint */
    enemy_verts = malloc_uncached(sizeof(T3DVertPacked) * BLOB_PKCD);
    mesh_gen_sphere(enemy_verts, BLOB_LAT, BLOB_LON, BLOB_R, 0xFFFFFFFF);

    /* Floor: checkerboard, flat (no lighting computation) */
    floor_verts  = malloc_uncached(sizeof(T3DVertPacked) * FL_PKCD);
    floor_mat_fp = malloc_uncached(sizeof(T3DMat4FP));
    mesh_gen_floor(floor_verts, FL_TW, FL_TH, FL_TILE);
    t3d_mat4fp_identity(floor_mat_fp);

    rspq_block_begin();
        t3d_matrix_push(floor_mat_fp);
        for (int b = 0; b < FL_BN; b++) {
            t3d_vert_load(floor_verts + b * FL_BSZ, 0, FL_BSZ * 2);
            for (int q = 0; q < FL_QPB; q++) {
                /* CCW from above: A(0),C(2),B(1) then B(1),C(2),D(3) */
                t3d_tri_draw(4*q, 4*q+2, 4*q+1);
                t3d_tri_draw(4*q+1, 4*q+2, 4*q+3);
            }
            t3d_tri_sync();
        }
        t3d_matrix_pop(1);
    floor_dpl = rspq_block_end();

    /* Enemy matrices */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemy_mat_fp[i] = malloc_uncached(sizeof(T3DMat4FP));
        t3d_mat4fp_identity(enemy_mat_fp[i]);
    }
}

void render_frame(surface_t *disp, const blob_state_t *blob,
                  const room_t *room, float time) {
    /* --- Camera: Zelda LttP-style 3/4 overhead, pulled back to show full room --- */
    T3DVec3 cam_pos    = {{ blob->x, 220.f, blob->z + 200.f }};
    T3DVec3 cam_target = {{ blob->x,   0.f, blob->z         }};
    T3DVec3 up         = {{ 0.f, 1.f, 0.f }};
    t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(62.f), 10.f, 700.f);
    t3d_viewport_look_at(&viewport, &cam_pos, &cam_target, &up);

    /* --- Blob matrix: squish + idle wobble + translation --- */
    float xz_s = 1.f + blob->speed_norm * 0.12f;
    float y_s  = (1.f - blob->speed_norm * 0.25f) *
                 (1.f + fm_sinf(blob->wobble) * 0.04f);
    float hs   = blob->face_angle * 0.5f;
    float bquat[4]  = { 0.f, fm_sinf(hs), 0.f, fm_cosf(hs) };
    float bscale[3] = { xz_s, y_s, xz_s };
    float bpos[3]   = { blob->x, BLOB_R * 0.5f, blob->z };
    t3d_mat4fp_from_srt(blob_mat_fp, bscale, bquat, bpos);

    /* --- Enemy matrices: bob at staggered phases --- */
    float id_quat[4]  = { 0.f, 0.f, 0.f, 1.f };
    float e_scale[3]  = { 0.67f, 0.67f, 0.67f };  /* visible radius ~16 */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!room->enemies[i].alive) continue;
        float bob     = fm_sinf(time * 2.5f + (float)i * 1.5f) * 6.f + BLOB_R * 0.5f;
        float epos[3] = { room->enemies[i].x, bob, room->enemies[i].z };
        t3d_mat4fp_from_srt(enemy_mat_fp[i], e_scale, id_quat, epos);
    }

    /* --- Begin frame --- */
    rdpq_attach(disp, &zbuf);
    t3d_frame_start();
    t3d_viewport_attach(&viewport);
    t3d_screen_clear_color(RGBA32(20, 10, 30, 255));
    t3d_screen_clear_depth();

    T3DVec3 light_dir = {{ -0.577f, 0.577f, -0.577f }};

    /* --- Floor pass: flat vertex colors, no per-vertex lighting --- */
    t3d_light_set_ambient(AMB_FLOOR);
    t3d_light_set_count(0);
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | T3D_FLAG_NO_LIGHT);
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rspq_block_run(floor_dpl);

    /* --- Blob pass: purple with directional lighting --- */
    t3d_light_set_ambient(AMB_MAIN);
    t3d_light_set_directional(0, LIGHT_COL, &light_dir);
    t3d_light_set_count(1);
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH);
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rspq_block_run(blob_dpl);

    /* --- Enemy pass: white sphere × prim color; staggered enemies flash --- */
    rdpq_mode_combiner(RDPQ_COMBINER1((SHADE, 0, PRIM, 0), (0, 0, 0, SHADE)));
    bool flash_on = ((int)(time * 10.f) & 1) == 0;
    t3d_matrix_push_pos(1);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!room->enemies[i].alive) continue;
        t3d_matrix_set(enemy_mat_fp[i], false);
        bool staggered = room->enemies[i].stagger_timer > 0.f;
        color_t col = (staggered && flash_on)
            ? RGBA32(255, 255, 200, 255)
            : unpack_rgba(room->enemies[i].color);
        rdpq_set_prim_color(col);
        for (int b = 0; b < BATCH_N; b++) {
            t3d_vert_load(enemy_verts + b * BATCH_SZ, 0, BATCH_SZ * 2);
            for (int q = 0; q < QPB; q++) {
                t3d_tri_draw(4*q,   4*q+1, 4*q+2);
                t3d_tri_draw(4*q+1, 4*q+3, 4*q+2);
            }
            t3d_tri_sync();
        }
    }
    t3d_matrix_pop(1);

    hud_draw(blob);

    rdpq_detach_show();
}
