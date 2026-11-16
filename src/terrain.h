#ifndef TERRAIN_H
#define TERRAIN_H

/* Mesmo formato de malha plana dos outros modelos: vertCount vértices,
 * 6 floats cada (x,y,z,r,g,b), prontos para glDrawArrays(GL_TRIANGLES). */
typedef struct {
    float *data;
    int vertCount;
} TerrainMesh;

/* Altura do terreno num ponto (x,z) do mundo — ilha com uma montanha.
 * Usada tanto para gerar a malha quanto para colisão/posição do jogador,
 * dos animais, da água, etc. */
float terrain_height(float x, float z);

/* Gera a malha da ilha dentro de um quadrado de lado `size`, com
 * `segments` divisões por eixo. */
void terrain_generate(TerrainMesh *out, float size, int segments);

void terrain_free(TerrainMesh *out);

#endif
