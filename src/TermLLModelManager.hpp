#pragma once
#include <ModelManager.hpp>


class TermLLModelManager {
public:
    const std::string modelPath;
    AIOne::ModelManager manager;

    explicit TermLLModelManager(const std::string& path);

    void loadModel(bool quiet, LLModelOptionsAsync options, FinishedCallback onDone);
    void sendMessage(std::string role, std::string text, bool hasSystemPrompt, std::string systemPrompt, TextGenOptions options);
    void clear();

private:
    static void onModelLoadProgress(const float progress, const int step, const int steps);
    static void onModelLoadDone();
};