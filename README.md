 # GlitchCraft: GameCube Edition — Stable 1.0.0

 GlitchCraft is an original voxel sandbox in **Stable** release. It is written in C++
with a C-compatible game API and PowerPC assembly math kernels. Desktop builds
use a custom software 3D voxel renderer in an X11 window; the GameCube target
uses a native GX hardware renderer and GameCube controller input through
devkitPro/libogc.

Stable 1.0.0 marks the supported feature set documented here. It does not
claim that every planned game system is implemented: worlds exist only in
memory, and LAN sessions are desktop-only and do not persist worlds. Review
the known limitations below before hosting or joining a session.

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

## Stable features and known limitations

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
- Placeable white, orange, magenta, light blue, yellow, lime, pink, gray,
  light gray, cyan, purple, blue, brown, green, red, and black shulker boxes.
  Each has a separate 27-slot inventory with 64-item block stacks.
- Placeable asphalt and marked road tiles: straight, curve, T-intersection,
  cross-intersection, dead end, and roundabout. Each road tile has its own
  generated top-surface markings and shared asphalt sides on desktop and GX.
- Placeable straight, curved, X-intersection, custom-curve, switch, hill, and
  turntable track pieces, provided terrain supports the rail.
- Placeable 4014 Big Boy, CSX, NS, BNSF, and Union Pacific locomotives; Big Boy
  tender; boxcar, hopper, gondola, flatcar, tank car, caboose, and autorack.
  Rolling stock is rendered with wheels, body silhouettes, and knuckle
  couplers in both 3D renderers. Adjacent stock is assigned to a train consist.
- Shinkansen train sets with red, yellow, blue, green, light blue, cyan, white,
  gray, and black front engines, rear engines, and separately placeable
  matching passenger coaches. New York City subway sets cover routes A-Z, each
  with a separately placeable, matching route engine and coach. Passenger
  engines are electric, need no onboard fuel, and run on the existing rail
  network; streamlined Shinkansen noses, windows, and route-color liveries
  render on desktop and GameCube.
- Flying Scotsman steam locomotive with a separately placeable oil tender and
  separate passenger coaches. The locomotive runs only when coupled to its
  matching tender, whose oil supply is consumed while powered.
- Diesel locomotives start with 100 units of diesel. The Big Boy needs an
  adjacent tender (which starts with 100 units of oil); its oil supply is
  consumed while powered and reported on the locomotive state. Fuel tanks
  hold up to 1,000 units. Loading validates oil/diesel compatibility; aim at
  the diesel locomotive or Big Boy tender and press E on desktop, or open the
  inventory and press Up on GameCube to add 100 units. The C API also supports
  explicit fuel type and amount. Switches, curves, hills, and turntables have
  preliminary route behavior.
- Redstone is currently a placeable block; circuitry and powered mechanisms
  are not implemented yet.
- GlitchCraft Realms currently provides private, player-hosted LAN sessions on
  desktop: choose **M → H** to host or **M → J** to join by host IPv4 address.
  The host sends the world seed and initial block edits; player positions and
  subsequent block edits are synchronized over TCP port 28777. Allow that port
  through the host's LAN firewall. This MVP has no accounts, cloud servers,
  world persistence, encryption, or internet matchmaking; rail/vehicle state
  and inventories are not yet replicated.
- C ABI in `include/glitchcraft/game_api.h` for use from C and other runtimes,
  including `gc_game_get_track`, `gc_game_get_track_at`,
  `gc_game_set_track_at`, `gc_game_remove_track_at`,
  `gc_game_set_track_switch`, and type-checked `gc_game_load_train_fuel`.
  Indexed track enumeration is sorted by world coordinates for repeatable
  results; direct track edits require valid terrain support and cannot remove
  or replace track occupied by rolling stock.
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
| While inventory is open | 3-9 eat food; G/D/S/W/T/U/V select a block; [ / ] cycle every block |
| C | Cycle block, track, and vehicle build modes |
| [ / ] | Select the previous / next rail or vehicle |
| O | Rotate the selected rail orientation |
| Y | Toggle locomotive power |
| E | Open a targeted shulker box, or refuel targeted rolling stock |
| Shulker box open | [ / ] select a slot; Y store 64 of the selected block; N clear slot |
| H | Toggle the targeted switch rail |
| Esc | Pause or resume |
| M | Open the LAN host/join menu |
| Q | Quit |

The controls are a keyboard stand-in for the intended GameCube controller:
left stick movement, C-stick look, A jump, B close inventory, X open inventory,
L place, R mine, and Start pause. Food can be eaten from inventory; water and
lava blocks are not drinkable. Z voice chat is not available in this offline
GameCube release. Realms are available as private host/join LAN sessions on
desktop; the GameCube runtime remains offline-only.
All shulker colors are selectable in block mode with D-pad left/right. Aim at a
box and press X to open its 27-slot grid; D-pad left/right selects a slot, Y
stores 64 of the selected block, A clears the slot, and B closes the box.
Container data is not synchronized to other LAN players.

## GameCube build

The GameCube target uses devkitPro's homebrew `devkitPPC` compiler, `libogc`,
and the Nintendo GX API. The official devkitPro container image is pinned in
`scripts/build-gamecube.sh`; install it once and build the DOL and image with:

```sh
docker pull devkitpro/devkitppc@sha256:44cb1a920e1ec3ec7c06767493c3b85f8d643d6137cc4661f0201895ac6e4967
./scripts/build-gamecube.sh -j2
```

The normal build creates `build/gamecube/glitchcraft.dol` and
`build/gamecube/glitchcraft-1.0.iso`. The ISO is a minimal, unencrypted GameCube
homebrew image made with this project's original apploader, boot header, BI2
metadata, DOL, and filesystem table; it is not a retail-authenticated disc.
Try it in Dolphin first. This environment has not run the image in Dolphin or
on physical hardware, so compatibility is not yet confirmed. Burned media
requires compatible console hardware and is not supported on an unmodified
retail GameCube; a homebrew loader such as Swiss is the recommended hardware
route.

Build only the image with `./scripts/build-gamecube.sh image -j2`. The
standalone image packer is `scripts/make-gamecube-image.py`. The same makefile
can build directly if `DEVKITPRO` and `DEVKITPPC` are installed on the host:

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
The GameCube runtime builds and links with devkitPro, but should still be
tested in Dolphin or on hardware.

devkitPro/libogc is an open-source homebrew toolchain, not Nintendo's
proprietary official SDK. This is an original GX-based block-world renderer,
not Mojang's game engine or a recreation of proprietary Xbox 360 assets.
