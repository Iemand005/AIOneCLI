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

    factory.loadSDMAsync(modelPath, options, [this, quiet, onDone = std::move(onDone)](SDModelPtr model) {
        if (!quiet) {
            onModelLoadDone();
        }

        this->model = std::move(model);
        onDone();
    });
}

void TermSDModelManager::createImage(bool quiet, bool showProgress, SDPreviewMode previewMode, bool previewNoise, std::string positivePrompt, std::string negativePrompt, std::optional<std::string> filename, SDImageOptions options) {
    SDModel *model = getSDM();
    model->setProgressCallback([showProgress, &options](const float progress, const int step, const int steps) {
        if (showProgress && steps == options.stepCount) {
            std::cout << progress * 100 << "%" << std::endl;
        }
    });
    
    std::string actualFilename;

    if (filename.has_value()) {
        actualFilename = filename.value();
    } else {
        uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        actualFilename = std::to_string(ms) + ".png";
    }

    if (previewMode != SDPreviewMode::None) {
        model->setPreviewCallback([&model, &actualFilename, previewNoise](int step, SDImage* image, bool isNoisy) {
            if (isNoisy == previewNoise) {
                model->saveImageAsPNG(*image, actualFilename);
            }
        }, previewMode);
    }

    const SDImage image = model->generateImage(positivePrompt, negativePrompt, options);
    model->clearProgressCallback();
    model->saveImageAsPNG(image, actualFilename);

    if (quiet) {
        std::cout << actualFilename << std::flush;
    } else {
        std::cout << "  Saved image as " << ANSI::FG_GREEN << ANSI::BOLD << actualFilename << ANSI::RESET << std::endl;
    }
}

SDModel* TermSDModelManager::getSDM() {
    return model.get();
}


void TermSDModelManager::onModelLoadProgress(const float progress, const int step, const int steps) {
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