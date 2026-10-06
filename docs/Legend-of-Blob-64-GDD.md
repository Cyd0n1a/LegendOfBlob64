# Legend of Blob 64 — Game Design & Technical Planning Document

> A physics-driven, top-down action-RPG for the Nintendo 64, in the spirit of *The Legend of Zelda: A Link to the Past*.
> Built with **Libdragon (preview branch)** + **Tiny3D**, in a reproducible **Docker** toolchain.

**Document version:** 0.2 (Pre-Production)
**Primary target:** **Analogue 3D** and other FPGA N64 clones with HDMI output (4K-aware); fully compatible with original N64 hardware that has an Expansion Pak fitted.
**Secondary target:** Stock N64 (NTSC/PAL) **with Expansion Pak**, Ares/Gopher64 emulation for development.
**Memory:** **8 MB required** — the 8 MB Expansion Pak is **mandatory**, not optional (see §0 and §9.7).
**Cartridge target:** 16–32 MB ROM.
**Asset rule:** **All 3D models and all sound effects are procedurally generated in code** — zero pre-authored mesh or audio-sample assets (see §0 and §7–§8).
**Accessories:** **Rumble Pak supported** (real + virtual); Controller Pak / EEPROM for saves (see §9.6).
**Engine stack:** Libdragon preview · Tiny3D · rdpq · libdragon mixer
**Genre:** Action-adventure / puzzle RPG (top-down, "2.5D" rendered)

---

## 0. Hard Constraints (read first)

These three constraints are load-bearing and shape every section below:

1. **Procedural assets only.** Every 3D model (the blob, enemies, bosses, props, environment geometry) is **generated at build-time or runtime from code** — parametric mesh generators, not imported `.glb`/`.fbx` files. Likewise, **every sound effect is synthesized from code** (oscillators, noise, envelopes, simple DSP) rather than streamed from recorded `.wav` samples. This is a deliberate creative + technical constraint: tiny ROM footprint, infinitely tweakable assets, and a coherent "born from pure math/color" aesthetic that fits a blob made of light. *(Music is the one allowed exception — see §8 — though it is also synthesis-driven.)*

2. **Expansion Pak is mandatory.** The game requires 8 MB of RDRAM and checks for it at boot; without it, the player gets a friendly "Expansion Pak required" screen and the game will not proceed. This is acceptable because the **primary target is FPGA clone hardware** (Analogue 3D and similar), where the Expansion Pak is **built in and enabled by default**. Original-hardware owners simply need the physical Pak installed.

3. **HDMI / FPGA-clone first.** The render and audio paths are designed assuming a clean digital HDMI pipeline and cycle-accurate FPGA timing. Nothing in the design relies on analog-video quirks or on emulator-only behavior, so it runs identically on real silicon, FPGA clones, and accurate emulators.

---

## 1. High-Concept

You are **Blob**, the last living spark of color in a world that has been drained to grey. Blob is a squishy, soft-body rainbow creature who survives not by swinging a sword, but by **eating the things that try to eat it** — absorbing the powers of enemies and bosses to fight, traverse, and solve the puzzles locked inside the world's corrupted dungeons.

The pitch in one line:

> *"What if Link could swallow his enemies like Kirby, the world ran on real squash-and-stretch physics, and every power you ate also changed how you rolled, squeezed, and bounced through a Zelda-style dungeon?"*

Three pillars hold the whole design up:

1. **Squish** — Blob is a physics object first and a character second. Movement, combat, and puzzles all read through soft-body deformation, momentum, and mass.
2. **Absorb** — Almost every enemy is also a tool. Defeating something is a question ("do I want its power right now?") rather than just a reward.
3. **Unlock** — Classic Zelda gating: each dungeon hands you a permanent *Core* power that re-opens the overworld and the dungeons you already cleared.

---

## 2. Design Goals & Non-Goals

### 2.1 Goals
- A **12–15 hour** main quest with 6 main dungeons + 1 finale, plus optional content.
- **Readable physics**: the blob should feel alive and squishy without making controls feel slippery or unfair.
- **Two-layer power system**: disposable enemy-copied powers (moment-to-moment) layered over permanent boss-granted Core powers (progression).
- **Runs on a stock N64** at a stable **30 fps** in normal play, degrading gracefully (never below ~20 fps) in heavy boss scenes.
- A **clean, data-driven content pipeline** so designers can author rooms, enemies, and powers without recompiling engine code.

