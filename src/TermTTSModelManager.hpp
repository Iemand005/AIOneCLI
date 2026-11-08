#pragma once
#include <ModelFactory.hpp>
#include <TTSModelOptions.h>

class TermTTSModelManager {
public:
    const std::string modelPath;
    const std::string mmprojPath;
    ModelFactory factory;
    TTSModelPtr model;

    explicit TermTTSModelManager(const std::string& path, const std::string& mmprojPath);

    void loadModel(bool quiet, TTSModelOptionsAsync options, ModelOptionsAsync mmprojOptions, FinishedCallback onDone);
    void synthesize(bool quiet, bool showProgress, std::string prompt, std::optional<std::string> filename, TTSSynthesisOptions options);
    TTSModel *getTTSM();

private:
    static void onModelLoadProgress(const float progress, const int step, const int steps);
    static void onModelLoadDone();
};