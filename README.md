# GlitchCraft: GameCube Edition

An original, early-stage voxel sandbox written in C++ with a C-compatible game
API and PowerPC assembly math kernels. Desktop builds use a custom software 3D
voxel renderer in an X11 window; the GameCube target uses a native GX hardware
renderer and GameCube controller input through devkitPro/libogc.

## Build and run

Requirements: CMake 3.16+, a C11 compiler, and a C++17 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/glitchcraft
```

Run the game in an interactive terminal with a desktop display available for
the 3D window. Without an X11 display, it falls back to the terminal renderer.
On first launch, accept the Terms of Service with **Y** or **Enter**.
Acceptance is remembered in
`~/.glitchcraft_terms_accepted`; declining with **N** or **Esc** exits.

## Current prototype

- Title screen, singleplayer world creation, and a one-time Terms of Service
  screen.
- Deterministic, procedurally generated terrain queried at arbitrary
  coordinates; local edits include placing and mining blocks.
- Custom 3D renderers in `GameEngine/`: a software triangle rasterizer with
  Z-buffer, directional lighting, and generated pixel-art textures for blocks
  and rails on desktop; the GameCube GX renderer uses the same shared 16x16
  materials packed into a hardware texture atlas.
- First-person camera with basic collision, jumping, flight,
  pause, inventory, and timed potion effects. Milk removes potion effects;
  cake, cookies, crackers, sandwiches, spaghetti, pizza, and cotton candy
  replenish the food meter.
- Grass, dirt, stone, wood, water, lava, and redstone block types.
- Placeable straight, curved, X-intersection, custom-curve, switch, hill, and
  turntable track pieces, provided terrain supports the rail.
- Placeable 4014 Big Boy, CSX, NS, BNSF, and Union Pacific locomotives; Big Boy
  tender; boxcar, hopper, gondola, flatcar, tank car, caboose, and autorack.
  Rolling stock is rendered with wheels, body silhouettes, and knuckle
  couplers in both 3D renderers. Adjacent stock is assigned to a train consist.
- Diesel locomotives start with diesel fuel. The Big Boy needs an adjacent
  tender (which starts with 100 units of oil) and consumes its oil while
  powered; the engine itself has no onboard fuel.
  Refueling and basic powered movement are implemented; switches, curves,
  hills, and turntables have preliminary route behavior.
- Redstone is currently a placeable block; circuitry and powered mechanisms
  are not implemented yet.
- GlitchCraft Realms currently provides private, player-hosted LAN sessions on
  desktop: choose **M → H** to host or **M → J** to join by host IPv4 address.
  The host sends the world seed and initial block edits; player positions and
  subsequent block edits are synchronized over TCP port 28777. Allow that port
  through the host's LAN firewall. This MVP has no accounts, cloud servers,
  world persistence, encryption, or internet matchmaking; rail/vehicle state
  and inventories are not yet replicated.
- C ABI in `include/glitchcraft/game_api.h` for use from C and other runtimes.
- C graphics kernels and PowerPC assembly replacements for vector dot products
  and depth-buffer clearing. CMake selects the PowerPC kernels on PowerPC builds.

### Keyboard controls

| Key | Action |
| --- | --- |
| W / A / S / D | Move |
| Double-tap a movement key | Sprint |
| I / J / K / L | Look up / left / down / right |
| Arrow keys | Also look |
| Space | Jump (or rise while flying) |
| F | Toggle flight |
| R / P | Mine / place the selected block, rail, or vehicle |
| X / B | Open / close inventory |
| 1 / 2 | Drink potion / milk |
| While inventory is open | 3-9 eat food; G/D/S/W/T/U/V select a block |
| C | Cycle block, track, and vehicle build modes |
| [ / ] | Select the previous / next rail or vehicle |
| O | Rotate the selected rail orientation |
| Y | Toggle locomotive power |
| E | Add 100 units of compatible fuel to targeted rolling stock |
| H | Toggle the targeted switch rail |
| Esc | Pause or resume |
| M | Open the LAN host/join menu |
| Q | Quit |

The controls are a keyboard stand-in for the intended GameCube controller:
left stick movement, C-stick look, A jump, B close inventory, X open inventory,
L place, R mine, and Start pause. Food can be eaten from inventory; water and
lava blocks are not drinkable. Z voice chat is not available in this offline
prototype. Realms are available as private host/join LAN sessions on desktop;
the GameCube runtime remains offline-only.

## GameCube build

The GameCube target uses devkitPro's homebrew `devkitPPC` compiler, `libogc`,
and the Nintendo GX API. The official devkitPro container image is pinned in
`scripts/build-gamecube.sh`; install it once and build a DOL with:

```sh
docker pull devkitpro/devkitppc@sha256:44cb1a920e1ec3ec7c06767493c3b85f8d643d6137cc4661f0201895ac6e4967
./scripts/build-gamecube.sh -j2
```

The output is `build/gamecube/glitchcraft.dol`. The same makefile can build
directly if `DEVKITPRO` and `DEVKITPPC` are installed on the host:

```sh
make -f Makefile.gamecube
```

The GX backend and GameCube controller runtime are in
`GameEngine/platform/gamecube/` and `gamecube/src/`. The controller maps the
main stick to movement, C-stick to looking, A to jump, X/B to inventory, L/R
to place/mine, and Start to pause. D-pad up toggles flight; Y cycles the
build mode (and rotates rail orientation while track mode is selected); D-pad
left/right cycles the current block, rail, or vehicle. L places the selected
object and R mines. A jumps except in vehicle mode, where it toggles locomotive
power. While the inventory is open, D-pad right drinks a potion, left drinks
milk, and up refuels compatible rolling stock; Y toggles a targeted switch.
The GameCube runtime builds and links with devkitPro, but
should still be tested in Dolphin or on hardware.

devkitPro/libogc is an open-source homebrew toolchain, not Nintendo's
proprietary official SDK. This is an original GX-based block-world renderer,
not Mojang's game engine or a recreation of proprietary Xbox 360 assets.