### 2.2 Non-Goals
- No online/multiplayer. No procedural generation of main-quest content.
- No full ragdoll/cloth simulation — soft-body is *stylized and cheap*, tuned for feel, not realism.
- No voice acting; story told through text boxes, pantomime cutscenes, and environmental storytelling.

---

## 3. Story & World

### 3.1 Premise
The world of **Chroma** was held together by the **Prism Heart**, a crystal that radiated all color and life. A grey hunger called the **Drabbening** cracked it into seven **Prism Shards**, which scattered into seven regions. As color bled out of the world, the creatures that remained twisted into colorless **Husks** — and each Shard fell under the control of a corrupted **Guardian**.

Blob is what's left when the Prism Heart shatters: a single drop of every color, alive and curious. Blob's quest is to recover the seven Shards, restore color to each region, and reassemble the Prism Heart before the Drabbening swallows the last of the world.

### 3.2 Tone
Charming, tactile, slightly mischievous. Think *Kirby* warmth crossed with *Zelda* mystery. The emotional through-line is **color returning to a dead world** — each dungeon cleared visibly repaints its region of the overworld.

### 3.3 The Overworld — Chroma Vale
A connected hub-and-spoke overworld in the *Link to the Past* tradition: a central **Vale** that branches into seven biomes, each themed to one color/Shard. The overworld is screen-streamed (Zelda-style room transitions on a grid), not open-world.

| Region | Color | Biome | Dungeon Theme | Core Power Reward |
|--------|-------|-------|---------------|-------------------|
| Emberreach | Red | Volcanic crags | Furnace / heat | **Ember Core** (ignite, melt, burn ropes) |
| Springwood | Orange | Bouncy fungal forest | Spring & momentum | **Spring Core** (charged super-bounce) |
| Voltmarsh | Yellow | Storm wetlands | Circuits & power | **Spark Core** (chain lightning, charge nodes) |
| Thornhollow | Green | Overgrown ruins | Grapple & growth | **Vine Core** (grapple-pull, grow platforms) |
| Tidepool | Blue | Coastal caverns | Water & ice | **Frost Core** (freeze water, douse fire) |
| Gravemont | Indigo | Sunken catacombs | Gravity & mass | **Weight Core** (become dense, smash, sink) |
| Spectergale | Violet | Haunted spires | Phase & light | **Phase Core** (pass ghost-walls, reveal) |
| **The Prism Crown** | White/All | Sky citadel | Finale | — |

---

## 4. Core Mechanics

### 4.1 The Blob as a Physics Object
Blob is simulated as a small **soft body**: a ring of point-masses connected by spring constraints around a central "core" mass, with pressure keeping it round. This is what produces:

- **Squash & stretch** on landing, dashing, and bonking walls.
- **Squeeze** — when pushed against a narrow gap, the blob deforms and can ooze through openings smaller than its rest diameter (puzzle traversal).
- **Momentum** — rolling builds speed; a charged roll becomes an attack. Heavy mass (Weight Core) changes inertia and stopping distance.
- **Splat** — falling from height flattens the blob briefly, a readable "stagger" window.

> **Design rule:** physics serves *feel and readability*, never punishes the player with loss of control. Input always wins over simulation — the sim is a visual + collision flavor layer on top of a deterministic capsule used for "real" gameplay collision.

### 4.2 Control Scheme (N64 controller)

| Input | Action |
|-------|--------|
| **Analog stick** | Move (8-directional feel, full analog under the hood) |
| **A** | Hop / context jump (squash-charge for higher hop) |
| **B** | Use current absorbed power (primary attack/ability) |
| **Z (trigger)** | Absorb: inhale/swallow adjacent stunned enemy or pick up object |
| **R** | Roll / dash (hold to charge a momentum roll) |
| **C-up** | Spit out / discard current power (becomes a projectile) |
| **C-left / C-right** | Cycle stored Core powers |
| **C-down** | Map / context |
| **Start** | Pause / inventory |

**Rumble Pak feedback** (real Pak in the controller's accessory slot, or the FPGA clone's virtual Rumble Pak): used as a *tactile language*, not just generic shake. Short crisp pulses on **absorb/swallow** and **spit**, a rising buzz while **charging a roll or super-bounce**, a heavy thud on **Weight-Core ground-pound and breaking cracked floors**, directional nudges on **bonking walls**, and distinct boss-phase/critical-hit patterns. Rumble is fully **toggleable** in options and auto-disables gracefully if no Pak is detected.

