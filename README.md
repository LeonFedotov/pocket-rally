# Pocket Rally · 0.6

An abstract top-down pixel racer for **two Guition JC2432W328C capacitive-touch ESP32 displays**. One winding circuit, two geometric gliders, three laps. Both devices share the same race, with a separate rotating camera following each player. The same firmware runs on both.

## Play

1. Power both boards. Choose **HOST RACE** on one and **JOIN RACE** on the other.
2. Once paired, tap Ready on both. A shared three-second countdown starts the race.
3. Slide one finger anywhere across the screen to steer: the further from the centre, the harder the turn, with a small straight-ahead zone in the middle. A marker on the bottom edge shows your steering. Acceleration is automatic; lift your finger to straighten. Your glider stays pointed up while the world scrolls and rotates around it.
4. Touch the **TURBO strip** across the top of the screen, or slide up into it, for a **2-second burst**, then wait **5 seconds** for it to recharge. The strip's bar shows burst time remaining, then recharge progress. Holding the strip does not retrigger turbo; leave it and come back for the next burst.
5. Glide over every section of the ribbon to light it in your colour. **100% coverage completes a lap automatically**, then clears your coverage for the next lap. First to complete three laps wins. You may collect sections in either direction or order. Moving outside the ribbon keeps normal speed; contact still pushes the other glider.
6. Pulsing magenta chevrons are **speed upgrades**. Each one you drive through raises your top speed by 12% for the rest of the race, up to three (the pips beside your P label). Three are on the ribbon at a time; a collected one reappears elsewhere after 5 seconds.
7. Tap both screens for a rematch. Hold the physical **BOOT** button for one second to return to the menu.

The artwork uses bright, saturated colours for the dim panels: a grass-green field, a wide sand-yellow ribbon (40 world pixels across) with pixel-dissolved edges, and two geometric gliders. Blue/P1 has a filled diamond silhouette; red/P2 has an open diamond. Small world-anchored specks provide a motion reference, and turbo lengthens the glider's particle trail. Covered ribbon sections turn light blue or pink on that player's screen. The 1,457-world-pixel circuit retains its long sweeping bends, with no lane markings, curbs or finish-line graphic.

The transparent overview map in the upper right shows the full circuit and live geometric ghosts. Pale sections remain to be covered; coloured sections show your coverage, and magenta dots mark speed upgrades. P1 is blue and P2 red, with your label underlined. The map keeps a fixed orientation while the main view rotates. Close ghost icons separate slightly, with lines pointing to their true map positions. Coverage can finish anywhere on the ribbon. Gliders far outside the ribbon are clamped to the map edge. The map is visible during active racing; countdown, pause and results overlays take priority.

Coverage is tracked separately for each player across **192 short ribbon sections** (about 7.6 world pixels each). A section counts when your glider's centre enters it within the ribbon width. Visiting the same section again adds nothing. Skipping a section keeps the lap incomplete until you return. Turbo and shortcuts outside the ribbon are allowed, but every ribbon section still needs a visit.

The host is blue/P1, the guest red/P2. Practice runs on one board against a bot. The bot is fallible: now and then it oversteers off the ribbon, and when that skips sections it turns around, drives back to collect them, then carries on.

The onboard RGB LED shows the link: **solid green** while connected to the other board, **blinking red** while searching or after the link drops, and off in the menu and practice. It flashes red, green, blue once at power-on. No router, internet, Wi-Fi password, or manual MAC-address entry is needed. With several hosts nearby, Join selects the first matching host advertisement; use one pair at a time in this version.

## Try the browser preview

Open `web/preview.html` locally. No installation, server, or network is required.

- **Race a bot:** you guide blue.
- **Two players:** blue uses A/D and W for turbo; red uses left/right arrows and Up for turbo. Each screen also takes the hardware's touch controls: drag across it to steer, touch its top strip for turbo.
- **Watch race:** both gliders move automatically.

The preview uses glider sprites, font and track geometry exported directly from the firmware renderer, plus a JavaScript port of its physics and rotating-camera renderer. It demonstrates gameplay locally; it does **not** measure or simulate ESP-NOW latency. The included screen renders come from the software renderer; they are not photographs of the boards.

## Flash the two boards

`dist/pocket-rally-0.6.bin` is a full image, to be flashed at **0x0**. Use that same file on each board. **Update both boards:** v0.6 uses protocol v4 (proportional steering and speed upgrades), which cannot pair with v0.5 or earlier. Flashing replaces the software currently on the board.

Install Python's serial flasher:

```sh
python3 -m pip install 'esptool==4.8.1'
python3 -m serial.tools.list_ports
```

Connect a board using a data-capable USB-C cable. Substitute its serial port below (`/dev/cu...` on macOS, `/dev/ttyUSB...` on Linux, or `COM...` on Windows):

```sh
python3 -m esptool --chip esp32 --port YOUR_PORT --baud 460800 write_flash --flash_mode dio --flash_size 4MB 0x0 dist/pocket-rally-0.6.bin
```

Repeat with the other board. esptool puts the board into download mode by itself through the USB-serial chip. Only if connecting fails, hold **BOOT**, tap **RESET**, and release BOOT when the upload connects. Try baud 115200 if transfers are unreliable. Disconnect any serial monitor before flashing.

## Check the actual display revision

This configuration follows the ST7789/CST820 capacitive board. It is not the resistive ESP32-2432S028R configuration. The hardware has not been physically tested in this development environment.

