#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

const char *g_item_names[ITEM_TYPE_COUNT] = { "Graveto", "Pedra", "Cipo", "Fruta", "Carne" };
const char *g_recipe_names[RECIPE_COUNT] = { "Machado de Pedra", "Lanca", "Tocha", "Cantil de Pedra", "Fogueira" };

static const int RECIPE_NEED[RECIPE_COUNT][ITEM_TYPE_COUNT] = {
    /*            Graveto Pedra Cipo Fruta Carne */
    /* Machado */ { 2,     1,    0,   0,    0 },
    /* Lanca   */ { 2,     0,    1,   0,    0 },
    /* Tocha   */ { 1,     0,    1,   0,    0 },
    /* Cantil  */ { 0,     1,    1,   0,    0 },
    /* Fogueira*/ { 3,     2,    0,   0,    0 },
};

#define DAY_LEN 720.0f
#define NIGHT_LEN 720.0f
#define CYCLE (DAY_LEN + NIGHT_LEN)

static float frand(void) { return (float)rand() / (float)RAND_MAX; }

static void valid_spot(float *ox, float *oz, float *oy) {
    float x, z, y;
    int tries = 0;
    do {
        x = (frand() - 0.5f) * 150.0f;
        z = (frand() - 0.5f) * 150.0f;
        y = terrain_height(x, z);
        tries++;
    } while ((y < 1.2f || y > 19.0f) && tries < 200);
    *ox = x; *oz = z; *oy = y;
}

static void spawn_item(World *w, ItemType t, float x, float z) {
    if (w->itemCount >= MAX_ITEMS) return;
    Item *it = &w->items[w->itemCount++];
    it->active = 1; it->type = t; it->x = x; it->z = z;
}

void world_init(World *w) {
    memset(w, 0, sizeof(World));
    w->gameState = 0; /* intro */
    w->introStep = 0; w->introT = 0;

    w->player.x = 9; w->player.z = 10; w->player.yaw = 0.2f; w->player.pitch = 0;

    w->health = 100; w->hunger = 100; w->thirst = 100; w->energy = 100; w->temp = 0.55f;
    w->timeOfDay = 6 * 60; w->dayCount = 1; w->isDay = 1;

    for (int i = 0; i < 55; i++) {
        float x, z, y; valid_spot(&x, &z, &y);
        ItemType t = (ItemType)(rand() % 4); /* Graveto, Pedra, Cipo, Fruta (sem Carne) */
        spawn_item(w, t, x, z);
    }
    /* ovo raro: sempre existe exatamente um, escondido em algum canto da ilha */
    {
        float x, z, y; valid_spot(&x, &z, &y);
        w->items[w->itemCount].active = 2; /* marca especial: 2 = ovo */
        w->items[w->itemCount].type = ITEM_FRUTA; /* tipo ignorado quando active==2 */
        w->items[w->itemCount].x = x; w->items[w->itemCount].z = z;
        w->itemCount++;
    }

    for (int i = 0; i < 6; i++) {
        float x, z, y; valid_spot(&x, &z, &y);
        Animal *a = &w->animals[w->animalCount++];
        a->active = 1; a->x = x; a->z = z;
        a->dir = frand() * 6.2831853f;
        a->nextTurn = 2.0f + frand() * 3.0f;
        a->animPhase = frand() * 6.28f;
    }
}

void world_advance_intro(World *w) {
    w->introStep++;
    w->introT = 0;
    if (w->introStep >= 8) w->gameState = 1; /* começa a jogar */
}

static int near_water(const World *w) {
    float d = sqrtf(w->player.x * w->player.x + w->player.z * w->player.z);
    return terrain_height(w->player.x, w->player.z) < 1.5f && d > 4.0f;
}

static Item *nearest_item(World *w, float maxDist) {
    Item *best = NULL; float bd = maxDist;
    for (int i = 0; i < w->itemCount; i++) {
        if (!w->items[i].active) continue;
        float dx = w->items[i].x - w->player.x, dz = w->items[i].z - w->player.z;
        float d = sqrtf(dx * dx + dz * dz);
        if (d < bd) { bd = d; best = &w->items[i]; }
    }
    return best;
}

static Animal *nearest_animal(World *w, float maxDist) {
    Animal *best = NULL; float bd = maxDist;
    for (int i = 0; i < w->animalCount; i++) {
        if (!w->animals[i].active) continue;
        float dx = w->animals[i].x - w->player.x, dz = w->animals[i].z - w->player.z;
        float d = sqrtf(dx * dx + dz * dz);
        if (d < bd) { bd = d; best = &w->animals[i]; }
    }
    return best;
}