### 4.3 The Absorption System (the heart of combat)
Two tiers of power:

**Tier 1 — Copied Powers (temporary).** Almost every standard enemy carries a power matching its color. The loop:
1. **Stagger** the enemy (roll into it, or hit it with your current power).
2. **Absorb** it (Z) while it's stunned — Blob inhales it and takes on its color + ability.
3. **Use** it (B). Copied powers have a **charge meter** that drains with use; when empty, Blob reverts to neutral (white/grey-tinged) and must find another enemy.
4. Optionally **spit** the power out (C-up) as a one-shot projectile for puzzle/combat utility.

You can only hold **one** copied power at a time — choosing what to eat *is* the combat decision.

**Tier 2 — Core Powers (permanent).** Earned from each dungeon **boss**. Cores never run out and can be freely cycled (C-left/right). They are the Zelda "dungeon items": each Core is the **key** that unlocks traversal in later dungeons and re-opens earlier regions (metroidvania-light backtracking on top of linear gating).

> Copied powers = improvisation and combat texture.
> Core powers = progression and gating.

### 4.4 The Seven Powers (Copied ⇄ Core mapping)

| Color | Copied (enemy) ability | Core (permanent) ability | Puzzle uses |
|-------|------------------------|--------------------------|-------------|
| Red | Fire dash, burning touch | **Ember Core**: aimed flame, sustained burn | Light torches, melt ice, burn rope/vine |
| Orange | Pogo bounce | **Spring Core**: charged super-bounce, wall-kick | Reach high switches, cross gaps, bounce-pads |
| Yellow | Short zap | **Spark Core**: chain lightning, hold-to-charge | Power circuits, stun groups, light dark rooms |
| Green | Vine whip | **Vine Core**: grapple-pull to hooks, grow platforms | Pull objects/self, create bridges, swing |
| Blue | Water splash | **Frost Core**: freeze surfaces, ice bridge | Freeze water into platforms, douse fire, slick floors |
| Indigo | Brief heavy slam | **Weight Core**: become dense, sink, ground-pound | Heavy switches, break cracked floors, sink underwater |
| Violet | Blink | **Phase Core**: pass ghost-walls, reveal hidden | Phase through marked walls, see invisible paths |

Combos are encouraged: e.g. **Frost** an oil slick, then **Ember** is denied — but **Spark** travels along it. Designers author puzzles assuming the player has the right Core and may also be holding a relevant copied power.

### 4.5 Combat Feel
- **Stagger-then-eat** rhythm: most enemies must be destabilized before they can be absorbed, so combat is about creating openings.
- **Roll** is the universal verb — it's traversal, a stagger tool, and a momentum attack.
- **Bosses** are large, multi-phase, and telegraph a specific weakness that maps to either your newest Core or a power you must absorb *from adds spawned in the arena* — teaching the dungeon's power as you fight.

---

## 5. Dungeon Design

### 5.1 Structure
Each dungeon follows the LttP template, adapted to Blob's verbs:

- A **locked, gated layout**: small keys (here: **Color Keys**) open doors; a **map** and **compass** (here: **Prism Lens**) reveal layout and the Shard room.
- A **mid-dungeon power** moment: the dungeon's signature interaction is taught early (via copied power from a themed enemy), then mastered.
- A **boss** that grants the permanent **Core** and the **Prism Shard**.
- **Backtrack hooks**: every dungeon hides 1–2 things only reachable with a *later* Core, rewarding return trips (heart-equivalent upgrades, see §6).

### 5.2 Puzzle Vocabulary
A shared library of interactable archetypes, all data-driven (see §9.3):

- **Colored switches/torches** — only the matching power activates them.
- **Squeeze gaps** — passable only by deforming (sometimes only when *light*, i.e. not holding Weight).
- **Mass plates** — require Weight Core or a pushed heavy object.
- **Conductive rails** — require Spark; route power to doors.
- **Freeze/melt water** — Frost makes platforms, Ember removes them; some rooms flip state.
- **Grapple hooks** — Vine pulls Blob across gaps or pulls objects to Blob.
- **Ghost-walls** — only Phase passes; often hide the optional rewards.

### 5.3 Difficulty Curve
- **D1 Emberreach** introduces one mechanic cleanly (heat/melt) with generous space.
- **D2–D4** layer two mechanics and introduce **combo puzzles** (chain two powers).
- **D5–D6** assume mastery: timed puzzles, multi-room state, mini-boss gauntlets.
- **Finale** remixes all seven powers in a "greatest hits" gauntlet.

