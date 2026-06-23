# Legend of Blob 64

A physics-driven, top-down action-RPG for the Nintendo 64 (Zelda: LttP style).
Squishy rainbow blob; absorbs enemy/boss powers for combat + dungeon puzzles.
Stack: **Libdragon (preview branch)** + **Tiny3D** + **rdpq** + libdragon mixer, in C.

Full design + technical spec (read when you need design context):
@docs/Legend-of-Blob-64-GDD.md

---

## Non-negotiable constraints

These are load-bearing. Violating any of them is a bug, not a style choice.

1. **Procedural assets only.** Every 3D mesh and every sound effect is generated in
   code — parametric mesh generators emitting Tiny3D `T3DVertPacked` buffers, and a
   code synth (oscillators/noise/envelope/DSP) for SFX. **Never** add a model importer,
   `.glb`/`.gltf`/`.fbx`/`.obj` loader, recorded `.wav`/sample audio, or `audioconv64`
   sample step. Music is sequence/pattern data played through the same code synth — no
   sampled instrument banks. Fonts (`.ttf` → `rdpq_text`) are the only allowed asset file.
2. **8 MB / Expansion Pak is mandatory.** `main.c` checks available RDRAM at boot; if
   < 8 MB, show the "Expansion Pak required" screen and halt. Never silently degrade to 4 MB.
3. **Libdragon preview branch is required** (Tiny3D depends on it). Pin libdragon and
   tiny3d submodules to exact SHAs in `.gitmodules`. Don't bump them casually — preview
   APIs break; upgrade deliberately on a branch and re-test.
4. **Target = FPGA clones first** (Analogue 3D et al.) then real N64 + Pak. Don't rely on
   analog-video quirks or emulator-only behavior.

> These are guidance, not enforced config. The procedural-only rule and the boot check
> should also be backed by a CI check / PreToolUse hook — don't rely on instructions alone.

---

## Build & run

Uses the `libdragon` Docker CLI — run build/exec commands through it, never assume a
host toolchain.

```bash
libdragon make                 # build -> build/legend-of-blob-64.z64
libdragon make clean
libdragon exec <cmd>           # run a command inside the toolchain container
libdragon exec ./tiny3d/build.sh   # (re)build Tiny3D against the pinned preview SHA
```

- First-time setup: `libdragon init --branch preview`, then add the tiny3d submodule.
- Makefile must `include $(T3D_INST)/t3d.mk`.
- Test target: **Ares** (Homebrew mode ON) for debugging; verify milestones on **real N64
  + Expansion Pak** and on **Analogue 3D** (SummerCart64). UNFLoader for on-console logs.

---

## Project layout

```
src/gen/       PROCEDURAL meshes (primitives + modifiers) + texture generation
src/audio/     PROCEDURAL code synth (osc/noise/env/DSP) + music sequencer
src/physics/   soft-body sim + swept-capsule world collision
src/entity/    ECS-lite: blob, enemies, props
src/powers/    power defs + dispatch (copied + Core)
src/world/     room streaming, transitions, overworld grid
src/render/    Tiny3D scene setup, camera, blob deform draw
src/input/     controller + Rumble Pak feedback
src/ui/        HUD, menus, text (rdpq + rdpq_text)
src/dungeons/  per-dungeon + boss logic as DSO overlays
src/core/      fixed-timestep loop, scene mgr, save (EEPROM-first)
assets/        AUTHORED DATA ONLY — maps, mesh-gen params, synth recipes, fonts
```

---

## Architecture invariants

- **Two collision bodies.** Gameplay uses a deterministic **swept capsule/AABB** vs.
  tiles/heightfield. The soft body is **visual + squeeze-checks only** — never let the
  soft body drive movement/collision. Input always overrides the sim; controls must stay
  predictable.
- **Fixed-timestep logic** decoupled from render. Keep the sim deterministic; don't tie
  physics to frame time.
- **No `malloc` in the hot loop.** Pre-allocate pools at boot/room-load. Generate static
  meshes once into cached pools; only the **blob** regenerates per frame.
- **Per-dungeon code is a DSO overlay** (`dlopen`/`dlsym`), loaded on entry, unloaded on exit.
- **Data-driven content.** Rooms/enemies/powers are data records; engine code stays
  generic. Adding content shouldn't require engine edits.
- **Power system is two-tier:** temporary *copied* powers (charge meter, one at a time) vs.
  permanent *Core* powers (boss-granted, gate progression). Don't conflate them.
- **Saves default to cartridge EEPROM** so the controller's single accessory slot stays
  free for the Rumble Pak. Checksum + write-verify all saves.

---

## Code conventions

- **C (C11)**, libdragon style. Fixed-point or `float` per libdragon/Tiny3D math helpers —
  match the surrounding code; don't introduce a second math convention.
- Naming: `snake_case` functions/vars, `UPPER_SNAKE_CASE` constants/macros,
  `power_id_t`-style `_t` suffix on typedefs.
- Keep RSP/RDP and DMA usage going through Tiny3D / rdpq APIs; don't hand-roll ucode.
- Comments explain **why**, not what.

---

## Never do

- Don't add asset importers, sampled audio, or any non-generated mesh/sound. (Constraint #1)
- Don't make the blob's soft body authoritative for gameplay collision.
- Don't allocate in the per-frame path.
- Don't bump libdragon/tiny3d SHAs without a deliberate, tested upgrade.
- Don't add a 4 MB fallback or weaken the Expansion-Pak boot check.
- Don't assume host tools — go through `libdragon exec`.

---

## Verifying a change

1. `libdragon make` builds clean (no new warnings).
2. Boots in Ares (Homebrew mode) with no RSP/RDP validation errors.
3. Holds the 30 fps target in the affected scene; profile if geometry/particles grew.
4. For asset-touching work, confirm it's still 100% generated (no new files in `assets/`
   except maps/params/recipes/fonts).
5. Hardware/clone smoke test at milestone boundaries (Pak check, rumble, HDMI, audio parity).

When you discover project-specific facts (a flaky step, a Tiny3D gotcha), let auto-memory
keep them — don't expand this file with one-off discoveries. Keep it short and high-signal.
