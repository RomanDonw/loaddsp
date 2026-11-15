# Simple PipeWire-based (my) DSP-filters modules' loader.

## Description
Simple loader of my own DSP-modules (PipeWire filters) with my own format.

## Dependencies
### Required
- CMake
- Pipewire
### Opitonal (but highly recommended)
- glibc (for `dlmopen`)

## Building
### CMake options
- `BUILD_EXAMPLES` (disabled by default) - enable building example modules (they can be also used as full-fledged, but simple DSP modules).
- `BUILD_TESTS` (enabled by default) - build test modules.
- `USE_GLIBC_DLMOPEN` (enabled by default) - use safety `dlmopen` from `glibc` for separated "symbol space" instead of POSIX `dlopen`.

## Usage
```
loaddsp [unnecessary loaddsp-specified options] <relative path to DSP module (.so library). for example ./dspmod0.so> [optional arguments passed to DSP filter module]
```