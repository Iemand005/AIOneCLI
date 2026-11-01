#pragma once
#include <iostream>

#include <ModelManager.hpp>


class CLIModelManager {
public:
    const std::string modelPath;
    ModelManager manager;

    explicit CLIModelManager(const std::string& path);

    void loadModel(LLModelOptionsAsync options, FinishedCallback onDone);

private:
    static void onModelLoadProgress(const float progress);
    static void onModelLoadDone();
};