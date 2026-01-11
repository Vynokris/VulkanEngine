# **Vulkan Renderer**

### General Information

This is a solo passion project with the aim of getting a deeper understanding of the Vulkan framework.

### Features

- Use of custom maths for vectors, matrices and quaternions
- Loading of OBJ, MTL and image files at runtime
- Mesh rendering with materials
- Texture mapping (albedo, alpha, normal, roughness, metallic, ambient occlusion)
- Lighting (directional, point, spot)
- Physically based rendering

### Building & Running

- Requires Windows x64 with Visual Studio 2022
- Run `Vulkan/Scripts/DownloadExternals.bat` to fetch all necessary external libs
- Open `Vulkan.sln`, then compile and run in release x64
