# LearningSkinnedAnimation

A small C++/CMake software renderer project.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Debug
.\build\Debug\RedRenderer.exe
```

The renderer loads sample assets from `obj/`. ImGui is expected at
`../imgui-master/imgui-master` relative to this repository.
