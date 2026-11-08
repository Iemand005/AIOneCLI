#include "TermTTSModelManager.hpp"
#include <iostream>
#include <vector>
#include <chrono>

#include <ModelFactory.hpp>

#include "ANSI.hpp"

TermTTSModelManager::TermTTSModelManager(const std::string& path, const std::string& mmprojPath) : modelPath(path), mmprojPath(mmprojPath) {
    
}

void TermTTSModelManager::loadModel(bool quiet, TTSModelOptionsAsync options, ModelOptionsAsync mmprojOptions, FinishedCallback onDone) {
    if (!quiet) {
        options.onProgress = onModelLoadProgress;
    }

    TTSModelOptions &syncOpts = dynamic_cast<TTSModelOptions&>(options);
    ModelOptions &syncMmprojOpts = dynamic_cast<ModelOptions&>(mmprojOptions);

    factory.loadTTSMAsync(modelPath, syncOpts, mmprojPath, syncMmprojOpts, [this, quiet, onDone = std::move(onDone)](TTSModelPtr model) {
        if (!quiet) {
            onModelLoadDone();
        }

        this->model = std::move(model);
        onDone();
    });
}

void TermTTSModelManager::synthesize(bool quiet, bool showProgress, std::string prompt, std::optional<std::string> filename, TTSSynthesisOptions options) {
    TTSModel *model = getTTSM();
    options.onFrame = [showProgress, &options](const int frameIdx, const mtmd_helper::gen_audio &gen) {
        if (showProgress) {
            std::cout << (frameIdx + 1) << std::endl;
        }
    };
    
    std::string actualFilename;

    if (filename.has_value()) {
        actualFilename = filename.value();
    } else {
        uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        actualFilename = std::to_string(ms) + ".wav";
    }

    const TTSSynthesisResult result = model->synthesize(prompt, options);
    model->saveAudioAsWAV(result.audio, actualFilename);

    if (quiet) {
        std::cout << actualFilename << std::flush;
    } else {
        std::cout << "  Saved audio as " << ANSI::FG_GREEN << ANSI::BOLD << actualFilename << ANSI::RESET << std::endl;
    }
}

TTSModel *TermTTSModelManager::getTTSM() {
    return model.get();
}


void TermTTSModelManager::onModelLoadProgress(const float progress, const int step, const int steps) {
    std::cout
        << ANSI::CARRIAGE_RETURN
        << ANSI::CLEAR_LINE
        << ANSI::FG_YELLOW
        << "  Loading model " << progress
        << ANSI::RESET
        << std::flush;
}

void TermTTSModelManager::onModelLoadDone() {
    std::cout
        << ANSI::CARRIAGE_RETURN
        << ANSI::CLEAR_LINE
        << ANSI::FG_BRIGHT_GREEN
        << "  Loaded model"
        << ANSI::RESET
        << std::endl;
}