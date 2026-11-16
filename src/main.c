#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "objloader.h"
#include "terrain.h"
#include "world.h"
#include "audio.h"
#include "primitives.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------- estado global simples (um jogo single-player, sem frescura) ---------- */
static SDL_Window *g_window = NULL;
static World g_world;
static Model g_treeModel, g_rockModel;
static TerrainMesh g_terrain;
static float g_gameTime = 0.0f;
static const char *SAVE_PATH = "ilha_save.txt";

static int g_sndBird = -1, g_sndGrowl = -1, g_sndWater = -1, g_sndPickup = -1;

static float g_crashX = 6.0f, g_crashZ = 6.0f, g_crashY = 0.0f;

typedef struct { float x, y, z, scale, rotY, phase; } Decor;
#define MAX_DECOR 48
static Decor g_trees[MAX_DECOR]; static int g_treeCount = 0;
static Decor g_rocks[MAX_DECOR]; static int g_rockCount = 0;

typedef struct { int active; float x, y, z; } DistantPlane;
static DistantPlane g_plane = { 0, -90, 55, -10 };

static const char *STORY[8] = {
    "O motor do aviao falha. Pelas janelas so se ve fumaca e, la embaixo, uma ilha coberta de montanhas e floresta densa.",
    "Voce se prepara para o impacto. A queda e violenta -- galhos quebram, metal se retorce, e tudo escurece por um instante.",
    "Voce acorda ferido entre as arvores. A fuselagem esta destruida e uma das asas se soltou, caida a poucos metros dali.",
    "Ao longe, um motor ronca. Outro aviao atravessa o ceu! Voce se levanta e aceha com os dois bracos, gritando por socorro.",
    "O aviao segue reto e desaparece no horizonte. O piloto nao te viu.",
    "Talvez do alto da montanha voce consiga sinalizar o proximo. Voce comeca a subir pelas pedras soltas da encosta.",
    "No meio do caminho, uma pedra cede sob seu pe. Voce escorrega e rola de volta ate a base da montanha, machucado.",
    "Ofegante, voce entende: ninguem vai te achar por acaso. Pressione ESPACO para comecar a sobreviver."
};

/* ---------- matemática de câmera (substitui gluLookAt, sem precisar de -lGLU) ---------- */
static void v_sub(const float a[3], const float b[3], float out[3]) { out[0]=a[0]-b[0]; out[1]=a[1]-b[1]; out[2]=a[2]-b[2]; }
static void v_cross(const float a[3], const float b[3], float out[3]) {
    out[0]=a[1]*b[2]-a[2]*b[1]; out[1]=a[2]*b[0]-a[0]*b[2]; out[2]=a[0]*b[1]-a[1]*b[0];
}
static void v_norm(float v[3]) {
    float len = sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    if (len > 0.00001f) { v[0]/=len; v[1]/=len; v[2]/=len; }
}
static void apply_look_at(const float eye[3], const float center[3], const float up[3]) {
    float f[3]; v_sub(center, eye, f); v_norm(f);
    float s[3]; v_cross(f, up, s); v_norm(s);
    float u[3]; v_cross(s, f, u);
    float m[16] = {
        s[0], u[0], -f[0], 0.0f,
        s[1], u[1], -f[1], 0.0f,
        s[2], u[2], -f[2], 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    glMultMatrixf(m);
    glTranslatef(-eye[0], -eye[1], -eye[2]);
}
static void set_perspective(double fovYdeg, double aspect, double zNear, double zFar) {
    double fH = tan(fovYdeg * M_PI / 360.0) * zNear;
    double fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

/* ---------- desenho ---------- */
static void draw_primitive(const float *posArray, int count, float x, float y, float z,
                            float sx, float sy, float sz, float rotY, float r, float g, float b) {
    glDisableClientState(GL_COLOR_ARRAY);
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotY * 180.0f / (float)M_PI, 0, 1, 0);
    glScalef(sx, sy, sz);
    glVertexPointer(3, GL_FLOAT, 0, posArray);
    glDrawArrays(GL_TRIANGLES, 0, count);
    glPopMatrix();
    glEnableClientState(GL_COLOR_ARRAY);
}
static void draw_colored_model(const Model *m, float x, float y, float z, float scale, float rotY) {
    if (!m->data || m->vertCount == 0) return;
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotY * 180.0f / (float)M_PI, 0, 1, 0);
    glScalef(scale, scale, scale);
    glVertexPointer(3, GL_FLOAT, 6 * sizeof(float), m->data);
    glColorPointer(3, GL_FLOAT, 6 * sizeof(float), m->data + 3);
    glDrawArrays(GL_TRIANGLES, 0, m->vertCount);
    glPopMatrix();
}

