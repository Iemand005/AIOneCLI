#include <iostream>
#include <limits>

#include <llama.h>
#include <ModelManager.hpp>
#include <argparse/argparse.hpp>

void onModelLoadProgress(const float progress) {
    std::cout << "\rLoading model " << progress << std::flush;
}

void onModelLoadDone() {
    std::cout << std::endl << "Loaded model" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "Hello world!" << std::endl;



    argparse::ArgumentParser program("AIOneCLI");

    program.add_argument("-m", "--model")
        .help("path to the model file")
        .required();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::string modelPath = program.get<std::string>("--model");

    ModelManager manager;
    LLModelOptionsAsync options;
    options.device = PreferredDevice::DGPU;
    options.onProgress = onModelLoadProgress;
    options.onDone = onModelLoadDone;
    options.useMmap = true;

    manager.loadLLMAsync(modelPath, options);

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    return 0;
}