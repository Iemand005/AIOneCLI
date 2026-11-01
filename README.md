# AIOneCLI

CLI client using AIOne as engine.

## How to build

### CPU (default)

```bash
cmake -S . -B build
cmake --build build --config Release
```

### Vulkan

You can additionally pass `-DAIONE_NOCPU=ON` for GPU-only if you prefer a smaller binary (not recommended because you won't have CPU fallback if model exceeds VRAM).

```bash
cmake -S . -B build -DAIONE_VULKAN=ON
cmake --build build --config Release
```

### CUDA

You can additionally pass `-DAIONE_NOCPU=ON` for GPU-only if you prefer a smaller binary (not recommended because you won't have CPU fallback if model exceeds VRAM).

Replace `<your compute capability>` with the number (without the dot) from https://developer.nvidia.com/cuda/gpus for your GPU.

```bash
cmake -S . -B build -DAIONE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=<your compute capability>
cmake --build build --config Release
```