static float frand(void) { return (float)rand() / (float)RAND_MAX; }

static void build_decor(void) {
    for (int i = 0; i < 40; i++) {
        float x, z, y; int tries = 0;
        do { x = (frand()-0.5f)*140.0f; z = (frand()-0.5f)*140.0f; y = terrain_height(x,z); tries++; }
        while ((y < 1.5f || y > 15.0f) && tries < 100);
        if (g_treeCount < MAX_DECOR) {
            Decor *d = &g_trees[g_treeCount++];
            d->x=x; d->y=y; d->z=z; d->scale=0.8f+frand()*0.7f; d->rotY=frand()*6.28f; d->phase=frand()*6.28f;
        }
    }
    for (int i = 0; i < 26; i++) {
        float x, z, y; int tries = 0;
        do { x = (frand()-0.5f)*150.0f; z = (frand()-0.5f)*150.0f; y = terrain_height(x,z); tries++; }
        while (y < 0.5f && tries < 100);
        if (g_rockCount < MAX_DECOR) {
            Decor *d = &g_rocks[g_rockCount++];
            d->x=x; d->y=y+0.3f; d->z=z; d->scale=0.4f+frand()*1.0f; d->rotY=frand()*6.28f; d->phase=0;
        }
    }
}

static void sky_color(const World *w, float *r, float *g, float *b) {
    float frac = w->isDay ? (w->timeOfDay/720.0f) : ((w->timeOfDay-720.0f)/720.0f);
    float bright = w->isDay ? (0.22f+0.78f*sinf(frac*(float)M_PI)) : (0.04f+0.10f*sinf(frac*(float)M_PI));
    float nr=0.039f, ng=0.063f, nb=0.149f;
    float dr=0.561f, dg=0.816f, db=1.0f;
    *r = nr + (dr-nr)*bright;
    *g = ng + (dg-ng)*bright;
    *b = nb + (db-nb)*bright;
}

/* câmera durante a cutscene inicial (reaproveita a mesma ideia da versão web) */
static void intro_camera(const World *w, float eye[3], float target[3]) {
    float base = terrain_height(w->player.x, w->player.z);
    float px = w->player.x, pz = w->player.z, eh = 1.7f, t = w->introT;
    if (w->introStep <= 1) {
        float k = 1.0f - t/2.2f; if (k < 0) k = 0;
        eye[0]=px+sinf(t*30.0f)*0.4f*k; eye[1]=base+28.0f*k+eh; eye[2]=pz+cosf(t*24.0f)*0.4f*k;
        target[0]=px; target[1]=base; target[2]=pz;
    } else if (w->introStep >= 2 && w->introStep <= 4) {
        eye[0]=px-3; eye[1]=base+eh; eye[2]=pz-3;
        target[0]=px+2; target[1]=base+1; target[2]=pz;
    } else if (w->introStep == 5) {
        float tt = t/2.5f; if (tt>1) tt=1;
        eye[0]=px+tt*3; eye[1]=base+eh+tt*4; eye[2]=pz+tt*2;
        target[0]=px+6; target[1]=base+8; target[2]=pz+4;
    } else if (w->introStep == 6) {
        float tt = t/1.4f; if (tt>1) tt=1;
        eye[0]=px+3-tt*3+sinf(t*50.0f)*0.3f; eye[1]=base+eh+4-tt*4; eye[2]=pz+2-tt*2;
        target[0]=px; target[1]=base; target[2]=pz;
    } else {
        eye[0]=px; eye[1]=base+eh; eye[2]=pz;
        target[0]=px+sinf(w->player.yaw); target[1]=base+eh; target[2]=pz+cosf(w->player.yaw);
    }
}

