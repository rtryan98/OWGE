# Open World Game Engine
A (highly experimental) game and rendering engine, made for open worlds.
It makes use of features only available to recent GPUs and thus requires a DirectX Feature Level 12_2 capable GPU.

## Build
Building OWGE requires CMake 3.18 and is only tested using Visual Studio 2022.
1. Run `cmake -G "Visual Studio 17" -B "build/"`.

## Meta
This project follows the [Canonical Project Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html).
Filenames containing multiple words will be delimited by an underscore.
Private sources and headers are to be placed in `<name>/private/` and are only added to the project with the `PRIVATE` visibility in CMake.

## Legal
Running CMake will download the D3D12 Agility SDK, the DirectStorage SDK and the DirectX Shader Compiler.
By doing so you accept the license terms provided by Microsoft for those SDKs.
A Copy of the Licenses is included in `COPYING.md`.
