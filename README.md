# Grass Renderer

Grass rendering playground written in C++, using **Dawn WebGPU**. 
![grass_preview.gif](preview%2Fgrass_preview.gif)

## Features
- Procedural blade generation
- Procedural blade wind movements, controlled by Bézier curves 
- GPU Instancing
- Per-bade Blinn-Phong lighting
- Screen-Space Shadows
- *Experimental* : sphere collisions


## Installation

### Prerequisites
Ensure you have the following tools and hardware requirements:

- CMake 3.20 or higher
- A C++20 compatible compiler (preferably MSVC)
- A WebGPU/Vulkan compatible GPU


### Build Instructions
1. Clone the repository:
```bash
git clone https://github.com/yourusername/grass-renderer.git
cd grass-renderer
```
2. Build the project using CMake (it may take a while):
```bash
cmake -S . -B build
cmake --build build
```

## Usage
- Run the compiled executable in the build directory.
- Hold the right mouse button to activate focus mode. Use the keyboard to navigate the scene and the mouse to control the camera (WASD, Unreal Engine type controls).


## License
This project is licensed under the CC-BY-NC-ND-4.0 License. See the LICENSE file for more details.


## Remaining features to implement and fixes
- Culling methods
- Multiple LODs
- Index buffer 