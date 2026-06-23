#ifndef INPUT_INPUT_H
#define INPUT_INPUT_H

#include <stdbool.h>

typedef struct {
    float move_x;
    float move_y;
    bool  btn_hop;
    bool  btn_power;
    bool  btn_absorb;
    bool  btn_roll;
    bool  btn_spit;
    bool  btn_map;
    bool  btn_start;
} input_state_t;

void                  input_init(void);
void                  input_poll(void);
const input_state_t  *input_get(void);
void                  input_rumble(bool on);

#endif
