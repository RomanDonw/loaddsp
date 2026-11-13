# Simple PipeWire-based (my) DSP-filters modules' loader.

## Description
Simple loader of my own DSP-modules (PipeWire filters) with my own format.

## Dependencies
- CMake
- Pipewire
- glibc (for `dlmopen`)

## Building
Just configure `/build/` folder through CMake as you want. Also here's supported some CMake options when setting up building environment:
- `BUILD_EXAMPLES` (disabled by default) - enable building example modules (they can be also used as full-fledged, but simple DSP modules).

## Usage
```
loaddsp [unnecessary loaddsp-specified options] <relative path to DSP module (.so library). for example ./dspmod0.so> [optional arguments passed to DSP filter module]
```