---

## 6. Progression & Economy

- **Health:** **Globs** (heart equivalent). Start with 3 Globs; max grows via **Glob Vials** (4 fragments = 1 Glob), scattered and dungeon-rewarded. Cap ~16.
- **Power charge:** copied powers run on a per-power **charge meter**; no global resource to manage.
- **Currency:** **Chroma** (collected drops) spends at Vale shops on consumables (Glob refills, charge boosters) and cosmetic blob "sheens."
- **Collectibles:** **Prism Motes** (think Korok-lite) hidden across the overworld; turning in sets unlocks optional content and a true-ending flourish.
- **No XP/levels** — progression is *capability* (Cores) and *survivability* (Globs), Zelda-style, not stat grinding.

---

## 7. Art Direction

> **All geometry is procedural.** There are no modeled/imported assets. Every mesh is produced by parametric **generator functions** in C that emit Tiny3D-ready vertex/index buffers. The art "assets" of this project are *the generators and their parameter sets*, not files. This both honors the core constraint and yields a unmistakable, math-born look.

### 7.1 Visual Style
- **Rendered 3D, played 2D**: procedurally generated Tiny3D meshes under a fixed **¾ top-down orthographic-ish camera**, recreating the LttP reading angle while getting real lighting, squash-and-stretch, and depth-sorted props. (This is the "2.5D" approach Tiny3D is well suited to.)
- **Color as the central theme**: the world starts desaturated; restoring a Shard floods that region's palette back. The blob itself is the most saturated thing on screen at all times. Because color is driven by **vertex colors / shading parameters in code**, "draining" and "restoring" a region is literally a palette-parameter lerp, not a texture swap.
- **Procedural-by-design aesthetic**: chunky, low-poly, high-charm shapes built from primitives (spheres, capsules, prisms, lathed/extruded profiles), noise-displaced for organic feel, with **vertex lighting** and **procedural/parametric textures** (gradients, checker, noise, dither patterns generated into small RAM textures). Leans into N64 strengths rather than fighting them.

### 7.2 Procedural Generation Approach
A shared **mesh-gen library** provides building blocks used by every entity:
- **Primitive generators:** UV-sphere, icosphere, capsule, box/prism, cylinder, cone, torus, grid/heightfield, lathe-from-profile, extrude-along-path.
- **Modifiers:** noise displacement (value/Perlin/simplex in fixed-point), bevel/smooth, twist, taper, symmetry/mirror, vertex-color ramps.
- **Seeded variation:** each enemy/prop instance takes a seed so a "Husk" type spawns with slight per-instance shape variation from one generator — cheap visual variety, zero extra asset cost.
- **Output:** generators write directly into the **`T3DVertPacked`** interleaved vertex layout Tiny3D expects, plus index lists, so generated meshes feed the normal Tiny3D draw path with no importer.
- **Where generation runs:** static, deterministic meshes (overworld props, enemy base shapes) are generated **once at boot or region-load** into RDRAM pools and cached; the **blob** is regenerated/deformed **per frame** (see §9.4–9.5).

### 7.3 The Blob
- A procedurally generated **deforming icosphere/metaball-ish mesh** whose vertices are driven by the soft-body sim each frame (the sim points *are* the low-frequency control cage; the render mesh is a smoothed skin over them).
- **Current power = current hue** (instant readability of state), set via vertex-color parameters in code. Neutral Blob is a shifting pale rainbow shimmer (animated hue parameter).
- Expressive **eyes** drawn as cheap procedural billboard quads/decals so personality survives heavy deformation.

### 7.4 Environments
- Each region gets a distinct **palette + procedural material set + ambient particle** (embers, spores, sparks, rain, bubbles, dust, fog) — all parameterized, all code-driven.
- Room/dungeon geometry is assembled from generated **modular tile meshes** placed by the data-driven map (see §9.3): the *layout* is authored data, the *geometry* is generated.
- Heavy use of **rdpq** for crisp 2D HUD/text over the 3D scene.

---

## 8. Audio

> **All sound effects are synthesized in code** — no recorded/streamed `.wav` SFX. A small in-engine synth produces every blip, splat, and boom at runtime.