static void render_hud(const World *w) {
    int ww, wh; SDL_GetWindowSize(g_window, &ww, &wh);
    glDisable(GL_DEPTH_TEST);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0, ww, wh, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    /* tinta de calor/frio sobre a tela toda */
    if (w->temp > 0.78f || w->temp < 0.22f) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float k = w->temp > 0.78f ? (w->temp-0.78f)/0.22f : (0.22f-w->temp)/0.22f;
        float pulse = 0.5f + 0.5f*sinf(g_gameTime*3.0f);
        if (w->temp > 0.78f) glColor4f(0.9f, 0.3f, 0.1f, 0.10f*k + 0.08f*k*pulse);
        else glColor4f(0.1f, 0.3f, 0.8f, 0.12f*k);
        glBegin(GL_QUADS);
        glVertex2f(0,0); glVertex2f((float)ww,0); glVertex2f((float)ww,(float)wh); glVertex2f(0,(float)wh);
        glEnd();
        glDisable(GL_BLEND);
    }

    /* barrinhas de status, canto superior esquerdo */
    float barW = 200.0f, barH = 10.0f, gap = 16.0f, x0 = 16.0f, y0 = 16.0f;
    float vals[5]; float colR[5], colG[5], colB[5];
    vals[0]=w->health/100.0f; colR[0]=0.44f; colG[0]=0.68f; colB[0]=0.35f;
    vals[1]=w->hunger/100.0f; colR[1]=0.78f; colG[1]=0.60f; colB[1]=0.23f;
    vals[2]=w->thirst/100.0f; colR[2]=0.23f; colG[2]=0.53f; colB[2]=0.78f;
    vals[3]=w->energy/100.0f; colR[3]=0.60f; colG[3]=0.44f; colB[3]=0.82f;
    vals[4]=w->temp;          colR[4]=w->temp>0.5f?0.75f:0.23f; colG[4]=0.4f; colB[4]=w->temp>0.5f?0.2f:0.78f;
    for (int i = 0; i < 5; i++) {
        float y = y0 + i*gap;
        glColor3f(0.1f,0.14f,0.13f);
        glBegin(GL_QUADS); glVertex2f(x0,y); glVertex2f(x0+barW,y); glVertex2f(x0+barW,y+barH); glVertex2f(x0,y+barH); glEnd();
        glColor3f(colR[i],colG[i],colB[i]);
        float fw = barW * (vals[i] < 0 ? 0 : (vals[i] > 1 ? 1 : vals[i]));
        glBegin(GL_QUADS); glVertex2f(x0,y); glVertex2f(x0+fw,y); glVertex2f(x0+fw,y+barH); glVertex2f(x0,y+barH); glEnd();
    }

    /* mira no centro */
    float cx = ww*0.5f, cy = wh*0.5f;
    glColor3f(1,1,1);
    glBegin(GL_QUADS); glVertex2f(cx-2,cy-2); glVertex2f(cx+2,cy-2); glVertex2f(cx+2,cy+2); glVertex2f(cx-2,cy+2); glEnd();

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
}

