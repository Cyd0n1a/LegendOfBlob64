#ifndef RENDER_RENDER_H
#define RENDER_RENDER_H

#include <libdragon.h>
#include "../entity/blob.h"
#include "../world/room.h"

void render_init(void);
void render_frame(surface_t *disp, const blob_state_t *blob,
                  const room_t *room, float time);

#endif
