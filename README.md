# Open World Game Engine
A (highly experimental) game and rendering engine, made for open worlds.
It makes use of features only available to recent GPUs.
Shader Model 6.7 is required which might not be supported on many GPUs.
D3D12 Enhanced Barriers are used extensively and might not be supported on some drivers and GPUs.
On AMD hardware it might be required to download different drivers than the mainline ones.

## Build
Building OWGE requires CMake 3.18 and is only tested using Visual Studio 2022.
1. Run `cmake -G "Visual Studio 17" -B "build/"`.

Optional steps:
- NVIDIA Nsight Perf SDK:
    1. If not already done, set the environment variable `NVPERF_SDK_PATH` to the topmost directory of the SDK.
    2. Enable the cmake option `OWGE_USE_NVPERF`.
- WinPixEventRuntime:
    1. Enable the cmake option `OWGE_USE_WIN_PIX_EVENT_RUNTIME`.

## Meta
This project follows the [Canonical Project Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html) with some variations.
Filenames containing multiple words will be delimited by an underscore.
Private sources and headers are to be placed in `<name>/private/` and are only added to the project with the `PRIVATE` visibility in CMake.

## Legal
Running CMake will download the D3D12 Agility SDK, the DirectStorage SDK, WinPixEventRuntime and the DirectX Shader Compiler.
By doing so you accept the license terms provided by Microsoft for those SDKs.
A Copy of the Licenses is included in `COPYING.md`.

To use NvPerf with OWGE you need to agree to NVIDIA's licence agreement provided by the SDK,
which can be found on [NVIDIA's Nsight Perf SDK page](https://developer.nvidia.com/nsight-perf-sdk).
