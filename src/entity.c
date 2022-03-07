#include "entity.h"

#include <stdlib.h>

entity_t *entity_list_add(entity_list_t *entities, uint32_t eid, uint8_t type, vec_2d_t pos) {
    entity_t *entity = (entity_t *)malloc(sizeof(entity_t));
    entity->id = eid;
    entity->type = type;
    entity->flags = 0;
    entity->pos = pos;
    entity->data = NULL;
    entity->next = NULL;

    if (entities->size == 0) {
        entities->first = entity;
        entities->last = entity;
    }
    else {
        entities->last->next = entity;
        entities->last = entity;
    }
    entities->size++;

    return entity;
}

entity_t *entity_list_find(entity_list_t *entities, uint32_t eid) {
    entity_t *entity = entities->first;
    while (entity) {
        if (entity->id == eid) {
            return entity;
        }
        else {
            entity = entity->next;
        }
    }
    return NULL;
}
