#pragma once
#include <ModelFactory.hpp>

class TermSDModelManager {
public:
    const std::string modelPath;
    ModelFactory factory;
    SDModelPtr model;

    explicit TermSDModelManager(const std::string& path);

    void loadModel(bool quiet, SDModelOptionsAsync options, FinishedCallback onDone);
    void createImage(bool quiet, bool showProgress, std::string positivePrompt, std::string negativePrompt, std::optional<std::string> filename, SDImageOptions options);
    SDModel* getSDM();

private:
    static void onModelLoadProgress(const float progress, const int step, const int steps);
    static void onModelLoadDone();
};