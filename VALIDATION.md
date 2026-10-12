# Validation · Pocket Rally 0.6

Built and tested in the development environment. Both JC2432W328C boards were attached over USB: both were flashed, and both boot reports showed touch and radio initialising. Gameplay on the boards, pairing and the RGB LED have not been observed here.

| Check | Result |
| --- | --- |
| ESP32 game firmware, pinned PlatformIO dependencies | Compiles and links successfully |
| Display/touch diagnostic firmware | Compiles and links successfully |
| Game static RAM | 104,040 / 327,680 bytes (31.8%) |
| Game application flash | 842,649 / 1,310,720 bytes (64.3%) |
| Diagnostic static RAM / application flash | 81,620 / 356,361 bytes |
| Full images with bootloader and partitions | Generated for flashing at 0x0 |
| Flashed to both boards | Hash verified. Boot banner reports `POCKET RALLY 0.6`, touch=1, radio=1 and about 185 KB free heap on each |
| Portable C++ game tests, `-Wall -Wextra` | Passed without warnings |
| Two-bot complete race | Finishes in about 107 seconds, three laps. Eight seeds range from 111 to 158 seconds |
| Bot mistakes | Each bot errs every 3–8 seconds and leaves the ribbon |
| Bot backtracking | Turns back for the nearest skipped section within 24 sections, then resumes forward. It gives up on overshoot or after 6 seconds. Longest observed backtrack: 3.6 seconds |
| Wide ribbon | Positions 18 world pixels from the centre line collect coverage (ribbon half-width 20, previously 12) |
| Speed upgrades | Collected on contact, cap at three, respawn elsewhere after 5 seconds. Three upgrades raise cruise speed from 48 to 65.3. Out-of-range upgrades and pickups are rejected |
| Ribbon coverage | One missing section blocks the lap; repeat visits add nothing; collecting the final section completes it |
| Personal progress | Each player owns their own coverage mask |
| Reverse driving | Collects coverage; a reverse-order lap passes |
| Lap reset | First two full coverages clear; third completes the race |
| Movement outside the ribbon | Acceleration matches the ribbon; outside positions add no coverage |
| Glider overlap | Resolves into separated positions |
| Finished race | Physics freezes |
| Invalid incoming state | Nonfinite positions/timers, out-of-range turbo and inconsistent coverage masks/counts are rejected |
| Turbo | Two-second duration, five-second recovery, repeated requests, denied cooldown taps and counter wrap passed |
| Rotating camera | Glider stays fixed at (60, 118), forward maps up, right maps right across headings |
| Link v4 messages (stub radio and clock) | Proportional steering and out-of-range rejection; duplicate, lost, reordered and old-race inputs; protocol mismatch; timeout; rematch; reboot; guest acknowledgement; coverage snapshots and invalid coverage rejection passed |
| Packet size | 180 bytes, within the 250-byte ESP-NOW limit |
| Preview logic execution | Passed under Node with a canvas renderer and simulated DOM controls |
| Preview parity | Pickup seeds and bot timers match the C++ code exactly. A two-bot trajectory matches the firmware within 0.01 world units for 1,000 ticks. Long races then drift slightly, because JavaScript uses 64-bit floats and the firmware uses 32-bit floats |
| Preview interactions | Canvas drag steering (dead zone, proportional, release centres), turbo strip (press and slide-in, once per entry), keyboard and buttons, turbo expiry/cooldown, pause timer freeze, solo mode, result, rematch, missing-section enforcement, coverage reset, pickups and backtracking passed |
| Coverage rendering | A short missed section stays dark between lit neighbours; minimap background remains transparent |
| Pixel frame inspection | Firmware and preview frames inspected: bright field and ribbon, top HUD and turbo strip, upgrade pips, pickup chevrons and minimap dots, steering marker. The preview differs from the firmware renderer by at most 12 single pixels, all float-rounding ties |

`renders/player-1.png` and `player-2.png` are actual software-rendered frames from the same instant, enlarged with nearest-neighbour scaling. The browser preview uses a JavaScript port of the physics, bots and renderer. Its track, sprites, font, palette, layout and rule constants come from the actual C++ code via `web/art.json`. It is not an emulation of the ESP32 or of the radio.

Still requiring physical-board verification:

- Actual colours and brightness of the new palette on the panel, touch axis orientation, and the feel of proportional steering and the top turbo strip.
- The RGB LED pins (4/16/17, active low, assumed from the common CYD wiring) and the power-on red/green/blue flash.
- ESP-NOW discovery, concurrent play, interruption, reboot/reconnection, and interference behaviour.
- Actual frame rate with the larger world view, dynamic free heap, touch-to-remote-screen delay and visible prediction corrections.

The source requests a 60 Hz game tick, about 60 Hz guest input, about 30 Hz host snapshots and a 30 fps render target. These rates are configuration targets rather than measurements.

The link tests replace the radio, queue and clock with test doubles. They verify message/state handling, not physical ESP-NOW transmission or latency.
