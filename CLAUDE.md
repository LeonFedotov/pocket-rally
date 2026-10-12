# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Pocket Rally is a two-player ESP32 racer for Guition JC2432W328C touch displays (ST7789 + CST820), linked over ESP-NOW. The same firmware runs on both boards. There is also a standalone browser preview (`web/preview.html`) that ports the physics to JavaScript.

## Commands

Portable tests (no hardware or PlatformIO needed). Write binaries to a temp dir, not the repo:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude test/game_test.cpp -o /tmp/game-test && /tmp/game-test [frame.ppm]
g++ -std=c++17 -O2 -Wall -Wextra -Itest/fakes -Iinclude test/link_test.cpp -o /tmp/link-test && /tmp/link-test
```

Each test is a single `main()` built from `assert`s, so there is no per-case runner. A failure aborts with the failing line. Tests must pass with no warnings under `-Wall -Wextra`. `game_test` optionally writes a rendered PPM frame to `argv[1]`.

Firmware (PlatformIO, pinned to espressif32@6.10.0 / Arduino-ESP32 2.0.17 and LovyanGFX 1.2.7):

```sh
pio run -e jc2432w328c        # game
pio run -e diagnostic         # color bars + touch test (-DRALLY_DIAGNOSTIC=1)
pio run -e jc2432w328c --target upload --upload-port PORT
python3 tools/package_firmware.py   # after building both envs → dist/*.bin (flash at 0x0)
```

Do not bump the platform version: newer Arduino-ESP32 versions changed the ESP-NOW callback signatures.

Browser preview pipeline (run after changing `render.hpp`, `game.hpp` constants, or the preview):

```sh
g++ -std=c++17 -O2 -Iinclude tools/export_art.cpp -o /tmp/export-art && /tmp/export-art web/art.json
python3 tools/build_preview.py          # preview-fragment.html + art.json → preview.html, preview.inline.html
node test/preview_test.cjs [out.png]    # needs @napi-rs/canvas resolvable from cwd; reads web/preview.inline.html
```

`tools/render_screens.cpp` (`./render-screens DIR`) writes `player-1.ppm`/`player-2.ppm` from the real renderer. `renders/*.png` are those frames, upscaled 2× with nearest-neighbour.

## Architecture

All game code is header-only under `include/` so the same sources compile for the ESP32 and for host-side tests and tools:

- `game.hpp`: pure, deterministic simulation with no Arduino dependencies. It contains `Track` (Catmull-Rom circuit sampled to 97 points), `Car`/`State` (POD, sent verbatim over the radio), `Game::step()` (fixed 1/60 s tick: turbo → move → collide → coverage → pickups → bot brains → finish), bot AI, and `validState()`, which sanity-checks every incoming network state.
  - Speed pickups live in `State`, with a xorshift `seed`, so the host decides spawns and the guest just copies them.
  - Bot memory (`Game::brains`: mistakes, backtrack target) is local only and never sent. Only practice mode, the tests and the tools use it.
- `render.hpp`: software renderer into a 120×160 RGB565 buffer (`Renderer::pixels`), drawn from a `View` (race state plus UI screen, local player, ready mask, and pause/link flags).
  - Layout: the top `TURBO_ZONE` rows are the HUD and turbo strip, and the world view fills the rest, with the car at `(60,CAR_Y)`.
  - It also holds the palette, glyph font and sprites. `export_art` dumps those into `web/art.json`, together with the layout and rule constants.
- `link.hpp`: ESP-NOW transport. The ISR-context receive callback only copies packets into a FreeRTOS queue. `Link::pump()` in `loop()` does all validation and state changes. The host is authoritative and sends `SNAPSHOT` at about 33 ms. The guest sends `CONTROL` at about 16 ms and predicts locally between snapshots. Every packet carries the full `State`.
- `display.hpp`: LovyanGFX panel config and the raw CST820 I²C touch driver. Revision quirks are handled by `RALLY_TFT_INVERT`, `RALLY_TFT_BGR` and `RALLY_TOUCH_FLIP_X/Y` build flags.
- `src/main.cpp`: wiring only. `loop()` runs on core 1 and handles touch input, BOOT-button and serial commands (`h j p r m t`), the RGB LED link status, `Link::pump`, and a 60 Hz fixed-step accumulator. A separate render task on core 0 copies `sharedView` under `viewMux` and pushes the frame upscaled 2× in scanline pairs. The render task must never touch `game` directly; it reads only `View`, through `publish()`. The `screen` value runs 0=menu, 1=hosting, 2=joining, 3=lobby, 4=race, 5=hardware error.

Invariants to preserve:

- **Packet/protocol:** `State` must stay under 160 bytes and `Packet` within 250 bytes (both have `static_assert`s). `State` is currently 156 bytes, so there is almost no headroom. Any layout or semantic change to `Packet`/`State` requires bumping `PROTOCOL` in `link.hpp`, because peers on other versions are rejected.
- **Turbo is a counter, not an event:** each tap increments a 16-bit `turboRequest`. The game fires a burst only when the counter changes and cooldown is 0, so repeated, reordered or lost packets cannot double-trigger. Keep this idempotent shape for any new one-shot input.
- **Coverage:** each car has a 192-bit mask (`coverage[6]`) plus a `covered` count that must equal its popcount (`validState` enforces this). A lap completes at 100% coverage and then clears the mask.
- **Coordinate spaces:** world 520×540, logical screen 120×160, physical touch 240×320. `onTap` receives halved coordinates. Steering and the turbo-strip check in `loop()` use raw touch coordinates.
- **Steering units:** steering is an `int8` percentage (-100..100) in `Packet`/`View`/`Link`, and a float -1..1 into `Game::step`.
- **JS port:** `web/preview-fragment.html` holds a hand-written JavaScript port of the `game.hpp` physics, bot brains and pickups, plus the renderer's camera, HUD and ribbon rasterizer. Gameplay changes in C++ must be mirrored there by hand. Track points, sprites, font, map geometry, palette, layout and rule constants come from `art.json`. Edit `preview-fragment.html`, never the generated `preview.html` or `preview.inline.html`.
- **Test fakes:** `test/fakes/` stubs `Arduino.h`, `WiFi.h`, `esp_now.h` and related headers, so `link.hpp` compiles on the host. Advance time with `fakeNow`. Feed packets by calling `receivePacket(...)` and then `Link::pump`.

## Conventions

- C++ style is deliberately dense: compact one-line functions, many statements per line, and short names. Match it rather than reformatting.
- The version string `0.6` is hardcoded in `src/main.cpp` (serial banner), `tools/package_firmware.py` (output filenames), `README.md` and `VALIDATION.md`. Update all of them together.
- `VALIDATION.md` records what was verified without hardware versus what still needs physical boards. Update it when test coverage or measured numbers (RAM/flash usage, packet size, bot race time) change.