static void do_interact(World *w, FrameEvents *ev) {
    Item *it = nearest_item(w, 2.4f);
    if (it) {
        if (it->active == 2) {
            w->hasEgg = 1;
            snprintf(ev->message, sizeof(ev->message), "Voce encontrou um ovo raro!");
        } else {
            w->inventory[it->type]++;
            snprintf(ev->message, sizeof(ev->message), "%s coletado.", g_item_names[it->type]);
        }
        it->active = 0;
        ev->playPickup = 1;
        return;
    }
    Animal *an = nearest_animal(w, 2.6f);
    if (an && w->hasLanca) {
        an->active = 0;
        if (w->carcassCount < MAX_CARCASSES) {
            Carcass *c = &w->carcasses[w->carcassCount++];
            c->active = 1; c->eaten = 0; c->x = an->x; c->z = an->z;
        }
        spawn_item(w, ITEM_CARNE, an->x + 0.3f, an->z);
        snprintf(ev->message, sizeof(ev->message), "Voce cacou um animal. Carcacas atraem predadores.");
        return;
    }
    if (near_water(w)) {
        w->thirst = w->thirst + 28.0f; if (w->thirst > 100) w->thirst = 100;
        ev->playWater = 1;
        snprintf(ev->message, sizeof(ev->message), "Voce bebeu agua.");
        return;
    }
}

void world_try_craft(World *w, int n, FrameEvents *ev) {
    if (n < 1 || n > RECIPE_COUNT) return;
    int idx = n - 1;
    if (idx == 0 && w->hasMachado) return;
    if (idx == 1 && w->hasLanca) return;
    if (idx == 2 && w->hasTocha) return;
    if (idx == 3 && w->hasCantil) return;
    for (int t = 0; t < ITEM_TYPE_COUNT; t++)
        if (w->inventory[t] < RECIPE_NEED[idx][t]) return;
    for (int t = 0; t < ITEM_TYPE_COUNT; t++)
        w->inventory[t] -= RECIPE_NEED[idx][t];
    if (idx == 0) w->hasMachado = 1;
    else if (idx == 1) w->hasLanca = 1;
    else if (idx == 2) w->hasTocha = 1;
    else if (idx == 3) w->hasCantil = 1;
    else if (idx == 4) w->fogueiraCount++;
    snprintf(ev->message, sizeof(ev->message), "%s criado.", g_recipe_names[idx]);
}

void world_try_hatch(World *w, FrameEvents *ev) {
    if (!w->hasEgg) return;
    w->hasEgg = 0; w->hasPet = 1;
    w->petX = w->player.x; w->petZ = w->player.z;
    snprintf(ev->message, sizeof(ev->message), "O ovo choca! Um companheiro agora te segue.");
}

static void place_campfire(World *w) {
    if (w->fogueiraCount <= 0 || w->campfireCount >= MAX_CAMPFIRES) return;
    w->fogueiraCount--;
    Campfire *c = &w->campfires[w->campfireCount++];
    c->active = 1; c->x = w->player.x; c->z = w->player.z;
}

static void update_player(World *w, float dt, const InputState *in) {
    w->player.yaw += in->lookDx;
    w->player.pitch -= in->lookDy;
    if (w->player.pitch > 1.3f) w->player.pitch = 1.3f;
    if (w->player.pitch < -1.3f) w->player.pitch = -1.3f;

    float coldSlow = (w->temp < 0.22f) ? 0.65f : 1.0f;
    float sprint = (in->sprint && w->energy > 4) ? 1.6f : 1.0f;
    float speed = 5.0f * coldSlow * sprint;

    float dx = 0, dz = 0;
    float sy = sinf(w->player.yaw), cy = cosf(w->player.yaw);
    if (in->forward) { dx += sy; dz += cy; }
    if (in->back)    { dx -= sy; dz -= cy; }
    if (in->left)    { dx -= cy; dz += sy; }
    if (in->right)   { dx += cy; dz -= sy; }
    float len = sqrtf(dx * dx + dz * dz);
    if (len > 0.0001f) {
        float nx = w->player.x + dx / len * speed * dt;
        float nz = w->player.z + dz / len * speed * dt;
        if (terrain_height(nx, nz) > 0.3f) { w->player.x = nx; w->player.z = nz; }
    }

    if (w->hasPet) {
        float px = w->player.x - sy * 1.8f, pz = w->player.z - cy * 1.8f;
        w->petX += (px - w->petX) * fminf(1.0f, dt * 2.0f);
        w->petZ += (pz - w->petZ) * fminf(1.0f, dt * 2.0f);
    }
}

