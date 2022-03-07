#include "math.h"

#include <stdint.h>

#ifndef ENTITY_H
#define ENTITY_H

static const uint8_t ENTITY_PLAYER = 1;
static const uint8_t ENTITY_CHARACTER = 2;

static const uint8_t ENTITY_FLAG_REMOVE = 0x01;

typedef struct entity {
    struct entity *next;
    uint32_t id;
    uint8_t type;
    uint8_t flags;
    vec_2d_t pos;
    void *data;
} entity_t;

typedef struct {
    int size;
    uint8_t flags;
    entity_t *first;
    entity_t *last;
} entity_list_t;

typedef struct {
    int8_t input_x;
    int8_t input_y;
    int8_t input_r;
    float speed;
    vec_2d_t vel;
} player_data_t;

entity_t *entity_list_add(entity_list_t *entities, uint32_t eid, uint8_t type, vec_2d_t pos);
entity_t *entity_list_find(entity_list_t *entities, uint32_t eid);

#endif