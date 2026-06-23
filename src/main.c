#include <libdragon.h>
#include <t3d/t3d.h>
#include "core/loop.h"
#include "audio/synth.h"
#include "input/input.h"
#include "render/render.h"

int main(void) {
    /* Hard constraint #2: 8 MB Expansion Pak required.
     * assert_memory_expanded() shows an error screen and halts if not present. */
    assert_memory_expanded();

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    joypad_init();
    timer_init();

    synth_init();
    input_init();
    t3d_init((T3DInitParams){});
    render_init();

    loop_run();

    return 0;
}
