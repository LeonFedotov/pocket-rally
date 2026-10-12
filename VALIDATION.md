# Validation · Pocket Rally 0.5

Completed in the development environment; no physical JC2432W328C was attached.

| Check | Result |
| --- | --- |
| ESP32 game firmware, pinned PlatformIO dependencies | Compiles and links successfully |
| Display/touch diagnostic firmware | Compiles and links successfully |
| Game static RAM | 102,872 / 327,680 bytes (31.4%) |
| Game application flash | 840,033 / 1,310,720 bytes (64.1%) |
| Diagnostic static RAM / application flash | 80,468 / 355,461 bytes |
| Full images with bootloader and partitions | Generated for flashing at 0x0 |
| Portable C++ game tests, `-Wall -Wextra` | Passed without warnings |
| Two-bot complete race | Finishes in about 82.1 seconds, three laps |
| Ribbon coverage | One missing section blocks the lap; repeat visits add nothing; collecting the final section completes it |
| Personal progress | Each player owns their own coverage mask |
| Reverse driving | Collects coverage; a reverse-order lap passes |
| Lap reset | First two full coverages clear; third completes the race |
| Movement outside the ribbon | Acceleration matches the ribbon; outside positions add no coverage |
| Glider overlap | Resolves into separated positions |
| Finished race | Physics freezes |
| Invalid incoming state | Nonfinite positions/timers, out-of-range turbo and inconsistent coverage masks/counts rejected |
| Turbo | Two-second duration, five-second recovery, repeated requests, denied cooldown taps and counter wrap passed |
| Rotating camera | Glider stays fixed, forward maps up, right maps right across headings |
| Link v3 messages (stub radio and clock) | Duplicate, lost, reordered and old-race inputs; protocol mismatch; timeout; rematch; reboot; guest acknowledgement; coverage snapshots and invalid coverage rejection passed |
| Packet size | 164 bytes, within the 250-byte ESP-NOW limit |
| Preview logic execution | Passed under Node with a canvas renderer and simulated DOM controls |
| Preview interactions | Keyboard/touch controls, turbo expiry/cooldown, pause timer freeze, solo mode, result, rematch, missing-section enforcement and coverage reset passed |
| Coverage rendering | A short missed section stays dark between lit neighbours; minimap background remains transparent |
| Pixel frame inspection | Firmware and preview artwork inspected, including the transparent ribbon map, geometric ghosts, distinct filled/open gliders, dissolved ribbon edges, turbo particles and coverage percentage |

The browser preview uses a JavaScript port of the physics. Its track, sprites and font come from the actual C++ renderer; its camera and ribbon rasterizer are ported to JavaScript. `renders/player-1.png` and `player-2.png` are actual software-rendered frames from the same instant, enlarged with nearest-neighbour scaling. It is not an emulation of the ESP32 or of the radio. A full browser layout test was not available; layout is responsive, with the two screens stacking on narrow displays.

Still requiring physical-board verification:

- Panel initialization, actual colors, touch axis orientation and finger-release behavior.
- ESP-NOW discovery, concurrent play, interruption, reboot/reconnection, and interference behavior.
- Actual frame rate, dynamic free heap, touch-to-remote-screen delay and visible prediction corrections.

The source requests a 60 Hz game tick, about 60 Hz guest input, about 30 Hz host snapshots and a 30 fps render target. These rates are configuration targets rather than measurements.

The link tests replace the radio, queue and clock with test doubles. They verify message/state handling, not physical ESP-NOW transmission or latency.
