#include "terrain.h"
#include <stdlib.h>
#include <math.h>

float terrain_height(float x, float z) {
    float d = sqrtf(x * x + z * z);
    float y = 16.0f - d * 0.22f;   /* ilha com praia de verdade: cruza 0 perto de d~72 */
    if (y < 0.0f) y = 0.0f;

    float mx = x - 15.0f, mz = z - 10.0f;
    float dm = sqrtf(mx * mx + mz * mz);
    float mountain = 30.0f - dm * 0.55f;
    if (mountain > 0.0f) y += mountain;

    y += sinf(x * 0.22f) * cosf(z * 0.22f) * 1.1f;
    return y;
}

static void color_for_height(float y, float *r, float *g, float *b) {
    if (y < 0.6f) { *r = 0.847f; *g = 0.769f; *b = 0.541f; }       /* areia */
    else if (y < 9.0f) { *r = 0.247f; *g = 0.420f; *b = 0.196f; }  /* grama */
    else if (y < 22.0f) { *r = 0.431f; *g = 0.388f; *b = 0.345f; } /* rocha */
    else { *r = 0.949f; *g = 0.965f; *b = 0.973f; }                /* neve */
}

void terrain_generate(TerrainMesh *out, float size, int segments) {
    int rows = segments + 1;
    float half = size * 0.5f;
    float step = size / (float)segments;

    /* grade de posições/cores */
    float *px = (float *)malloc(sizeof(float) * rows * rows);
    float *py = (float *)malloc(sizeof(float) * rows * rows);
    float *pz = (float *)malloc(sizeof(float) * rows * rows);
    float *pr = (float *)malloc(sizeof(float) * rows * rows);
    float *pg = (float *)malloc(sizeof(float) * rows * rows);
    float *pb = (float *)malloc(sizeof(float) * rows * rows);

    for (int j = 0; j < rows; j++) {
        for (int i = 0; i < rows; i++) {
            float x = -half + i * step;
            float z = -half + j * step;
            float y = terrain_height(x, z);
            int idx = j * rows + i;
            px[idx] = x; py[idx] = y; pz[idx] = z;
            color_for_height(y, &pr[idx], &pg[idx], &pb[idx]);
        }
    }

    int cells = segments * segments;
    int vertCount = cells * 6; /* 2 triângulos * 3 vértices */
    float *data = (float *)malloc(sizeof(float) * 6 * vertCount);
    int o = 0;

    for (int j = 0; j < segments; j++) {
        for (int i = 0; i < segments; i++) {
            int a = j * rows + i;
            int b = j * rows + (i + 1);
            int c = (j + 1) * rows + (i + 1);
            int d = (j + 1) * rows + i;
            int tri[6] = { a, b, c, a, c, d };
            for (int t = 0; t < 6; t++) {
                int v = tri[t];
                data[o++] = px[v]; data[o++] = py[v]; data[o++] = pz[v];
                data[o++] = pr[v]; data[o++] = pg[v]; data[o++] = pb[v];
            }
        }
    }

    free(px); free(py); free(pz); free(pr); free(pg); free(pb);
    out->data = data;
    out->vertCount = vertCount;
}

void terrain_free(TerrainMesh *out) {
    if (out->data) free(out->data);
    out->data = NULL;
    out->vertCount = 0;
}
