#pragma once
#include <ModelManager.hpp>


class TermLLModelManager {
public:
    const std::string modelPath;
    ModelManager manager;

    explicit TermLLModelManager(const std::string& path);

    void loadModel(bool quiet, LLModelOptionsAsync options, FinishedCallback onDone);
    void sendMessage(std::string role, std::string text, bool hasSystemPrompt, std::string systemPrompt, TextGenOptions options);
    void clear();

private:
    static void onModelLoadProgress(const float progress);
    static void onModelLoadDone();
};