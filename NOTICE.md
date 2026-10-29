# Notices

**Doctor Migrana - Tuner** — Copyright (C) 2026 GuitarFros

This program is free software: you can redistribute it and/or modify it under the terms of the
GNU Affero General Public License as published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
General Public License for more details. You should have received a copy of it along with this
program (file `LICENSE`); if not, see <https://www.gnu.org/licenses/>.

## Third-party components

| Component | Use | Licence |
|-----------|-----|---------|
| [JUCE 8](https://juce.com) | application / plugin framework (downloaded by CMake, not redistributed in this repo) | used under its **AGPLv3** option |
| Steinberg VST 3 SDK | bundled inside JUCE | see the licence shipped with JUCE's `modules/juce_audio_processors/format_types/VST3_SDK` |
| YIN algorithm | de Cheveigné & Kawahara, JASA 111(4), 2002 | algorithm only; the code in `Source/PitchDetector.h` is original |

*VST is a registered trademark of Steinberg Media Technologies GmbH.*

## Artwork

The images in `Assets/` are **not** part of the AGPL grant. They were generated with Meta AI and may be used and
distributed in this non-commercial open-source project; the "Doctor Migraña" logo and the project names are the
owner's property. See `Assets/ARTWORK.md`.
