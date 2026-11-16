#ifndef WORLD_H
#define WORLD_H

#define MAX_ITEMS 80
#define MAX_ANIMALS 10
#define MAX_PREDATORS 6
#define MAX_CARCASSES 10
#define MAX_CAMPFIRES 20
#define RECIPE_COUNT 5

typedef enum { ITEM_GRAVETO, ITEM_PEDRA, ITEM_CIPO, ITEM_FRUTA, ITEM_CARNE, ITEM_TYPE_COUNT } ItemType;
extern const char *g_item_names[ITEM_TYPE_COUNT];
extern const char *g_recipe_names[RECIPE_COUNT];

typedef struct { int active; ItemType type; float x, z; } Item;
typedef struct { int active; float x, z, dir, animPhase; float nextTurn; } Animal;
typedef struct { int active, eaten; float x, z; } Carcass;
typedef struct { int active; float x, z; int carcassIdx; } Predator;
typedef struct { int active; float x, z; } Campfire;

typedef struct {
    float x, z, yaw, pitch;
} Player;

/* Estado de input que o main.c (SDL) preenche a cada frame. */
typedef struct {
    int forward, back, left, right, sprint;
    float lookDx, lookDy;       /* delta do mouse, em radianos já escalados */
    int interactPressed;        /* tecla E, borda de subida */
    int placeFirePressed;       /* tecla F, borda de subida */
    int craftHotkey;            /* 1..RECIPE_COUNT, ou 0 se nenhuma */
    int hatchPressed;           /* tecla O, chocar o ovo raro */
} InputState;

/* O que aconteceu neste frame — o main.c usa isso pra tocar som e
 * imprimir mensagens, sem o world.c precisar saber de SDL. */
typedef struct {
    int playBird, playGrowl, playWater, playPickup;
    char message[160];
} FrameEvents;

typedef struct {
    int gameState; /* 0=intro, 1=jogando, 2=fim de jogo */
    float introT;
    int introStep;

    Player player;
    float health, hunger, thirst, energy, temp;
    float timeOfDay; int dayCount; int isDay;

    int inventory[ITEM_TYPE_COUNT];
    int hasMachado, hasLanca, hasTocha, hasCantil;
    int fogueiraCount;
    int hasEgg, hasPet;
    float petX, petZ;

    Item items[MAX_ITEMS]; int itemCount;
    Animal animals[MAX_ANIMALS]; int animalCount;
    Carcass carcasses[MAX_CARCASSES]; int carcassCount;
    Predator predators[MAX_PREDATORS]; int predatorCount;
    Campfire campfires[MAX_CAMPFIRES]; int campfireCount;
} World;

float terrain_height(float x, float z); /* de terrain.c, reaproveitada aqui */

void world_init(World *w);
void world_advance_intro(World *w); /* chamada ao apertar o botão/tecla de continuar */
void world_update(World *w, float dt, const InputState *in, FrameEvents *ev);
void world_try_craft(World *w, int recipeNumber1to5, FrameEvents *ev);
void world_try_hatch(World *w, FrameEvents *ev);

int world_save(const World *w, const char *path);
int world_load(World *w, const char *path);

#endif
