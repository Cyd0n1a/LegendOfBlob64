#include "room.h"

void room_init(room_t *r) {
    r->enemies[0] = (enemy_t){ .x =  70.f, .z =  60.f,  .color = 0xFF3010FF, .alive = true, .power = PWR_EMBER  };
    r->enemies[1] = (enemy_t){ .x = -65.f, .z =  70.f,  .color = 0x40FF60FF, .alive = true, .power = PWR_VINE   };
    r->enemies[2] = (enemy_t){ .x =  55.f, .z = -30.f,  .color = 0x40C0FFFF, .alive = true, .power = PWR_FROST  };
    r->enemies[3] = (enemy_t){ .x = -55.f, .z = -30.f,  .color = 0xFFEE20FF, .alive = true, .power = PWR_SPARK  };
}
