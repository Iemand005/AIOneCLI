#include "CLIModelManager.hpp"
#include <iostream>

#include <ModelManager.hpp>

#include "ANSI.hpp"

CLIModelManager::CLIModelManager(const std::string& path) : modelPath(path) {
    manager.initLlama();
}

void CLIModelManager::loadModel(LLModelOptionsAsync options, FinishedCallback onDone) {
    options.onProgress = onModelLoadProgress;
    options.onDone = [this, onDone = std::move(onDone)]() {
        onModelLoadDone();
        onDone();
    };

    manager.loadLLMAsync(modelPath, options);
}

void CLIModelManager::onModelLoadProgress(const float progress) {
    std::cout
        << ANSI::CARRIAGE_RETURN
        << ANSI::CLEAR_LINE
        << ANSI::FG_YELLOW
        << "Loading model " << progress
        << ANSI::RESET
        << std::flush;
}

void CLIModelManager::onModelLoadDone() {
    std::cout
        << ANSI::CARRIAGE_RETURN
        << ANSI::CLEAR_LINE
        << ANSI::FG_BRIGHT_GREEN
        << "Loaded model"
        << ANSI::RESET
        << std::endl;
}