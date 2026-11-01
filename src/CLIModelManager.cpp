#include "CLIModelManager.hpp"
#include <iostream>
#include <vector>

#include <ModelManager.hpp>

#include "ANSI.hpp"

CLIModelManager::CLIModelManager(const std::string& path) : modelPath(path) {
    manager.initLlama();
}

void CLIModelManager::loadModel(bool quiet, LLModelOptionsAsync options, FinishedCallback onDone) {
    if (!quiet) {
        options.onProgress = onModelLoadProgress;
    }

    options.onDone = [this, quiet, onDone = std::move(onDone)]() {
        if (!quiet) {
            onModelLoadDone();
        }

        onDone();
    };

    manager.loadLLMAsync(modelPath, options);
}

void CLIModelManager::sendMessage(bool quiet, std::string role, std::string text, bool hasSystemPrompt, std::string systemPrompt, TextGenOptions options) {
    LLModel *llm = manager.getLLM();
    std::vector<Message> messages(hasSystemPrompt ? 2 : 1);
    uint8_t idx = 0;

    if (hasSystemPrompt) {
        Message &systemMsg = messages[idx];
        systemMsg.role = "system";
        systemMsg.content = systemPrompt;
        idx++;
    }
    
    Message &msg = messages[idx];
    msg.role = role;
    msg.content = text;

    llm->completeAny(llm->chatToPrompt(messages), options);
}


void CLIModelManager::onModelLoadProgress(const float progress) {
    std::cout
        << ANSI::CARRIAGE_RETURN
        << ANSI::CLEAR_LINE
        << ANSI::FG_YELLOW
        << "  Loading model " << progress
        << ANSI::RESET
        << std::flush;
}

void CLIModelManager::onModelLoadDone() {
    std::cout
        << ANSI::CARRIAGE_RETURN
        << ANSI::CLEAR_LINE
        << ANSI::FG_BRIGHT_GREEN
        << "  Loaded model"
        << ANSI::RESET
        << std::endl;
}