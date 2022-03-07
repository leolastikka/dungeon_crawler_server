#include "game.h"
#include "entity.h"
#include "message.h"
#include "shared.h"
#include "world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct game_time {
    int fps;
    time_t start_time;
    time_t cur_time;
    time_t last_time; // last cycle time
    time_t last_frame_time;
    time_t next_frame_time;
    time_t delta_time; // delta time per cycle
    time_t frame_delta_time; // delta time since last frame
    float frame_delta_float;
} game_time_t;

void init_time(game_time_t *time);
void init_world(world_t *world, entity_list_t *entities);

void update_time(game_time_t *time);
void update_entity(world_t *world, entity_list_t *entities, game_time_t *time, entity_t *entity);

void *game_thread(void *arg) {
    game_shared_t *shared = (game_shared_t*)arg;
    msg_queue_t mq_game = {0, NULL, NULL, NULL};

    game_time_t time;
    time.fps = shared->game_fps;
    init_time(&time);

    entity_list_t entities = {0, 0, NULL, NULL};
    uint32_t current_entity_id = 1;

    world_t world;
    init_world(&world, &entities);

    printf("Game thread running\n");
    while (1) {
        // update time
        update_time(&time);

        // fetch incoming messages
        msg_queue_fetch_safe(&mq_game, shared->mq_in);
        if (mq_game.size != 0) {
            msg_t *msg = msg_queue_pop_first(&mq_game);
            while (msg) {
                if (msg->type == MSG_CLIENT_INPUT) {
                    entity_t *player = msg->client->entity; // unsafe client access?
                    player_data_t *player_data = (player_data_t *)player->data;
                    int8_t *data = (int8_t *)msg->data;
                    player_data->input_x = data[0];
                    player_data->input_y = data[1];
                    player_data->input_r = data[2];
                    // update_entity(&world, &entities, &time, player); // update position immediately?
                }
                else if (msg->type == MSG_SERVER_CLIENT) {
                    // add client entity to entities
                    entity_t *player = entity_list_add(
                        &entities,
                        current_entity_id++,
                        ENTITY_PLAYER,
                        world.start_pos
                    );

                    player_data_t *player_data = (player_data_t *)malloc(sizeof(player_data_t));
                    player_data->input_x = 0;
                    player_data->input_y = 0;
                    player_data->input_r = 0;
                    vec_2d_t vel = {0.0f, 0.0f};
                    player_data->vel = vel;
                    player_data->speed = 1.0f;
                    player->data = player_data;

                    msg->client->entity = player; // unsafe client access?
                    uint32_t *msg_data = msg->data;
                    msg_data[1] = player->id; // [cid, eid]

                    // broadcast added entity
                    msg_t *e_msg = (msg_t *)malloc(sizeof(msg_t));
                    e_msg->type = MSG_SERVER_ADD;
                    e_msg->flags = MSG_FLAG_BROADCAST;
                    e_msg->client = NULL;
                    e_msg->next = NULL;

                    char *data = (char *)malloc(13);
                    data[0] = ENTITY_CHARACTER; // entity type
                    memcpy(&data[1], &player->id, 4); // eid
                    memcpy(&data[5], &player->pos.x, 4); // pos x
                    memcpy(&data[9], &player->pos.y, 4); // pos y
                    e_msg->data = data;

                    msg_queue_push_last_safe(shared->mq_out, e_msg);

                    // push same message to mq_out, don't free it here
                    msg_queue_push_last_safe(shared->mq_out, msg);
                    msg = msg->next;
                    continue;
                }
                else if (msg->type == MSG_SERVER_REMOVE) {
                    entity_t *entity = (entity_t *)msg->data;
                    entity->flags |= ENTITY_FLAG_REMOVE;
                    entities.flags |= ENTITY_FLAG_REMOVE;
                    msg->data = NULL; // dont free client here
                }
                else {
                    printf("Game unimplemented msg %d\n", msg->type);
                    exit(EXIT_FAILURE);
                }

                msg_t *msg_next = msg->next;
                msg_free(msg);
                msg = msg_next;
            }
        }

        // handle game logic at fixed framerate
        if (time.cur_time >= time.next_frame_time) {
            // update frame time
            time.frame_delta_time = time.cur_time - time.last_frame_time;
            time.frame_delta_float = (float)(time.frame_delta_time / CLOCKS_PER_SEC);

            // float total = (float)(time.cur_time - time.start_time) / CLOCKS_PER_SEC;
            // printf("total time %.3f\n", total);

            // update all entites
            entity_t *entity = entities.first;
            while (entity) {
                if (entity->type == ENTITY_PLAYER) {
                    // update velocity
                    player_data_t *data = (player_data_t *)entity->data;
                    data->vel.x = data->input_x * data->speed;
                    data->vel.y = data->input_y * data->speed;

                    // update position
                    vec_2d_t move = data->vel;
                    move = vec_2d_mult(move, time.frame_delta_float);
                    entity->pos = vec_2d_add(entity->pos, move);

                    // broadcast entity state
                    msg_t *msg = (msg_t *)malloc(sizeof(msg_t));
                    msg->type = MSG_SERVER_MOVE;
                    msg->flags = MSG_FLAG_BROADCAST;
                    msg->client = NULL;
                    msg->next = NULL;

                    char *msg_data = (char *)malloc(20);
                    memcpy(&msg_data[0], &entity->id, 4); // eid
                    memcpy(&msg_data[4], &entity->pos.x, 4); // pos x
                    memcpy(&msg_data[8], &entity->pos.x, 4); // pos y
                    memcpy(&msg_data[12], &data->vel.x, 4); // vel x
                    memcpy(&msg_data[16], &data->vel.y, 4); // vel y
                    msg->data = msg_data;

                    msg_queue_push_last_safe(shared->mq_out, msg);
                }

                entity = entity->next;
            }

            // check entity list flags
            if (entities.flags) {
                if (entities.flags & ENTITY_FLAG_REMOVE) {
                    entity_t *entity = entities.first;
                    entity_t *prev = NULL;
                    while (entity) {
                        if (entity->flags & ENTITY_FLAG_REMOVE) {
                            entity_t *next = entity->next;

                            if (entities.size == 1) {
                                entities.first = NULL;
                                entities.last = NULL;
                            }
                            else {
                                if (prev) { // not first
                                    prev->next = entity->next;

                                    if (entity == entities.last) { // last
                                        entities.last = prev;
                                    }
                                }
                                else { // first
                                    entities.first = entity->next;
                                }
                            }

                            // broadcast entity remove
                            msg_t *r_msg = (msg_t *)malloc(sizeof(msg_t));
                            r_msg->type = MSG_SERVER_REMOVE;
                            r_msg->flags = MSG_FLAG_BROADCAST;
                            r_msg->client = NULL;
                            r_msg->next = NULL;
                            uint32_t *data = (uint32_t *)malloc(sizeof(uint32_t));
                            *data = entity->id;
                            r_msg->data = data;
                            msg_queue_push_last_safe(shared->mq_out, r_msg);

                            if (entity->data) {
                                free(entity->data);
                            }
                            free(entity);

                            entities.size--;
                            entity = next;
                        }
                        else {
                            prev = entity;
                            entity = entity->next;
                        }
                    }

                    entities.flags &= ~ENTITY_FLAG_REMOVE;
                }
            }

            // update frame time
            time_t time_spent = time.cur_time - time.last_frame_time; 
            time.last_frame_time = time.cur_time;
            time.next_frame_time = time.cur_time + (double)(1.0 / time.fps) * CLOCKS_PER_SEC - time_spent;
        }
    }
}

void init_time(game_time_t *time) {
    time->start_time = clock();
    time->cur_time = time->start_time;
    time->last_time = time->start_time;
    time->last_frame_time = time->start_time;
    time->next_frame_time = time->start_time;
    time->delta_time = 0.0f;
    time->frame_delta_time = 0.0f;
}

void init_world(world_t *world, entity_list_t *entities) {
    vec_2d_t start_pos = {0,0};
    world->start_pos = start_pos;
}

void update_time(game_time_t *time) {
    time->cur_time = clock();
    time->delta_time = time->cur_time - time->last_time;
    time->last_time = time->cur_time;
}

void update_entity(world_t *world, entity_list_t *entities, game_time_t *time, entity_t *entity) {
    // handle changed input
    // send update to everyone
}
