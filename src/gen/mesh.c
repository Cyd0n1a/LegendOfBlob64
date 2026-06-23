#include "mesh.h"
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <math.h>

#ifndef T3D_PI
#define T3D_PI 3.14159265f
#endif

void mesh_gen_sphere(T3DVertPacked *out, int lat, int lon, float radius,
                     uint32_t rgba) {
    int idx = 0;
    for (int i = 0; i < lat; i++) {
        float th0 = T3D_PI * (float)i       / lat;
        float th1 = T3D_PI * (float)(i + 1) / lat;
        float s0 = sinf(th0), c0 = cosf(th0);
        float s1 = sinf(th1), c1 = cosf(th1);

        for (int j = 0; j < lon; j++) {
            float ph0 = 2.f * T3D_PI * (float)j       / lon;
            float ph1 = 2.f * T3D_PI * (float)(j + 1) / lon;
            float cp0 = cosf(ph0), sp0 = sinf(ph0);
            float cp1 = cosf(ph1), sp1 = sinf(ph1);

            T3DVec3 nA = {{ s0*cp0, c0, s0*sp0 }};
            T3DVec3 nB = {{ s0*cp1, c0, s0*sp1 }};
            T3DVec3 nC = {{ s1*cp0, c1, s1*sp0 }};
            T3DVec3 nD = {{ s1*cp1, c1, s1*sp1 }};

            out[idx++] = (T3DVertPacked){
                .posA  = { (int16_t)(nA.v[0]*radius), (int16_t)(nA.v[1]*radius), (int16_t)(nA.v[2]*radius) },
                .normA = t3d_vert_pack_normal(&nA), .rgbaA = rgba, .stA = {0,0},
                .posB  = { (int16_t)(nB.v[0]*radius), (int16_t)(nB.v[1]*radius), (int16_t)(nB.v[2]*radius) },
                .normB = t3d_vert_pack_normal(&nB), .rgbaB = rgba, .stB = {0,0},
            };
            out[idx++] = (T3DVertPacked){
                .posA  = { (int16_t)(nC.v[0]*radius), (int16_t)(nC.v[1]*radius), (int16_t)(nC.v[2]*radius) },
                .normA = t3d_vert_pack_normal(&nC), .rgbaA = rgba, .stA = {0,0},
                .posB  = { (int16_t)(nD.v[0]*radius), (int16_t)(nD.v[1]*radius), (int16_t)(nD.v[2]*radius) },
                .normB = t3d_vert_pack_normal(&nD), .rgbaB = rgba, .stB = {0,0},
            };
        }
    }
}

void mesh_gen_floor(T3DVertPacked *out, int w, int h, float tile_sz) {
    T3DVec3  up   = {{ 0.f, 1.f, 0.f }};
    uint16_t norm = t3d_vert_pack_normal(&up);
    int idx = 0;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            float x0 = (j - w * 0.5f) * tile_sz;
            float x1 = x0 + tile_sz;
            float z0 = (i - h * 0.5f) * tile_sz;
            float z1 = z0 + tile_sz;
            uint32_t col = ((i + j) & 1) ? 0x383844FF : 0x404858FF;
            /* packed[2k]  : A=(x0,z0), B=(x1,z0) → RSP slots 4k, 4k+1 */
            /* packed[2k+1]: C=(x0,z1), D=(x1,z1) → RSP slots 4k+2, 4k+3 */
            out[idx++] = (T3DVertPacked){
                .posA={(int16_t)x0,0,(int16_t)z0},.normA=norm,.rgbaA=col,.stA={0,0},
                .posB={(int16_t)x1,0,(int16_t)z0},.normB=norm,.rgbaB=col,.stB={0,0},
            };
            out[idx++] = (T3DVertPacked){
                .posA={(int16_t)x0,0,(int16_t)z1},.normA=norm,.rgbaA=col,.stA={0,0},
                .posB={(int16_t)x1,0,(int16_t)z1},.normB=norm,.rgbaB=col,.stB={0,0},
            };
        }
    }
}

void mesh_gen_capsule(T3DVertPacked *out, int lat, int lon,
                      float radius, float half_height, uint32_t rgba) {
    int half_lat = lat / 2;
    int idx = 0;

    for (int pass = 0; pass < 2; pass++) {
        float y_off = (pass == 0) ? half_height : -half_height;

        for (int i = 0; i < half_lat; i++) {
            float t0 = T3D_PI * 0.5f * (float)i       / half_lat;
            float t1 = T3D_PI * 0.5f * (float)(i + 1) / half_lat;
            /* Bottom hemisphere: flip latitude */
            if (pass == 1) { t0 = T3D_PI - t0; t1 = T3D_PI - t1; }

            float s0 = sinf(t0), c0 = cosf(t0);
            float s1 = sinf(t1), c1 = cosf(t1);

            for (int j = 0; j < lon; j++) {
                float ph0 = 2.f * T3D_PI * (float)j       / lon;
                float ph1 = 2.f * T3D_PI * (float)(j + 1) / lon;
                float cp0 = cosf(ph0), sp0 = sinf(ph0);
                float cp1 = cosf(ph1), sp1 = sinf(ph1);

                T3DVec3 nA = {{ s0*cp0, c0, s0*sp0 }};
                T3DVec3 nB = {{ s0*cp1, c0, s0*sp1 }};
                T3DVec3 nC = {{ s1*cp0, c1, s1*sp0 }};
                T3DVec3 nD = {{ s1*cp1, c1, s1*sp1 }};

                out[idx++] = (T3DVertPacked){
                    .posA  = { (int16_t)(nA.v[0]*radius), (int16_t)(nA.v[1]*radius + y_off), (int16_t)(nA.v[2]*radius) },
                    .normA = t3d_vert_pack_normal(&nA), .rgbaA = rgba, .stA = {0,0},
                    .posB  = { (int16_t)(nB.v[0]*radius), (int16_t)(nB.v[1]*radius + y_off), (int16_t)(nB.v[2]*radius) },
                    .normB = t3d_vert_pack_normal(&nB), .rgbaB = rgba, .stB = {0,0},
                };
                out[idx++] = (T3DVertPacked){
                    .posA  = { (int16_t)(nC.v[0]*radius), (int16_t)(nC.v[1]*radius + y_off), (int16_t)(nC.v[2]*radius) },
                    .normA = t3d_vert_pack_normal(&nC), .rgbaA = rgba, .stA = {0,0},
                    .posB  = { (int16_t)(nD.v[0]*radius), (int16_t)(nD.v[1]*radius + y_off), (int16_t)(nD.v[2]*radius) },
                    .normB = t3d_vert_pack_normal(&nD), .rgbaB = rgba, .stB = {0,0},
                };
            }
        }
    }
}
