# Doctor Migrana – Tuner

A chromatic **guitar-pedal style tuner** plugin (VST3 + Standalone) by **GuitarFros**.

* **Hz knob** – reference pitch of A4, 400 – 480 Hz in 0.1 Hz steps (double-click = 440 Hz), for alternate tunings.
* **Cents knob** – "in tune" tolerance, 1 – 25 cents in 1-cent steps (double-click = 1). The centre green LED lights when
  the string is within ± this many cents of the nearest note; the other LEDs show how far flat / sharp you are.
* **MUTE / BYPASS switch** – while the tuner is active, *left* silences the output (classic tuner mute), *right* lets the signal keep flowing.
* **Footswitch + ACTIVE LED** – turns the tuner on and off. With the pedal off the plugin is fully transparent.

The pitch detector (`Source/PitchDetector.h`) is a YIN implementation with parabolic interpolation, running on a
background thread; the audio callback never allocates or locks. `Tests/test_pitch_detector.cpp` checks that it stays
within 1 cent over E1 – E6 with harmonics and noise.

> *Descrición curta (gl):* afinador cromático en formato pedal, en VST3 e Standalone. Un pomo para o La de referencia
> (Hz), outro para a tolerancia en cents (desde 1 cent), interruptor mute/bypass e footswitch con LED.

## Building

Requirements: CMake ≥ 3.22, a C++17 compiler and the usual JUCE platform dependencies
(Linux: ALSA, X11, freetype, fontconfig, GL – see `.github/workflows/build.yml`). JUCE 8 is downloaded automatically.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
# optional test: cmake -B build -DDMT_BUILD_TESTS=ON && ctest --test-dir build -C Release
```

Outputs are in `build/DoctorMigranaTuner_artefacts/Release/`:

* `VST3/Doctor Migrana - Tuner.vst3`
* `Standalone/Doctor Migrana - Tuner` (`.exe` / `.app`)

**Linux / Reaper:** on Linux the build automatically copies the plugin to `~/.vst3/`, which Reaper scans by default.
Open Reaper → *Preferences → Plug-ins → VST → Re-scan* (first time only) and look for **Doctor Migrana - Tuner** under *FX → VST3*.
Use `-DDMT_COPY_AFTER_BUILD=OFF` to disable the copy. The accuracy test is off by default: `-DDMT_BUILD_TESTS=ON`.

Ubuntu build dependencies:

```sh
sudo apt install build-essential cmake git libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libgl1-mesa-dev
```

Options: `-DDMT_JUCE_DIR=/path/to/JUCE` to use a local JUCE checkout, `-DDMT_COPY_AFTER_BUILD=OFF` to skip installing the VST3 after building (default ON),
`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` for a universal macOS binary.

**Standalone note:** JUCE mutes the audio input of standalone apps by default to avoid feedback. Open the
*Options → Audio/MIDI Settings* dialog, choose your interface and untick *Mute audio input*.

## Releases

Every tag that starts with `v` builds the plugin on **Linux, Windows and macOS** (GitHub Actions) and publishes three ZIPs in the
repository's *Releases* page: `DoctorMigranaTuner-<tag>-Linux.zip`, `-Windows.zip` and `-macOS.zip`. Each one contains the
VST3, the Standalone, `LICENSE`, `NOTICE.md`, `README.md` and `ARTWORK.md`.

To publish a version:

```sh
git tag v1.0.0
git push origin v1.0.0
```

Installing the VST3 by hand:

| System | Copy `Doctor Migrana - Tuner.vst3` to |
|--------|----------------------------------------|
| Linux | `~/.vst3/` |
| Windows | `C:\Program Files\Common Files\VST3\` |
| macOS | `~/Library/Audio/Plug-Ins/VST3/` |

The binaries are **not code-signed**. On macOS, if the system blocks them, run
`xattr -dr com.apple.quarantine "Doctor Migrana - Tuner.vst3"`. On Windows, SmartScreen may show a warning for the Standalone.

## Artwork

`Assets/pedal_base.jpg` and `Assets/knob.png` are the GUI images embedded in the plugin. They were generated with Meta AI and
may be used and distributed in this non-commercial open-source project; the "Doctor Migraña" logo is the property of its owner.
See `Assets/ARTWORK.md`: **the artwork is not covered by the AGPL grant of the source code**. If you fork the project, replace it with your own.
`tools/prepare_assets.py` shows how the images were prepared (LEDs / lever removed, knob cut out).

## License

Copyright (C) 2026 GuitarFros. Licensed under the **GNU Affero General Public License v3.0 or later**
for the **source code** (see `LICENSE` and `NOTICE.md`). JUCE is used under its AGPLv3 option.
The artwork in `Assets/` has separate terms (`Assets/ARTWORK.md`).

*VST is a registered trademark of Steinberg Media Technologies GmbH.*