static void render_frame(World *w) {
    int ww, wh; SDL_GetWindowSize(g_window, &ww, &wh);
    if (ww < 1) ww = 1;
    if (wh < 1) wh = 1;
    glViewport(0, 0, ww, wh);

    float r, g, b; sky_color(w, &r, &g, &b);
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    set_perspective(74.0, (double)ww/(double)wh, 0.1, 400.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float eye[3], target[3], up[3] = {0,1,0};
    if (w->gameState == 0) {
        intro_camera(w, eye, target);
    } else {
        float base = terrain_height(w->player.x, w->player.z);
        eye[0]=w->player.x; eye[1]=base+1.7f; eye[2]=w->player.z;
        float fwd[3] = { sinf(w->player.yaw)*cosf(w->player.pitch), sinf(w->player.pitch), cosf(w->player.yaw)*cosf(w->player.pitch) };
        target[0]=eye[0]+fwd[0]; target[1]=eye[1]+fwd[1]; target[2]=eye[2]+fwd[2];
    }
    apply_look_at(eye, target, up);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    /* terreno */
    glVertexPointer(3, GL_FLOAT, 6*sizeof(float), g_terrain.data);
    glColorPointer(3, GL_FLOAT, 6*sizeof(float), g_terrain.data + 3);
    glDrawArrays(GL_TRIANGLES, 0, g_terrain.vertCount);

    /* cenário: árvores (com balanço) e pedras */
    for (int i = 0; i < g_treeCount; i++) {
        Decor *d = &g_trees[i];
        float sway = sinf(g_gameTime*0.6f + d->phase) * 0.04f;
        draw_colored_model(&g_treeModel, d->x, d->y, d->z, d->scale, d->rotY + sway);
    }
    for (int i = 0; i < g_rockCount; i++) {
        Decor *d = &g_rocks[i];
        draw_colored_model(&g_rockModel, d->x, d->y, d->z, d->scale, d->rotY);
    }

    /* destroços do avião */
    draw_primitive(CUBE_VERTS, CUBE_VERT_COUNT, g_crashX, g_crashY+0.8f, g_crashZ, 2.0f,1.6f,7.0f, 0.4f, 0.73f,0.76f,0.78f);
    draw_primitive(CUBE_VERTS, CUBE_VERT_COUNT, g_crashX+4.0f, g_crashY+0.3f, g_crashZ-2.5f, 6.5f,0.25f,1.6f, 0.9f, 0.56f,0.60f,0.62f);

    /* avião distante, só durante a cutscene */
    if (w->gameState == 0 && g_plane.active) {
        draw_primitive(CUBE_VERTS, CUBE_VERT_COUNT, g_plane.x, g_plane.y, g_plane.z, 1.6f,0.5f,2.2f, 0.0f, 1,1,1);
    }

    if (w->gameState != 0) {
        /* itens */
        for (int i = 0; i < w->itemCount; i++) {
            const Item *it = &w->items[i];
            if (!it->active) continue;
            float gy = terrain_height(it->x, it->z) + 0.3f;
            if (it->active == 2) { /* ovo raro */
                float pulse = 1.0f + 0.15f*sinf(g_gameTime*3.0f);
                draw_primitive(ICO_VERTS, ICO_VERT_COUNT, it->x, gy, it->z, 0.22f*pulse,0.26f*pulse,0.22f*pulse, 0, 0.91f,0.82f,0.48f);
                continue;
            }
            switch (it->type) {
                case ITEM_GRAVETO: draw_primitive(CUBE_VERTS, CUBE_VERT_COUNT, it->x, gy, it->z, 0.07f,0.5f,0.07f, 0, 0.54f,0.42f,0.27f); break;
                case ITEM_PEDRA:   draw_primitive(ICO_VERTS, ICO_VERT_COUNT, it->x, gy, it->z, 0.22f,0.22f,0.22f, 0, 0.55f,0.55f,0.55f); break;
                case ITEM_CIPO:    draw_primitive(CUBE_VERTS, CUBE_VERT_COUNT, it->x, gy, it->z, 0.05f,0.5f,0.05f, 0, 0.29f,0.48f,0.16f); break;
                case ITEM_FRUTA:   draw_primitive(ICO_VERTS, ICO_VERT_COUNT, it->x, gy, it->z, 0.16f,0.16f,0.16f, 0, 0.76f,0.23f,0.27f); break;
                case ITEM_CARNE:   draw_primitive(CUBE_VERTS, CUBE_VERT_COUNT, it->x, gy, it->z, 0.3f,0.15f,0.3f, 0, 0.60f,0.23f,0.17f); break;
                default: break;
            }
        }
        /* animais */
        for (int i = 0; i < w->animalCount; i++) {
            const Animal *a = &w->animals[i];
            if (!a->active) continue;
            float gy = terrain_height(a->x, a->z);
            float bob = sinf(a->animPhase) * 0.05f;
            float waddle = sinf(a->animPhase) * 0.15f;
            draw_primitive(ICO_VERTS, ICO_VERT_COUNT, a->x, gy+0.4f+bob, a->z, 0.5f,0.35f,0.8f, a->dir+waddle, 0.48f,0.35f,0.23f);
            draw_primitive(ICO_VERTS, ICO_VERT_COUNT, a->x+sinf(a->dir)*0.7f, gy+0.55f+bob, a->z+cosf(a->dir)*0.7f, 0.22f,0.22f,0.22f, a->dir, 0.42f,0.29f,0.18f);
        }
        /* predadores */
        for (int i = 0; i < w->predatorCount; i++) {
            const Predator *p = &w->predators[i];
            if (!p->active) continue;
            float gy = terrain_height(p->x, p->z);
            float bob = sinf(g_gameTime*8.0f) * 0.06f;
            draw_primitive(ICO_VERTS, ICO_VERT_COUNT, p->x, gy+0.4f+bob, p->z, 0.55f,0.4f,0.9f, 0, 0.16f,0.14f,0.12f);
        }
        /* carcaças */
        for (int i = 0; i < w->carcassCount; i++) {
            const Carcass *c = &w->carcasses[i];
            if (!c->active || c->eaten) continue;
            float gy = terrain_height(c->x, c->z);
            draw_primitive(CUBE_VERTS, CUBE_VERT_COUNT, c->x, gy+0.1f, c->z, 0.5f,0.2f,0.3f, 0, 0.48f,0.16f,0.13f);
        }
        /* fogueiras, com chama tremeluzindo */
        for (int i = 0; i < w->campfireCount; i++) {
            const Campfire *c = &w->campfires[i];
            if (!c->active) continue;
            float gy = terrain_height(c->x, c->z);
            float flick = 0.85f + 0.3f*sinf(g_gameTime*18.0f + c->x);
            draw_primitive(CUBE_VERTS, CUBE_VERT_COUNT, c->x, gy+0.3f, c->z, 0.3f,0.6f*flick,0.3f, g_gameTime, 0.75f,0.33f,0.12f);
        }
        /* companheiro (pet) */
        if (w->hasPet) {
            float gy = terrain_height(w->petX, w->petZ);
            float bob = sinf(g_gameTime*5.0f) * 0.06f;
            draw_primitive(ICO_VERTS, ICO_VERT_COUNT, w->petX, gy+0.3f+bob, w->petZ, 0.22f,0.22f,0.22f, 0, 1.0f,0.88f,0.54f);
        }
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    if (w->gameState != 0) render_hud(w);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    g_window = SDL_CreateWindow("Ilha: Sobrevivencia",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 800,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!g_window) { fprintf(stderr, "SDL_CreateWindow falhou: %s\n", SDL_GetError()); return 1; }

    SDL_GLContext ctx = SDL_GL_CreateContext(g_window);
    if (!ctx) { fprintf(stderr, "SDL_GL_CreateContext falhou: %s\n", SDL_GetError()); return 1; }
    SDL_GL_SetSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.08f, 0.1f, 1.0f);

    if (model_load_obj("models/palmeira.obj", &g_treeModel) != 0)
        fprintf(stderr, "aviso: sem models/palmeira.obj (rode o jogo a partir da pasta do projeto)\n");
    if (model_load_obj("models/pedra.obj", &g_rockModel) != 0)
        fprintf(stderr, "aviso: sem models/pedra.obj\n");

    terrain_generate(&g_terrain, 180.0f, 90);
    build_decor();

    world_init(&g_world);
    g_crashY = terrain_height(g_crashX, g_crashZ);

    if (audio_init() == 0) {
        g_sndBird = audio_load("sounds/passaro.wav");
        g_sndGrowl = audio_load("sounds/predador.wav");
        g_sndWater = audio_load("sounds/agua.wav");
        g_sndPickup = audio_load("sounds/coleta.wav");
    } else {
        fprintf(stderr, "aviso: audio nao inicializou, o jogo segue sem som\n");
    }

    printf("\n=== ILHA: SOBREVIVENCIA ===\n");
    printf("A historia aparece aqui no terminal; a janela mostra a cena em 3D.\n\n");
    printf("%s\n", STORY[0]);

    int running = 1, mouseCaptured = 0, gameOverPrinted = 0;
    Uint32 lastTicks = SDL_GetTicks();
    float saveTimer = 0.0f;

    while (running) {
        InputState in; memset(&in, 0, sizeof(in));
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (g_world.gameState == 1 && !mouseCaptured) { SDL_SetRelativeMouseMode(SDL_TRUE); mouseCaptured = 1; }
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) {
                    if (mouseCaptured) { SDL_SetRelativeMouseMode(SDL_FALSE); mouseCaptured = 0; }
                }
                if (g_world.gameState == 0) {
                    if (k == SDLK_SPACE || k == SDLK_RETURN) {
                        int prevStep = g_world.introStep;
                        world_advance_intro(&g_world);
                        if (prevStep + 1 == 3) g_plane.active = 1;
                        if (g_world.introStep < 8) printf("%s\n", STORY[g_world.introStep]);
                        if (g_world.gameState == 1) {
                            SDL_SetRelativeMouseMode(SDL_TRUE); mouseCaptured = 1;
                            SDL_GetRelativeMouseState(NULL, NULL);
                            printf("\n-- Comecou! WASD move, mouse olha, E interage, F fogueira,\n");
                            printf("   1-5 fazer itens na mao, O choca ovo raro, F5 salva, F9 carrega. --\n\n");
                        }
                    }
                } else if (g_world.gameState == 1) {
                    if (k == SDLK_e) in.interactPressed = 1;
                    if (k == SDLK_f) in.placeFirePressed = 1;
                    if (k == SDLK_o) in.hatchPressed = 1;
                    if (k >= SDLK_1 && k <= SDLK_5) in.craftHotkey = (int)(k - SDLK_0);
                    if (k == SDLK_F5) { if (world_save(&g_world, SAVE_PATH) == 0) printf("Jogo salvo.\n"); }
                    if (k == SDLK_F9) { if (world_load(&g_world, SAVE_PATH) == 0) printf("Jogo carregado.\n"); }
                } else if (g_world.gameState == 2) {
                    if (k == SDLK_r) {
                        world_init(&g_world);
                        gameOverPrinted = 0;
                        printf("\n=== ILHA: SOBREVIVENCIA ===\n%s\n", STORY[0]);
                    }
                }
            }
        }

        if (g_world.gameState == 1 && mouseCaptured) {
            int mx, my; SDL_GetRelativeMouseState(&mx, &my);
            in.lookDx = -(float)mx * 0.0022f;
            in.lookDy = (float)my * 0.0022f;
        }
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        in.forward = keys[SDL_SCANCODE_W];
        in.back    = keys[SDL_SCANCODE_S];
        in.left    = keys[SDL_SCANCODE_A];
        in.right   = keys[SDL_SCANCODE_D];
        in.sprint  = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.0f; lastTicks = now;
        if (dt > 0.1f) dt = 0.1f;
        g_gameTime += dt;

        if (g_world.gameState == 0) {
            g_world.introT += dt;
            if (g_plane.active) { g_plane.x += dt * 22.0f; if (g_plane.x > 90.0f) g_plane.active = 0; }
        } else if (g_world.gameState == 1) {
            FrameEvents ev;
            world_update(&g_world, dt, &in, &ev);
            if (ev.playBird && g_sndBird >= 0) audio_play(g_sndBird, 0.4f);
            if (ev.playGrowl && g_sndGrowl >= 0) audio_play(g_sndGrowl, 0.5f);
            if (ev.playWater && g_sndWater >= 0) audio_play(g_sndWater, 0.6f);
            if (ev.playPickup && g_sndPickup >= 0) audio_play(g_sndPickup, 0.6f);
            if (ev.message[0]) printf("%s\n", ev.message);

            saveTimer += dt;
            if (saveTimer > 30.0f) { saveTimer = 0; world_save(&g_world, SAVE_PATH); }
        } else if (g_world.gameState == 2) {
            if (!gameOverPrinted) { printf("\nVoce nao resistiu. Pressione R para recomecar.\n"); gameOverPrinted = 1; }
        }

        render_frame(&g_world);
        SDL_GL_SwapWindow(g_window);
    }

    audio_shutdown();
    model_free(&g_treeModel);
    model_free(&g_rockModel);
    terrain_free(&g_terrain);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