| Function | GPIO / setting |
| --- | --- |
| Display | ST7789, portrait 240×320 |
| SPI clock / MOSI | 14 / 13 |
| Display CS / DC | 15 / 2 |
| Backlight | 27, active high |
| Touch | CST820-family I²C, address 0x15 |
| Touch SDA / SCL / reset | 33 / 32 / 25 |
| Exit-to-menu button | BOOT, GPIO 0 |
| RGB LED (red / green / blue) | 4 / 16 / 17, active low |

If the screen is blank, colors look wrong, or touch is reversed, first flash `dist/pocket-rally-diagnostic-0.6.bin` at 0x0. It displays red/green/blue bars and draws touch points, with coordinates printed at 115200 baud. Then reflash the game.

The backlight (GPIO 27) is driven fully on; there is no dimmer setting. The RGB LED pins follow the common CYD wiring. If the power-on red/green/blue flash shows the wrong colours or none, set `RALLY_LED_R`, `RALLY_LED_G`, `RALLY_LED_B` or `RALLY_LED_ACTIVE_LOW` in the build flags.

The display configuration is isolated in `include/display.hpp`. `RALLY_TFT_INVERT`, `RALLY_TFT_BGR`, `RALLY_TOUCH_FLIP_X`, and `RALLY_TOUCH_FLIP_Y` can be set to 1 in PlatformIO build flags as needed for a particular revision. The display SPI clock is 40 MHz; lower `freq_write` to 27000000 if the physical panel shows artifacts. The game uses portrait orientation and expects touch X 0–239, Y 0–319.

## Build from source

Use the PlatformIO extension in VS Code, or:

```sh
python3 -m pip install 'platformio==6.1.18'
pio run -e jc2432w328c
pio run -e diagnostic
pio run -e jc2432w328c --target upload --upload-port YOUR_PORT
```

Pinned dependencies: PlatformIO Espressif32 6.10.0 (Arduino-ESP32 2.0.17) and LovyanGFX 1.2.7. Newer Arduino-ESP32 versions changed the ESP-NOW callback signatures; do not silently change the pinned platform.

To create a full binary after rebuilding, use `tools/package_firmware.py`. It merges the bootloader, partition table, boot-app image, and application at their appropriate offsets.

## How synchronization works

- ESP-NOW on fixed radio channel 6, with Wi-Fi power saving disabled.
- Broadcast discovery, then a single selected peer using unicast packets.
- One host owns physics, collisions, turbo timers, per-player ribbon coverage, speed upgrades and their spawn positions, lap completion, countdown, and the result. The guest predicts motion between authoritative state updates for immediate local feedback.
- Guest input is sent about every 16 ms; host state about every 33 ms. Rendering targets 30 fps, with physics at 60 Hz. These are software targets, not measured hardware guarantees.
- Entire current input/ready state and race state are repeated. Lost packets do not lose a one-time Ready or Turbo event. Each turbo tap has a repeated 16-bit request counter, acknowledged in the player state; duplicates cannot extend or restart the burst. Requests from an old race are ignored. Session IDs, boot nonces, source MAC checks, and increasing sequence numbers reject stale or unrelated state.
- After 600 ms without valid peer traffic, the race pauses. The host advertises again to allow the same peer to reconnect. A rebooted host creates a new session and returns the pair to the lobby.
- Pixel frames are rendered locally at 120×160 and enlarged exactly 2×. Network messages contain game state, not screen images. The ghost map uses those same glider positions. Each snapshot also carries each player’s 192-bit coverage mask and count, upgrade level, and the three pickups. The 180-byte packet fits ESP-NOW’s 250-byte limit; nothing needs extra packets. Steering is sent as a percentage (-100 to 100).

This version uses unencrypted local ESP-NOW pairing. Pairing is for convenience, not authentication against an adversarial nearby player. There is no online multiplayer or remote service.

## Validation and remaining hardware checks

See `VALIDATION.md` for the completed build and test results. The remaining checks need your physical boards: display colors/orientation, touch coordinates, pairing and reconnection, sustained frame rate, and actual touch-to-other-screen latency.

Run the portable physics tests with:

```sh
g++ -std=c++17 -O2 -Iinclude test/game_test.cpp -o game-test
./game-test
g++ -std=c++17 -O2 -Itest/fakes -Iinclude test/link_test.cpp -o link-test
./link-test
```

The test covers complete bot races (including bot mistakes and backtracking), the wide ribbon, speed pickups and their limits, finish freeze, collision separation, invalid numeric state, personal coverage, missed-section enforcement, reverse-order collection, lap reset, equal speed inside/outside the ribbon, turbo duration/cooldown/deduplication, and camera rotation. `test/link_test.cpp` checks link messages with stub radio and clock functions; it does not test radio hardware. The optional preview test uses Node and `@napi-rs/canvas`.

To regenerate the browser art and preview after editing the renderer or game constants (`art.json` also carries the palette, layout and rule constants the preview reads):

```sh
g++ -std=c++17 -O2 -Iinclude tools/export_art.cpp -o export-art
./export-art web/art.json
python3 tools/build_preview.py
```

## Hardware and API references

- [Guition JC2432W328C product specification](https://www.guition.com/esp32-display-module/esp32-display-lcd)
- [Working JC2432W328C display/touch example and pin mapping](https://github.com/pay191/DIY_Malls-JC2432W328C_Tests/tree/main/BigYellowDisplay_3_CapcitiveTouch)
- [openHASP's JC2432W328 hardware support](https://www.openhasp.com/0.7.0/hardware/guition/jc2432w328/)
- [Espressif ESP-NOW documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/network/esp_now.html)
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)

Original game code and procedural pixel artwork are MIT licensed. Third-party dependencies retain their own licenses.
