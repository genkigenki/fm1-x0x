# X0X on the M5Stack Cardputer

X0X by [Charles Vestal](https://github.com/charlesvestal) is a groovebox: a 909, an 808, two 303s with an acid line generator and a breakbeat player, written for the M-Vave FM-1. This branch
runs it on the original **M5Stack Cardputer** (v1, ESP32-S3, no PSRAM): the firmware's own sources, with a small
board layer ([mvave-to-cardputer](https://github.com/genkigenki/mvave-to-cardputer)) in place of the FM-1's panel.

Unofficial: not by the firmware's author, not affiliated with M-Vave or M5Stack. At your own risk.

## Install

1. Install [M5Launcher](https://github.com/bmorcelli/Launcher) on the Cardputer (its web flasher or M5Burner).
2. Download `x0x-cardputer.bin` from this fork's [releases](https://github.com/genkigenki/fm1-x0x/releases) and copy it to the SD card.
3. Launcher: SD, pick the file, Install. Start it from the Launcher's app list.

`x0x-cardputer.full.bin` is the whole flash image for a Cardputer without the Launcher (written at `0x0`).

## Keys

```
`    1    2    3    4    5    6  | 7    8    9    0    -    =    del
SEQ  A#4  C#5  D#5   .   F#5   . | SEL+ ALG+ PRE+ K1+  K2+  K3+  K4+
tab  q    w    e    r    t    y   | u    i    o    p    [    ]    HOME B4   C5   D5   E5   F5   G5  | SEL- ALG- PRE- K1-  K2-  K3-  K4-
fn sh a    s    d    f    g    h    j    k    l    ;    '    enter
      OCT- F#3  G#3  A#3   .   C#4  D#4   .   F#4  G#4  OCT+ REC
ct op al z    x    c    v    b    n    m    ,    .    /    space
LK -  +  F3   G3   A3   B3   C4   D4   E4   F4   G4   A4   PLAY
fn+:     FX   SEL  ENV  LFO  EDIT GLO  SAVE ARP
```

- Two piano rows: 27 notes, F3 to G5.
- The seven knobs are key columns: the upper key turns up, the lower one down (shift: 4 steps, held: repeats).
- opt / alt: volume. ctrl: locks the fn layer. G0 stays the Launcher's.

## Differences from the FM-1

- The tape delay is 0.5 s (the FM-1: longer), to fit the RAM.
- No undo (the FM-1 has one), to fit the RAM.
- No MIDI yet (USB, BLE, TRS), no Wi-Fi or Bluetooth features.
- One mono speaker. A speaker chain keeps it clean: a crossover at 180 Hz, "virtual bass" (the bass line's overtones), a look-ahead limiter.

## Build

PlatformIO, in `cardputer/`: `pio run` (never `pio run -t upload` on a Cardputer with the Launcher: it would
overwrite the Launcher's partition layout). The generated headers come first (the firmware's own `BUILDING.md`).
The board layer in `lib/mvave_cardputer` is vendored from the kit.

## Licence

X0X: GPL-3.0, as the original (see `LICENSE`). The board layer: GPL-3.0-only, M. Geisler (genkigenki).
