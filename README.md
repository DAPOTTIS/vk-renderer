# My Vulkan Renderer
This is the repo to my simple vulkan engine (LVE) following the [Brendan Galea](https://www.youtube.com/playlist?list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR) tutorial.

The main branch contains the code to the 3D branch.

# Building
This project uses CMake so it can be built with an IDE like VS or Jetbrains CLion, or using either of the following commands:
```bash
$ cmake -DCMAKE_BUILD_TYPE=Debug -B "build/"
```
for a debug build, or:
```bash
$ cmake -DCMAKE_BUILD_TYPE=Release -B "build/"
```
for a release build.

It is configured to automatically compile the shaders then compile the project. I've only tested it on Linux.
