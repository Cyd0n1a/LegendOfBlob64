![Logo1](https://github.com/Cyd0n1a/LegendOfBlob64/blob/master/n64brew.webp?raw=true) <br>

# Legend of Blob 64

A physics-driven, top-down action-RPG for Nintendo 64 (Zelda: LttP style). Control a squishy rainbow blob that absorbs enemy and boss powers for combat and dungeon puzzles. This repository contains the engine, procedural asset generators, and game code; the full design & technical specification is in Legend-of-Blob-64-GDD.md.

Status
- Prototype / work-in-progress. Target platforms: FPGA N64 clones (Analogue 3D family) and real N64 hardware with Expansion Pak.
- Build system: libdragon (preview branch) + Tiny3D + rdpq.

Non-negotiable constraints (read me first)
- Procedural assets only: ALL meshes and sound effects are generated in code. No model importers, no sampled audio, no recorded WAVs. Fonts (.ttf) are the only permitted external assets.
- Expansion Pak required: This project requires >= 8 MB RDRAM. main.c performs a runtime check and will halt with an "Expansion Pak required" message if < 8 MB is present.
- libdragon preview branch required: libdragon and Tiny3D are pinned to exact SHAs in .gitmodules. Do NOT bump these SHAs casually.
- Target hardware path: FPGA clones (first) → real N64 + Expansion Pak. Do not rely on emulator-specific behavior.

Quick start (from a fresh clone)
1. Initialize libdragon toolchain (first-time only)
   ```
   libdragon init --branch preview
   git submodule update --init --recursive
   ```

2. Build the ROM (uses the libdragon Docker CLI)
   ```
   libdragon make
   # produced ROM: build/legend-of-blob-64.z64
   ```

3. Rebuild Tiny3D if you change the submodule or need to recompile:
   ```
   libdragon exec ./tiny3d/build.sh
   ```

4. Run for testing:
   - Use the provided Ares test target (Homebrew mode ON) for debugging; prefer real hardware verification on FPGA clones and N64 with Expansion Pak.
   - To run inside the libdragon container:
     ```
     libdragon exec ./build/legend-of-blob-64.z64
     ```

Project layout (important directories)
```
src/
  audio/    # procedural synth (oscillators/noise/envelopes/DSP) + sequencer
  core/     # fixed-timestep loop, scene manager, save (EEPROM-first)
  dungeons/ # per-dungeon code as DSO overlays
  entity/   # ECS-lite: blob, enemies, props
  gen/      # procedural mesh + texture generators (parametric)
  input/    # controller handling + Rumble Pak
  physics/  # deterministic swept-capsule world collision + soft-body visuals
  powers/   # power definitions (copied vs Core)
  render/   # Tiny3D scene setup, camera, blob draw
  ui/       # HUD, menus, text (rdpq + rdpq_text)
  world/    # room streaming, overworld grid
assets/     # authored config: mesh-gen params, synth recipes, fonts
```

How it fits together
- Runtime flow: boot -> Expansion Pak / RDRAM check -> initialize Tiny3D/rdpq -> load core scene manager -> deterministic fixed-timestep updates on physics & ECS -> render each frame with Tiny3D. Dungeons are compiled/loaded as dynamic overlays (DSOs) to limit memory footprint.

Development notes & rules
- No dynamic allocation in the hot path: pools are preallocated at boot or room load.
- Soft body is visual-only; gameplay collision uses the swept capsule + AABB deterministic system.
- DSO overlays: per-dungeon logic is loaded/unloaded to keep memory small.
- All asset generation code lives in src/gen and src/audio. Add new shapes or SFX by editing those generators, not by importing assets.
- Do not bump libdragon/tiny3d SHAs without testing on a branch.

Contributing
- Follow C11 and existing naming conventions (snake_case variables/functions, UPPER_SNAKE_CASE constants).
- Keep comments explaining *why*, not *what*.
- Add tests / CI hooks for non-negotiable rules (e.g., verify main.c detects <8MB RDRAM and fails boot).
- If you add content, prefer data-driven records under assets/ — engine code should stay generic.

Troubleshooting
- If build fails: ensure libdragon is initialized with `--branch preview` and submodules are updated.
- To verify pinned SHAs: check `.gitmodules` for the submodule SHAs and confirm you are on the pinned commit before building.
- If you see an Expansion Pak error at boot, confirm your target has >= 8MB RDRAM (Expansion Pak installed) — this is intentional.

Design doc
- Full design & technical spec: Legend-of-Blob-64-GDD.md (read when you need design context)

License
- TODO: add LICENSE (recommended: permissive open-source license if you intend to accept contributions, or mark proprietary).

Maintainer / Contact
- Repo owner: Cyd0n1a
- For design questions, see Legend-of-Blob-64-GDD.md.

Acknowledgements
- Uses libdragon (preview) and Tiny3D; see .gitmodules for pinned SHAs.
