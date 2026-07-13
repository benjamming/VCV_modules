# Ombak

A VCV Rack module that takes a 1V/octave pitch and outputs a second pitch
offset by a fixed number of Hz, for gamelan-style "ombak" beating/shimmer
effects, plus an unmodified pass-through of the original pitch.

## How it works

Given input volts `v`, base frequency `F0` (fixed at C4 = 261.6256 Hz for
now), and a user-set `delta_frequency` (the DELTA knob, in Hz):

```
input frequency:   f_in  = F0 * 2^v
output frequency:  f_out = f_in + delta_frequency
output volts:       w    = log2( 2^v + delta_frequency / F0 )
```

- **V/OCT** (input) — your pitch source (keyboard, sequencer, etc).
- **THRU** (output) — the input, unchanged.
- **SHIMMER** (output) — the input, shifted by `delta_frequency`.

Run both outputs into a mixer (or two oscillators) to hear the original
pitch and the shifted pitch beating against each other.

The DELTA knob currently ranges from **-15 Hz to +15 Hz** (default **7 Hz**).
Typical "ombak" beating sits around 5-12 Hz; beyond roughly 15-20 Hz most
ears stop hearing one shimmering tone and start hearing two distinct
pitches, so this range covers the musical sweet spot with headroom on
either side. To change it, edit the three constants at the top of
`src/Ombak.cpp`:

```cpp
static constexpr float DELTA_MIN = -15.f;
static constexpr float DELTA_MAX = 15.f;
static constexpr float DELTA_DEFAULT = 7.f;
```

`BASE_FREQ` (also at the top of `src/Ombak.cpp`) is the F0 in your formula.
It's fixed at `dsp::FREQ_C4` for now. Making it user-adjustable (a knob or
CV input) is a natural next step -- the math already supports it, since
`BASE_FREQ` is just a variable in the `w` calculation.

## Building

This was written against the Rack 2 Plugin API, following VCV's own
[Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial)
and [Plugin API Guide](https://vcvrack.com/manual/PluginGuide). It hasn't
been compiled in this environment (no network access / Rack SDK here), so
please build and test it locally:

1. Install VCV Rack and set up your build environment per
   [vcvrack.com/manual/Building](https://vcvrack.com/manual/Building)
   (a C++ compiler toolchain, plus the Rack SDK matching your OS).

2. Point `RACK_DIR` at your extracted Rack SDK:
   ```bash
   export RACK_DIR=<path to Rack-SDK>
   ```

3. From this folder, build:
   ```bash
   make
   ```

4. Install into your Rack user folder and test in the app:
   ```bash
   make install
   ```
   (Or `make dist` to produce a distributable zip in `dist/`.)

5. Open Rack, add the "Ombak" module from the "Ombak" brand in the module
   browser, patch a V/OCT source into it, and patch THRU/SHIMMER into an
   audio path (e.g. two VCOs, or a mixer summing both) to hear the effect.

If the module doesn't appear, check `log.txt` in your Rack user folder for
load errors.

## About the panel

`res/Ombak.svg` is a plain, functional placeholder panel (a wave motif,
knob/port bezels, no text -- Rack's SVG renderer doesn't support fonts/text,
per the [Panel Guide](https://vcvrack.com/manual/Panel)). The component
positions in `src/Ombak.cpp` (the `mm2px(Vec(...))` calls in `OmbakWidget`)
match the bezel positions on this panel. If you design your own panel in
Inkscape later, follow the Panel Guide's `components` layer convention and
update those `Vec(...)` coordinates (or re-run `helper.py createmodule`) to
match your new layout.

## Project layout

```
Ombak/
├── plugin.json       # manifest (name, version, license, module list)
├── Makefile           # standard Rack plugin build file
├── LICENSE.txt
├── res/
│   └── Ombak.svg      # panel graphic
└── src/
    ├── plugin.hpp     # shared declarations
    ├── plugin.cpp     # plugin init / model registration
    └── Ombak.cpp      # the module: DSP (process()) + panel widget
```
