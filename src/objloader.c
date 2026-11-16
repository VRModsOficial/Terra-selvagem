#include "objloader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { float x, y, z, r, g, b; } RawVert;

int model_load_obj(const char *path, Model *out) {
    out->data = NULL;
    out->vertCount = 0;

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[objloader] não consegui abrir %s\n", path);
        return -1;
    }

    int rawCap = 64, rawCount = 0;
    RawVert *raw = (RawVert *)malloc(sizeof(RawVert) * rawCap);

    int outCap = 64, outCount = 0; /* em floats-de-vértice, cada unidade = 6 floats */
    float *outData = (float *)malloc(sizeof(float) * 6 * outCap);

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == 'v' && line[1] == ' ') {
            RawVert v; v.r = 0.6f; v.g = 0.6f; v.b = 0.6f;
            int n = sscanf(line + 2, "%f %f %f %f %f %f",
                           &v.x, &v.y, &v.z, &v.r, &v.g, &v.b);
            if (n >= 3) {
                if (rawCount >= rawCap) {
                    rawCap *= 2;
                    raw = (RawVert *)realloc(raw, sizeof(RawVert) * rawCap);
                }
                raw[rawCount++] = v;
            }
        } else if (line[0] == 'f' && line[1] == ' ') {
            int a, b, c;
            if (sscanf(line + 2, "%d %d %d", &a, &b, &c) == 3) {
                int idx[3] = { a - 1, b - 1, c - 1 };
                for (int i = 0; i < 3; i++) {
                    if (idx[i] < 0 || idx[i] >= rawCount) continue;
                    if (outCount >= outCap) {
                        outCap *= 2;
                        outData = (float *)realloc(outData, sizeof(float) * 6 * outCap);
                    }
                    RawVert *v = &raw[idx[i]];
                    float *dst = outData + outCount * 6;
                    dst[0] = v->x; dst[1] = v->y; dst[2] = v->z;
                    dst[3] = v->r; dst[4] = v->g; dst[5] = v->b;
                    outCount++;
                }
            }
        }
    }
    fclose(f);
    free(raw);

    out->data = outData;
    out->vertCount = outCount;
    return 0;
}

void model_free(Model *m) {
    if (m->data) free(m->data);
    m->data = NULL;
    m->vertCount = 0;
}
