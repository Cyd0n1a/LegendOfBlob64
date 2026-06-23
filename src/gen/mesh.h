#ifndef GEN_MESH_H
#define GEN_MESH_H

#include <t3d/t3d.h>
#include <stdint.h>

/* Number of T3DVertPacked entries for a UV sphere (each entry holds 2 verts). */
#define SPHERE_PACKED_COUNT(lat, lon)  ((lat) * (lon) * 2)

void mesh_gen_sphere(T3DVertPacked *out, int lat, int lon, float radius,
                     uint32_t rgba);

void mesh_gen_capsule(T3DVertPacked *out, int lat, int lon,
                      float radius, float half_height, uint32_t rgba);

/* Number of T3DVertPacked entries for a flat tile floor grid. */
#define FLOOR_PACKED_COUNT(w, h)  ((w) * (h) * 2)

void mesh_gen_floor(T3DVertPacked *out, int tiles_w, int tiles_h, float tile_sz);

#endif