static void update_time(World *w, float dt) {
    w->timeOfDay += dt;
    if (w->timeOfDay >= CYCLE) { w->timeOfDay -= CYCLE; w->dayCount++; }
    w->isDay = w->timeOfDay < DAY_LEN;
}

static void update_stats(World *w, float dt, const InputState *in) {
    int moving = in->forward || in->back || in->left || in->right;
    float petBonus = w->hasPet ? 0.85f : 1.0f;
    w->hunger -= dt * 0.11f * petBonus; if (w->hunger < 0) w->hunger = 0;
    w->thirst -= dt * (w->hasCantil ? 0.10f : 0.17f); if (w->thirst < 0) w->thirst = 0;
    w->energy -= dt * (moving ? 0.16f : -0.04f);
    if (w->energy > 100) w->energy = 100;
    if (w->energy < 0) w->energy = 0;

    float alt = (terrain_height(w->player.x, w->player.z) - 2.0f) / 22.0f;
    if (alt < 0) alt = 0;
    if (alt > 1) alt = 1;
    int nearFire = 0;
    for (int i = 0; i < w->campfireCount; i++) {
        if (!w->campfires[i].active) continue;
        float ddx = w->campfires[i].x - w->player.x, ddz = w->campfires[i].z - w->player.z;
        if (sqrtf(ddx * ddx + ddz * ddz) < 6.0f) { nearFire = 1; break; }
    }
    float target = 0.5f + (w->isDay ? 0.2f : -0.28f) - alt * 0.26f + (nearFire ? 0.35f : 0.0f) + (w->hasTocha ? 0.08f : 0.0f);
    if (target < 0) target = 0;
    if (target > 1) target = 1;
    w->temp += (target - w->temp) * fminf(1.0f, dt * 0.12f);

    if (w->hunger <= 0 || w->thirst <= 0) w->health -= dt * 1.4f;
    if (w->temp < 0.12f || w->temp > 0.92f) w->health -= dt * 1.1f;
    if (w->health < 0) w->health = 0;
    if (w->health <= 0) w->gameState = 2;
}

static void update_creatures(World *w, float dt, FrameEvents *ev) {
    for (int i = 0; i < w->animalCount; i++) {
        Animal *a = &w->animals[i];
        if (!a->active) continue;
        a->animPhase += dt * 4.0f;
        a->nextTurn -= dt;
        if (a->nextTurn <= 0) { a->dir += (frand() - 0.5f) * 2.0f; a->nextTurn = 2.0f + frand() * 3.0f; }
        float nx = a->x + sinf(a->dir) * 0.6f * dt, nz = a->z + cosf(a->dir) * 0.6f * dt;
        float ny = terrain_height(nx, nz);
        if (ny > 1.0f && ny < 19.0f) { a->x = nx; a->z = nz; } else { a->dir += 3.14159f; }
    }
    for (int i = 0; i < w->predatorCount; i++) {
        Predator *p = &w->predators[i];
        if (!p->active) continue;
        Carcass *c = &w->carcasses[p->carcassIdx];
        if (c->eaten) { p->active = 0; continue; }
        float dx = c->x - p->x, dz = c->z - p->z, d = sqrtf(dx * dx + dz * dz);
        if (d > 0.6f) { p->x += dx / d * 1.6f * dt; p->z += dz / d * 1.6f * dt; }
        else {
            c->eaten = 1; p->active = 0;
            float ddx = p->x - w->player.x, ddz = p->z - w->player.z;
            if (sqrtf(ddx * ddx + ddz * ddz) < 30.0f)
                snprintf(ev->message, sizeof(ev->message), "Um predador encontrou a carcaca...");
        }
    }
    /* chance de um predador nascer para uma carcaça ainda não comida e sem perseguidor */
    for (int i = 0; i < w->carcassCount; i++) {
        Carcass *c = &w->carcasses[i];
        if (!c->active || c->eaten) continue;
        int hasHunter = 0;
        for (int p = 0; p < w->predatorCount; p++)
            if (w->predators[p].active && w->predators[p].carcassIdx == i) hasHunter = 1;
        if (!hasHunter && frand() < dt * 0.02f && w->predatorCount < MAX_PREDATORS) {
            float x, z, y; valid_spot(&x, &z, &y);
            Predator *p = &w->predators[w->predatorCount++];
            p->active = 1; p->x = x; p->z = z; p->carcassIdx = i;
            ev->playGrowl = 1;
        }
    }
    if (frand() < dt * 0.02f && !w->isDay) ev->playGrowl = 1;
    if (frand() < dt * 0.03f && w->isDay) ev->playBird = 1;
}

