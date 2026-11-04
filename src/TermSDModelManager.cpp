#include "TermSDModelManager.hpp"
#include <iostream>
#include <vector>
#include <chrono>

#include <ModelFactory.hpp>

#include "ANSI.hpp"

TermSDModelManager::TermSDModelManager(const std::string& path) : modelPath(path) {
    
}

void TermSDModelManager::loadModel(bool quiet, SDModelOptionsAsync options, FinishedCallback onDone) {
    if (!quiet) {
        options.onProgress = onModelLoadProgress;
    }

    factory.loadSDMAsync(modelPath, options, [this, quiet, &onDone = std::move(onDone)](SDModelPtr model) {
        if (!quiet) {
            onModelLoadDone();
        }

        this->model = std::move(model);
        onDone();
    });
}

void TermSDModelManager::createImage(bool quiet, std::string positivePrompt, std::string negativePrompt, std::optional<std::string> filename, SDImageOptions options) {
    SDImage &image = getSDM()->generateImage(positivePrompt, negativePrompt, options);
    std::string actualFilename;

    if (filename.has_value()) {
        actualFilename = filename.value();
    } else {
        uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        actualFilename = std::to_string(ms) + ".png";
    }
    
    getSDM()->saveImageAsPNG(image, actualFilename);

    if (quiet) {
        std::cout << actualFilename;
    } else {
        std::cout << "  Saved image as " << ANSI::FG_GREEN << ANSI::BOLD << actualFilename << ANSI::RESET << std::endl;
    }
}

SDModel* TermSDModelManager::getSDM() {
    return model.get();
}


void TermSDModelManager::onModelLoadProgress(const float progress) {
    std::cout
        << ANSI::CARRIAGE_RETURN
        << ANSI::CLEAR_LINE
        << ANSI::FG_YELLOW
        << "  Loading model " << progress
        << ANSI::RESET
        << std::flush;
}

void TermSDModelManager::onModelLoadDone() {
    std::cout
        << ANSI::CARRIAGE_RETURN
        << ANSI::CLEAR_LINE
        << ANSI::FG_BRIGHT_GREEN
        << "  Loaded model"
        << ANSI::RESET
        << std::endl;
}