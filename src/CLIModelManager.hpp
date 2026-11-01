#pragma once
#include <iostream>

#include <ModelManager.hpp>


class CLIModelManager {
public:
    const std::string modelPath;
    ModelManager manager;

    explicit CLIModelManager(const std::string& path);

    void loadModel(bool quiet, LLModelOptionsAsync options, FinishedCallback onDone);
    void sendMessage(bool quiet, std::string role, std::string text, bool hasSystemPrompt, std::string systemPrompt, TextGenOptions options);
    void clear();

private:
    static void onModelLoadProgress(const float progress);
    static void onModelLoadDone();
};