void world_update(World *w, float dt, const InputState *in, FrameEvents *ev) {
    memset(ev, 0, sizeof(FrameEvents));
    if (w->gameState != 1) return;

    update_player(w, dt, in);
    update_time(w, dt);
    update_stats(w, dt, in);
    update_creatures(w, dt, ev);

    if (in->interactPressed) do_interact(w, ev);
    if (in->placeFirePressed) place_campfire(w);
    if (in->craftHotkey) world_try_craft(w, in->craftHotkey, ev);
    if (in->hatchPressed) world_try_hatch(w, ev);
}

int world_save(const World *w, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "x %f\nz %f\nyaw %f\n", w->player.x, w->player.z, w->player.yaw);
    fprintf(f, "health %f\nhunger %f\nthirst %f\nenergy %f\ntemp %f\n", w->health, w->hunger, w->thirst, w->energy, w->temp);
    fprintf(f, "timeOfDay %f\ndayCount %d\n", w->timeOfDay, w->dayCount);
    for (int i = 0; i < ITEM_TYPE_COUNT; i++) fprintf(f, "inv_%s %d\n", g_item_names[i], w->inventory[i]);
    fprintf(f, "machado %d\nlanca %d\ntocha %d\ncantil %d\n", w->hasMachado, w->hasLanca, w->hasTocha, w->hasCantil);
    fprintf(f, "fogueiraCount %d\nhasEgg %d\nhasPet %d\npetX %f\npetZ %f\n", w->fogueiraCount, w->hasEgg, w->hasPet, w->petX, w->petZ);
    fprintf(f, "campfireCount %d\n", w->campfireCount);
    for (int i = 0; i < w->campfireCount; i++)
        fprintf(f, "campfire %f %f\n", w->campfires[i].x, w->campfires[i].z);
    fclose(f);
    return 0;
}

int world_load(World *w, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char key[64]; 
    w->campfireCount = 0;
    while (fscanf(f, "%63s", key) == 1) {
        if (strcmp(key, "x") == 0) fscanf(f, "%f", &w->player.x);
        else if (strcmp(key, "z") == 0) fscanf(f, "%f", &w->player.z);
        else if (strcmp(key, "yaw") == 0) fscanf(f, "%f", &w->player.yaw);
        else if (strcmp(key, "health") == 0) fscanf(f, "%f", &w->health);
        else if (strcmp(key, "hunger") == 0) fscanf(f, "%f", &w->hunger);
        else if (strcmp(key, "thirst") == 0) fscanf(f, "%f", &w->thirst);
        else if (strcmp(key, "energy") == 0) fscanf(f, "%f", &w->energy);
        else if (strcmp(key, "temp") == 0) fscanf(f, "%f", &w->temp);
        else if (strcmp(key, "timeOfDay") == 0) fscanf(f, "%f", &w->timeOfDay);
        else if (strcmp(key, "dayCount") == 0) fscanf(f, "%d", &w->dayCount);
        else if (strcmp(key, "machado") == 0) fscanf(f, "%d", &w->hasMachado);
        else if (strcmp(key, "lanca") == 0) fscanf(f, "%d", &w->hasLanca);
        else if (strcmp(key, "tocha") == 0) fscanf(f, "%d", &w->hasTocha);
        else if (strcmp(key, "cantil") == 0) fscanf(f, "%d", &w->hasCantil);
        else if (strcmp(key, "fogueiraCount") == 0) fscanf(f, "%d", &w->fogueiraCount);
        else if (strcmp(key, "hasEgg") == 0) fscanf(f, "%d", &w->hasEgg);
        else if (strcmp(key, "hasPet") == 0) fscanf(f, "%d", &w->hasPet);
        else if (strcmp(key, "petX") == 0) fscanf(f, "%f", &w->petX);
        else if (strcmp(key, "petZ") == 0) fscanf(f, "%f", &w->petZ);
        else if (strcmp(key, "campfireCount") == 0) { int n; fscanf(f, "%d", &n); }
        else if (strcmp(key, "campfire") == 0) {
            float cx, cz; fscanf(f, "%f %f", &cx, &cz);
            if (w->campfireCount < MAX_CAMPFIRES) {
                w->campfires[w->campfireCount].active = 1;
                w->campfires[w->campfireCount].x = cx;
                w->campfires[w->campfireCount].z = cz;
                w->campfireCount++;
            }
        } else {
            int found = 0;
            for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
                char full[64]; snprintf(full, sizeof(full), "inv_%s", g_item_names[i]);
                if (strcmp(key, full) == 0) { fscanf(f, "%d", &w->inventory[i]); found = 1; break; }
            }
            if (!found) { float junk; fscanf(f, "%f", &junk); }
        }
    }
    fclose(f);
    w->gameState = 1;
    return 0;
}
