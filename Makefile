BUILD_DIR = build

include $(N64_INST)/include/n64.mk
include tiny3d/t3d.mk

N64_ROM_TITLE = "Legend of Blob 64"
N64_ROM_SAVETYPE = eeprom16k

all: legend-of-blob-64.z64

OBJS = \
    $(BUILD_DIR)/src/main.o \
    $(BUILD_DIR)/src/core/loop.o \
    $(BUILD_DIR)/src/audio/synth.o \
    $(BUILD_DIR)/src/input/input.o \
    $(BUILD_DIR)/src/render/render.o \
    $(BUILD_DIR)/src/gen/mesh.o \
    $(BUILD_DIR)/src/entity/blob.o \
    $(BUILD_DIR)/src/world/room.o \
    $(BUILD_DIR)/src/powers/powers.o \
    $(BUILD_DIR)/src/ui/hud.o

legend-of-blob-64.z64: $(BUILD_DIR)/legend-of-blob-64.elf
$(BUILD_DIR)/legend-of-blob-64.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) legend-of-blob-64.z64 legend-of-blob-64.elf.sym

.PHONY: all clean
