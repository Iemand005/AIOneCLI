#include <iostream>
#include <limits>

#include <llama.h>
#include <ModelManager.hpp>

void onModelLoadProgress(const float progress) {
    std::cout << "\rLoading model " << progress << std::flush;
}

void onModelLoadDone() {
    std::cout << std::endl << "Loaded model" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "Hello world!" << std::endl;

    ModelManager manager;
    LLModelOptionsAsync options;
    options.device = PreferredDevice::DGPU;
    options.onProgress = onModelLoadProgress;
    options.onDone = onModelLoadDone;
    options.useMmap = true;

    manager.loadLLMAsync("F:\\AIOne\\AIOneMetaProject\\AIOneCLI\\models\\Qwen3-0.6B-Q4_K_M.gguf", options);

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    return 0;
}