- **SFX synth engine:** a lightweight code synthesizer feeds the libdragon **mixer**. Each sound is a recipe of **oscillators (sine/square/saw/triangle), noise, ADSR envelopes, pitch sweeps, and simple DSP** (one-pole filters, bitcrush, ring-mod) rendered into short buffers on the fly. This gives squishy, wet, bouncy blob foley and **distinct per-color absorb/spit/power-activate cues** (e.g. Ember = filtered noise burst + downward sweep; Spark = ring-modded zap; Frost = bright sine with shimmer) — all parameterized, all tiny in ROM.
- **Parametric variation:** SFX take small random offsets (pitch/decay) per trigger so repeated sounds don't fatigue the ear, at zero asset cost.
- **Music:** kept as the *one* authored-content exception, but still synthesis-driven — compact **tracker/sequence data** (note/pattern data, *not* sampled instruments) played through the same code-synth voices, so the soundtrack is essentially a chiptune engine rather than streamed audio. Each region's theme has a **"drab" and "restored" variant**; restoring the Shard crossfades to the colorful mix.
- **No `audioconv64` sample pipeline:** because there are no recorded samples, the audio build step is just compiling synth recipes + sequence data. Budget mixer voices carefully (they're finite) and reserve a few for music vs. SFX.
- **FPGA/HDMI fit:** target hardware recreates the N64's RSP audio path and outputs clean digital PCM over HDMI, so synthesized audio is reproduced faithfully and identically across real silicon, clones, and accurate emulators.

---

## 9. Technical Architecture

### 9.1 Toolchain Overview
- **Libdragon (preview branch)** is the SDK. The preview branch is where new rendering features live and is **required by Tiny3D**; it tracks ahead of stable and its APIs can change, which we accept and pin via submodule commit.
- **Tiny3D** is a third-party, from-scratch N64 3D microcode + C API that interoperates directly with **rdpq**. Although Tiny3D ships a GLTF importer for modeled assets, **we don't use it** — we feed Tiny3D's interleaved `T3DVertPacked` vertex buffers directly from our **procedural mesh generators** (§7.2). We still use Tiny3D's matrix stack, lighting, and skinning math.
- **Docker** provides the prebuilt GCC 14 / Newlib C11 toolchain so every developer builds identically, via the `libdragon` CLI wrapper (no need to learn Docker internals).
- **Emulator:** **Ares** (with "Homebrew mode" on) for development debugging; **Gopher64** for performance sanity. **Real hardware** via EverDrive64 / SummerCart64 + **UNFLoader** for on-console logging.

> ⚠️ **Preview-branch risk:** because we depend on the preview branch (and Tiny3D, which targets it), upstream API breakage is expected. Mitigation in §12.

### 9.2 Repository Layout
```
legend-of-blob-64/
├─ .gitmodules                 # pins libdragon (preview) + tiny3d commits
├─ libdragon/                  # submodule @ preview (pinned SHA)
├─ tiny3d/                     # submodule (pinned SHA), built against preview
├─ Makefile                    # includes $(T3D_INST)/t3d.mk
├─ Dockerfile.dev              # optional: extends libdragon image w/ our tools
├─ tools/                      # build-time codegen (synth recipe compiler, map packer)
├─ assets/                     # AUTHORED DATA ONLY — no mesh/audio binaries
│  ├─ maps/      (room/dungeon layouts: JSON/Tiled → packed binary)
│  ├─ meshgen/   (generator parameter sets / seeds for procedural meshes)
│  ├─ synth/     (SFX synth recipes + music sequence/pattern data)
│  └─ fonts/     (.ttf → rdpq_text font64 — only non-generated asset type)
├─ src/
│  ├─ main.c                   # boot, Expansion-Pak check, display/rdpq/t3d init, loop
│  ├─ core/                    # fixed-timestep loop, scene mgr, save (Controller Pak/EEPROM)
│  ├─ gen/                     # PROCEDURAL: mesh generators (primitives+modifiers), tex gen
│  ├─ audio/                   # PROCEDURAL: code synth (oscillators/env/DSP) + sequencer
│  ├─ physics/                 # soft-body sim + swept-capsule world collision
│  ├─ entity/                  # ECS-lite: blob, enemies, props
│  ├─ powers/                  # power definitions + dispatch (copied + cores)
│  ├─ world/                   # room streaming, transitions, overworld grid
│  ├─ render/                  # t3d scene setup, camera, blob deformation draw
│  ├─ input/                   # controller + Rumble Pak feedback driver
│  ├─ ui/                      # HUD, menus, text boxes (rdpq + rdpq_text)
│  └─ dungeons/                # per-dungeon logic as DSO overlays (see §9.6)
└─ build/                      # ROM output (legend-of-blob-64.z64)
```

### 9.3 Data-Driven Content
Engine code is generic; designers author **data**:
- **Rooms/maps** authored in Tiled (or a custom editor), exported to a compact binary blob: tile/prop placements, collision, spawn tables, switch wiring, transition links.
- **Enemies** described by data records (mesh-**generator id + params/seed**, stats, AI behavior id, copied-power id, synth-SFX id, drop table).
- **Powers** described by records: color, copied-ability id, core-ability id, charge cost, particle-FX + synth-SFX ids.
- This keeps the build fast (most edits don't recompile C) and lets puzzle wiring be authored visually.

Example power record (illustrative C):
```c
typedef enum { PWR_NONE, PWR_EMBER, PWR_SPRING, PWR_SPARK,
               PWR_VINE,  PWR_FROST,  PWR_WEIGHT, PWR_PHASE } power_id_t;

typedef struct {
    power_id_t  id;
    color_rgb_t hue;          // drives blob tint + UI
    uint16_t    charge_max;   // copied-power budget (0 = N/A for pure cores)
    uint16_t    charge_cost;  // per activation
    fx_id_t     activate_fx;  // particle/sfx handle
    void      (*on_use)(entity_t *blob, const input_t *in);
    bool      (*solves)(const interactable_t *obj); // puzzle predicate
} power_def_t;
```

### 9.4 Physics Model
- **Blob soft-body:** ~12–20 perimeter point-masses + center, integrated with **Verlet** + distance/pressure constraints, a handful of constraint iterations per frame. Cheap, stable, deterministic enough.
- **World collision:** gameplay collision uses a **swept capsule/AABB** against tile/heightfield data — *not* the soft body — so movement stays fair and predictable. The soft body reads the capsule and "drapes" over it visually + handles squeeze checks.
- **Fixed timestep:** simulation runs at a fixed dt (e.g. 60 Hz logic) decoupled from a 30 Hz render where needed, to keep physics deterministic regardless of frame dips.
- **Mass states:** Weight Core swaps the blob's mass/inertia parameters and its collision behavior (sink, smash cracked floors).

### 9.5 Rendering Pipeline (Tiny3D)
- Init display + `rdpq` + Tiny3D each frame; set up a fixed ¾ ortho-style camera matrix.
- **Procedural geometry feeds the pipeline directly:** static room/prop/enemy meshes are generated once into cached `T3DVertPacked` buffers; the **blob** mesh is regenerated/deformed per frame from the soft-body cage.
- Draw the current room's cached geometry + props; draw dynamic entities (depth-sorted).
- Blob is drawn by uploading **per-frame deformed vertices** (Tiny3D DMAs verts/matrices to the RSP each draw, so updating them every frame is the intended usage).
- HUD/text composited afterward with **rdpq** / `rdpq_text`.
- Target **30 fps**; budget triangle counts per room and cap simultaneous particles/enemies. On FPGA clones, an optional internal-resolution bump is feasible since the 8 MB budget is guaranteed (output scaling to 4K is handled by the console, not the game).

### 9.6 Code Overlays (DSO)
- Per-dungeon logic and big one-off boss code live in **DSO overlays** (libdragon's `dlopen`/`dlsym` dynamic libraries), loaded on dungeon entry and unloaded on exit. Even with a guaranteed 8 MB, overlays keep the resident footprint small and load times low, leaving generous headroom for cached procedural geometry and audio voices.

### 9.7 Memory Budget (8 MB — Expansion Pak mandatory)
The game targets the full **8 MB** of RDRAM and checks for it at boot (see §0). On FPGA clones this RAM is always present; on original hardware the physical Expansion Pak is required. The extra 4 MB is spent on cached **procedural geometry/texture pools** and **audio voice buffers** rather than streamed assets.

| Bucket | Budget (approx) | Notes |
|--------|-----------------|-------|
| Code + engine statics | ~0.7 MB | core engine + mesh-gen + synth code resident |
| Current dungeon overlay | ~0.4 MB | DSO, swapped per dungeon |
| Cached procedural geometry | ~2.0 MB | generated room/prop/enemy `T3DVertPacked` pools |
| Procedural textures (gen'd) | ~0.6 MB | small RAM textures from code (gradients/noise/dither) |
| Entities + physics + scratch | ~0.6 MB | pools, no malloc in hot loop |
| Audio (synth voices + seq data) | ~0.6 MB | code-synth buffers; no sample banks |
| Framebuffers + Z + RSP | ~0.9 MB | depth + RSP/RDP working set |
| Headroom | remainder (~2 MB) | safety, generous on 8 MB |

> **Boot check:** `main.c` queries available RDRAM; if < 8 MB, show an "Expansion Pak required" screen and halt. This is intentional given the FPGA-clone-first target, where the Pak is built in and on by default.

### 9.8 Accessories & Save System
- **Rumble Pak:** detected via the controller accessory slot (or the clone's virtual Rumble Pak). Effects are authored as a small library of pulse patterns (§4.2). Fully optional/toggleable; absence is handled silently.
- **Controller Pak / EEPROM saves:** save data (Shards, Cores, Globs, motes, position, options) is compact. Primary path is **EEPROM (4/16 Kbit)** on the cartridge for friction-free saving across all targets; an optional **Controller Pak** path is supported for players who prefer it (FPGA clones expose a virtual Controller Pak). Three save slots, each checksummed with write-verify to guard against corruption on flashcarts.
- **Note on simultaneous accessories:** original controllers have a single accessory slot, so Rumble and a physical Controller Pak can't be used at once. We default saves to cartridge EEPROM specifically so the slot stays free for the Rumble Pak.

---

## 10. Build & Run (Docker workflow)

**Prerequisites:** Docker (recent version) and Git. The `libdragon` CLI is distributed via npm or as a standalone binary and wraps the container.

```bash
# 1. Install the libdragon CLI (npm route)
npm install -g libdragon@latest

# 2. Create the project on the PREVIEW toolchain (Tiny3D requires it)
libdragon init --branch preview
#    -> sets up ./libdragon submodule + pulls the preview Docker image

# 3. Add Tiny3D as a submodule and build it against preview
git submodule add https://github.com/HailToDodongo/tiny3d.git tiny3d
libdragon exec ./tiny3d/build.sh        # build lib + tools + examples inside container

# 4. In our Makefile, include Tiny3D's make fragment:
#       include $(T3D_INST)/t3d.mk
#    (links the Tiny3D ucode + C API into the ROM)

# 5. Build the ROM
libdragon make            # -> build/legend-of-blob-64.z64

# 6. Run it
#    Ares (Homebrew mode ON) for debugging, or copy to EverDrive64/SC64 via UNFLoader
#    Hardware/clone verification: SummerCart64 on real N64 (+Expansion Pak) AND on Analogue 3D
```

> **No asset-conversion stage.** Because all meshes and SFX are generated from code, the build has **no model importer and no `audioconv64` sample step** — `tools/` only compiles synth recipes, mesh-gen parameter sets, and packs map data. Build times stay fast and the ROM stays small.

**Reproducibility:** the libdragon and Tiny3D **submodule SHAs are pinned** in `.gitmodules`; CI builds the exact same image, so "works on my machine" is the same as "works in CI," "works on cart," and "works on the Analogue 3D."

> **Clone test pass:** add a recurring CI/manual checklist item to boot the latest ROM on **Analogue 3D** (current 3DOS firmware) verifying the Expansion-Pak boot check, Rumble Pak behavior (real + virtual), HDMI output at 1080p/4K modes, and audio parity.

---

## 11. Production Roadmap

### Phase 0 — Tech Slice (4–6 wks)
- Docker/libdragon-preview/Tiny3D pipeline building a ROM in CI.
- **Expansion-Pak boot check** + "Pak required" screen; verify on real N64 and Analogue 3D.
- First-pass **procedural mesh generator** (primitives + noise displacement) feeding Tiny3D, and a minimal **code-synth** producing a few SFX.
- Blob soft-body + capsule collision in one test room. Camera. rdpq HUD stub. Basic **Rumble Pak** pulse on bonk.
- **Exit criteria:** you can roll a procedurally-generated, squishy blob around a room at 30 fps on real hardware *and* on an Analogue 3D, with synthesized SFX and rumble.

### Phase 1 — Vertical Slice (8–10 wks)
- One full region (Emberreach) + one full dungeon (D1) end-to-end.
- Absorption loop (copied + 1 Core), 3 enemy types, 1 boss, save/load, synthesized SFX + first synth music track.
- Data-driven room + power pipeline; Tiled→binary maps; mesh-gen **parameter authoring** workflow + in-engine live tweak tool.
- **Exit criteria:** a stranger can play D1 start-to-finish and "get it," with all geometry/SFX generated from code.

### Phase 2 — Content Production (16–24 wks)
- Remaining regions/dungeons D2–D6, all seven powers, all bosses.
- DSO overlay system per dungeon; Expansion Pak quality path.
- Full audio set (drab/restored variants), particle systems, polish passes.

### Phase 3 — Finale & Systems Polish (6–8 wks)
- The Prism Crown finale, true-ending content, motes economy, shops.
- Difficulty tuning, accessibility options, PAL/NTSC, performance hardening.

### Phase 4 — Cert/Release (4 wks)
- Hardware soak testing across flashcarts, memory-leak/regression sweeps, localization text pass, ROM packaging.

---

## 12. Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Preview-branch / Tiny3D API breakage | Build breaks mid-dev | Pin submodule SHAs; update on a schedule behind a test branch; keep a thin engine-side abstraction over Tiny3D calls |
| Soft-body physics feels slippery/unfair | Hurts core feel | Decouple gameplay collision (capsule) from visual soft body; input overrides sim; heavy playtest tuning |
| Performance dips in boss rooms | Below 30 fps | Entity/particle caps, triangle budgets per room, profile on hardware early, LOD on generated meshes |
| Procedural meshes/SFX look or sound "samey" or sterile | Weak art/audio identity | Invest early in the generator + synth toolset; seeded per-instance variation; treat parameter authoring as the real art job; build in-engine live tweak tools |
| Procedural generation cost (CPU/RSP) at load or per-frame | Hitches, frame dips | Generate static meshes once into cached pools (not per-frame); only the blob regenerates per frame; profile generator hot paths early |
| Expansion Pak requirement excludes stock-4MB N64 owners | Smaller original-hardware audience | Accepted: FPGA-clone-first target where the Pak is built in; clear "Expansion Pak required" boot screen; document the requirement on the box/readme |
| Behavior differs between real N64, FPGA clones, and emulator | Hard-to-repro bugs | Test matrix across Ares + real N64 (+Pak) + Analogue 3D every milestone; avoid analog/emulator-only assumptions |
| Scope creep (7 powers × N puzzles) | Schedule slip | Data-driven puzzle archetypes reused across dungeons; lock the power list after Vertical Slice |
| Save corruption on real carts | Player loss | EEPROM checksums, triple-slot, write-verify, test across flashcart models |

---

## 13. Open Questions (to resolve in Pre-Production)
1. **Copied-power persistence** — pure charge-meter (Kirby-style loss on empty) vs. a small "last power" memory buffer? Leaning charge-meter for tension.
2. **Camera** — strict ortho top-down vs. slight perspective tilt? Prototype both; pick on readability.
3. **Backtracking weight** — how much optional content is gated behind later Cores before it feels like padding?
4. **Logic tick rate** — 60 Hz logic / 30 Hz render everywhere, or 30/30 in heavy scenes? Decide after hardware profiling.
5. **Map format** — adopt Tiled + custom exporter, or build a bespoke room editor for switch-wiring ergonomics?

---

## 14. Appendix — Reference Stack
- **Libdragon** (SDK, preview branch): modern GCC 14 / Newlib, C11, rdpq, mixer, DSO overlays, rdpq_text. (We deliberately skip the sample/FMV asset paths — everything is generated.)
- **Tiny3D** (HailToDodongo): from-scratch N64 3D ucode + C API, direct rdpq interop, matrix/lighting/skinning math, `T3DVertPacked` vertex layout; requires the preview branch. **Its GLTF importer is intentionally unused** — we feed procedurally generated vertex buffers instead.
- **libdragon-docker** (`libdragon` CLI): containerized, per-project toolchain; `--branch preview` selects the preview toolchain.
- **Target hardware:** **Analogue 3D** and other FPGA N64 clones (HDMI, 4K-aware, Expansion-Pak-built-in, real-accessory + virtual Rumble/Controller Pak support) as primary; original N64 **with Expansion Pak** as secondary.
- **Dev/test:** Ares (Homebrew mode) + Gopher64; EverDrive64 / SummerCart64 / 64drive; UNFLoader for USB console logging; SummerCart64 for booting on real N64 and on Analogue 3D.

*Names, numbers, and budgets in this document are pre-production targets and will be revised against hardware profiling during the Tech Slice.*
