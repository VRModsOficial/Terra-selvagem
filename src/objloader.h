#ifndef OBJLOADER_H
#define OBJLOADER_H

/* Carregador de .obj minimalista, sem dependências externas.
 * Suporta o formato estendido "v x y z r g b" (cor por vértice, 0-1)
 * e faces triangulares "f a b c" (índices simples, 1-based, sem /vt/vn).
 * Cada modelo carregado vira uma lista plana de vértices (6 floats cada:
 * x,y,z,r,g,b), já expandida por triângulo — pronta para desenhar com
 * glDrawArrays(GL_TRIANGLES, ...). */

typedef struct {
    float *data;      /* vertCount * 6 floats: x,y,z,r,g,b */
    int vertCount;     /* número de vértices (vertCount/3 triângulos) */
} Model;

/* Carrega um .obj do disco. Devolve 0 em sucesso, -1 em erro.
 * Em caso de erro, imprime uma mensagem em stderr e *out fica zerado. */
int model_load_obj(const char *path, Model *out);

/* Libera a memória do modelo. */
void model_free(Model *m);

#endif
