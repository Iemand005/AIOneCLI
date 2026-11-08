#include "Main.hpp"
#include <iostream>
#include <limits>
#include <csignal>
#include <condition_variable>
#include <mutex>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <ModelManager.hpp>
#include <ModelFactory.hpp>
#include <argparse/argparse.hpp>
#include <hwinfo/hwinfo.h>
#include <ggml-backend.h>

#include "TermLLModelManager.hpp"
#include "TermSDModelManager.hpp"
#include "TermTTSModelManager.hpp"
#include "ANSI.hpp"
#include "Version.hpp"

std::condition_variable cliExitCV;
std::mutex cliExitMutex;
bool cliExitBool = false;


void exitCli() {
    std::lock_guard<std::mutex> lock(cliExitMutex);
    cliExitBool = true;
    cliExitCV.notify_all();
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        exitCli();
    }
}

void printLogo() {
    // get available backends
    size_t backendCount = ggml_backend_reg_count();
    std::string backends = "";

    for (size_t i = 0; i < backendCount; i++) {
        const char *name = ggml_backend_reg_name(ggml_backend_reg_get(i));

        if (backends != "") {
            backends += ", ";
        }
        
        backends += name;
    }

    std::cout
        << std::endl
        << ANSI::RESET << "   █████╗  ██╗  ██╗" << std::endl
        << ANSI::RESET << "  ██╔══██╗ ██║ ███║  " << ANSI::FG_BLACK << "\\" << std::endl
        << ANSI::RESET << "  ███████║ ██║ ╚██║  " << ANSI::FG_BLACK << "|> " << ANSI::FG_WHITE << ANSI::BOLD << "AIOne CLI v" << AIONE_CLI_VERSION << std::endl
        << ANSI::RESET << "  ██╔══██║ ██║  ██║  " << ANSI::FG_BLACK << "|> " << ANSI::FG_WHITE << "Available backends: " << backends << std::endl
        << ANSI::RESET << "  ██║  ██║ ██║  ██║  " << ANSI::FG_BLACK << "/" << std::endl
        << ANSI::RESET << "  ╚═╝  ╚═╝ ╚═╝  ╚═╝" << std::endl
        << std::endl;
}

// forward declare
void interactive(bool imageMode);


int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    
#ifdef _WIN32
    // set to UTF-8 cuz Windows doesn't do that by default ofc (for ASCII art)
    SetConsoleOutputCP(CP_UTF8);
#endif

    auto applyDefaultArgs = [](argparse::ArgumentParser &parser) {
        parser.add_argument("-m", "--model")
            .help("path to the model file")
            .required();
        parser.add_argument("--flash-attn", "--flash-attention")
            .help("enables flash attention for inference")
            .default_value(false)
            .implicit_value(true);
        parser.add_argument("--threads", "--cores", "-t")
            .help("how many CPU threads to use for inference")
            .scan<'i', int>()
            .default_value(-1);
        parser.add_argument("--seed")
            .help("set seed for inference")
            .scan<'i', int>();
        parser.add_argument("--no-logo", "--quiet", "-q")
            .help("hides the startup logo and info")
            .default_value(false)
            .implicit_value(true);
    };

    argparse::ArgumentParser textCommand("text");
    applyDefaultArgs(textCommand);
    textCommand.add_argument("--mmap")
        .help("enables mmap for model loading")
        .default_value(false)
        .implicit_value(true);
    textCommand.add_argument("--temperature", "--temp")
        .help("set temperature for inference")
        .scan<'g', float>()
        .default_value(0.7f);
    textCommand.add_argument("-d", "--device")
        .help("preferred device to use for inference")
        .choices("any", "cpu", "igpu", "integrated-gpu", "dgpu", "dedicated-gpu", "accel", "accelerator")
        .default_value(std::string("any"));
    textCommand.add_argument("--gpu-layers", "--offload-layers", "--offload", "-ngl", "-l")
        .help("how many layers to offload to gpu, -1 for all")
        .scan<'i', int>()
        .default_value(-1);
    textCommand.add_argument("--context-size", "--context", "-ctx")
        .help("the context size for the model")
        .scan<'i', int>()
        .default_value(1024);
    textCommand.add_argument("--batch-size", "--batch", "-bs")
        .help("evaluation batch size for prompt processing")
        .scan<'i', int>()
        .default_value(512);
    textCommand.add_argument("--max-tokens", "-max")
        .help("how many tokens the LLM can output")
        .scan<'i', int>()
        .default_value(4096);
    textCommand.add_argument("--min-p")
        .help("set min p for inference")
        .scan<'g', float>()
        .default_value(0.95f);
    textCommand.add_argument("--top-k")
        .help("set top k for inference")
        .scan<'i', int>()
        .default_value(40);
    textCommand.add_argument("--system-prompt", "-sys")
        .help("set system prompt for inference");
    textCommand.add_argument("--role")
        .help("set role for user messages")
        .default_value("user");
    textCommand.add_argument("--recommended", "-r")
        .help("automatically applies --mmap and --flash-attention")
        .default_value(false)
        .implicit_value(true);
    textCommand.add_argument("prompt")
        .help("the prompt to send (non-interactive mode)")
        .remaining();


    argparse::ArgumentParser imageCommand("image");
    applyDefaultArgs(imageCommand);
    imageCommand.add_argument("--mmap")
        .help("enables mmap for model loading")
        .default_value(false)
        .implicit_value(true);
    imageCommand.add_argument("--temperature", "--temp")
        .help("set temperature for inference")
        .scan<'g', float>()
        .default_value(0.7f);
    imageCommand.add_argument("--backend", "-b")
        .help("preferred backend to use for inference (cpu, gpu, cuda0, vulkan0, metal)")
        .default_value(std::string(""));
    imageCommand.add_argument("--vae")
        .help("path to the vae file");
    imageCommand.add_argument("--clip-g", "-cg")
        .help("path to the clip_g file");
    imageCommand.add_argument("--clip-l", "-cl")
        .help("path to the clip_l file");
    imageCommand.add_argument("--t5xxl", "-t5")
        .help("path to the t5xxl file");
    imageCommand.add_argument("--step-count", "--steps", "-s")
        .help("amount of steps to generate image")
        .scan<'i', int>()
        .default_value(25);
    imageCommand.add_argument("--cfg-scale", "-cfg")
        .help("the classifier-free guidance scale")
        .scan<'g', float>()
        .default_value(6.0f);
    imageCommand.add_argument("--width", "-w")
        .help("resulting image width")
        .scan<'i', int>()
        .default_value(512);
    imageCommand.add_argument("--height", "-h")
        .help("resulting image height")
        .scan<'i', int>()
        .default_value(512);
    imageCommand.add_argument("--positive", "-pos")
        .help("the positive prompt for image generation (non-interactive mode)");
    imageCommand.add_argument("--negative", "-neg")
        .help("the negative prompt for image generation (non-interactive mode)");
    imageCommand.add_argument("--progress")
        .help("shows progress in non-interactive mode")
        .default_value(false)
        .implicit_value(true);
    imageCommand.add_argument("--preview")
        .help("generates and saves a preview every step in non-interactive mode")
        .choices("none", "vae", "vae-noise", "tae", "tae-noise", "proj", "proj-noise")
        .default_value(std::string("none"));
    imageCommand.add_argument("--output-path", "--output", "--filename", "-o")
        .help("the output path (.png)")
        .required();
    imageCommand.add_argument("--recommended", "-r")
        .help("automatically applies --mmap and --flash-attention")
        .default_value(false)
        .implicit_value(true);


    argparse::ArgumentParser ttsCommand("tts");
    applyDefaultArgs(ttsCommand);
    ttsCommand.add_argument("-d", "--device")
        .help("preferred device to use for inference")
        .choices("any", "cpu", "igpu", "integrated-gpu", "dgpu", "dedicated-gpu", "accel", "accelerator")
        .default_value(std::string("any"));
    ttsCommand.add_argument("--mmproj", "-mm")
        .help("path to the mmproj file")
        .required();
    ttsCommand.add_argument("--voice-name", "-v")
        .help("name of the built-in voice");
    ttsCommand.add_argument("--voice-index", "-vi")
        .help("index of the built-in voice")
        .scan<'i', int>()
        .default_value(0);
    ttsCommand.add_argument("--voice-file", "-vf")
        .help("path to the reference voice file");
    ttsCommand.add_argument("--batch-size", "--batch", "-bs")
        .help("evaluation batch size for prompt processing")
        .scan<'i', int>()
        .default_value(512);
    ttsCommand.add_argument("--max-tokens", "-max")
        .help("how many tokens the model can generate")
        .scan<'i', int>()
        .default_value(4096);
    ttsCommand.add_argument("--top-p")
        .help("set top p for inference")
        .scan<'g', float>()
        .default_value(1.0f);
    ttsCommand.add_argument("--top-k")
        .help("set top k for inference")
        .scan<'i', int>()
        .default_value(50);
    ttsCommand.add_argument("--lang")
        .help("set language of text")
        .default_value(std::string(""));
    ttsCommand.add_argument("--progress")
        .help("shows progress in non-interactive mode")
        .default_value(false)
        .implicit_value(true);
    // ttsCommand.add_argument("--preview")
    //     .help("generates and saves a preview every frame in non-interactive mode")
    //     .default_value(false)
    //     .implicit_value(true);
    ttsCommand.add_argument("--output-path", "--output", "--filename", "-o")
        .help("the output path (.wav)")
        .required();
    ttsCommand.add_argument("--recommended", "-r")
        .help("automatically applies --flash-attention")
        .default_value(false)
        .implicit_value(true);
#ifdef AIONE_ENABLE_TTS_INSTRUCT
    ttsCommand.add_argument("--instruction", "--instruct", "-i")
        .help("set instruction for voice generation or modification")
        .default_value(std::string(""));
#endif
    ttsCommand.add_argument("prompt")
        .help("the prompt to send (non-interactive mode)")
        .remaining();


    argparse::ArgumentParser program("AIOneCLI", AIONE_CLI_VERSION.c_str());
    program.add_subparser(textCommand);
    program.add_subparser(imageCommand);
    program.add_subparser(ttsCommand);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    
    const bool textMode = program.is_subcommand_used("text");
    const bool imageMode = program.is_subcommand_used("image");
    const bool ttsMode = program.is_subcommand_used("tts");
    if (!ttsMode && !imageMode && !textMode) {
        std::cout << program << std::endl;
        return 1;
    }

#ifndef AIONE_ENABLE_STABLE_DIFFUSION
    if (imageMode) {
        std::cerr
            << "AIONE_ENABLE_STABLE_DIFFUSION = OFF" << std::endl
            << "Stable diffusion support disabled for this build!" << std::endl;
        return 2;
    }
#endif

#ifndef AIONE_ENABLE_TTS
    if (ttsMode) {
        std::cerr
            << "AIONE_ENABLE_TTS = OFF" << std::endl
            << "TTS support disabled for this build!" << std::endl;
        return 2;
    }
#endif


    argparse::ArgumentParser *localArgs = (ttsMode ? &ttsCommand : (imageMode ? &imageCommand : &textCommand));

    auto defaultVal = [&localArgs](std::string opt, auto minVal, auto defVal) {
        using T = decltype(minVal);
        T val = localArgs->get<T>(opt);

        return val < minVal ? defVal : val;
    };


    // get physical cpu cores and else default to 6
    short cpuCores = defaultVal("--threads", 1, 0);

    if (cpuCores == 0) {
        auto cpus = hwinfo::cpus();
        if (cpus) {
            for (const auto &cpu : *cpus) {
                cpuCores += cpu.physical_cores;
            }
        }

        if (cpuCores == 0) {
            cpuCores = 6;
        }
    }


    bool recommended = localArgs->get<bool>("--recommended");

    auto applyRecommended = [recommended, &localArgs](std::string opt) {
        if (recommended) {
            return true;
        }

        return localArgs->get<bool>(opt);
    };

    PreferredDevice device;
    if (imageMode || ttsMode) {
        // string to enum for PreferredDevice
        std::string deviceStr = localArgs->get<std::string>("--device");
        if (deviceStr == "cpu") {
            device = PreferredDevice::CPU;
        } else if (deviceStr == "igpu" || deviceStr == "integrated-gpu") {
            device = PreferredDevice::IGPU;
        } else if (deviceStr == "dgpu" || deviceStr == "dedicated-gpu") {
            device = PreferredDevice::DGPU;
        } else if (deviceStr == "accel" || deviceStr == "accelerator") {
            device = PreferredDevice::ACCELERATOR;
        } else {
            device = PreferredDevice::ANY;
        }
    }

    if (textMode) {
        std::string prompt = "";
        bool hasPrompt = false;

        try {
            auto rest = localArgs->get<std::vector<std::string>>("prompt");
            
            for (const auto &arg : rest) {
                if (prompt != "") {
                    prompt += " ";
                }
                
                prompt += arg;
            }

            hasPrompt = true;
        } catch (const std::logic_error &e) {
            // no prompt
        }

        
        TermLLModelManager modelManager(localArgs->get<std::string>("--model"));

        if (!hasPrompt && !localArgs->get<bool>("--no-logo")) {
            printLogo();
        }
        
        LLModelOptionsAsync options;
        options.device = device;
        options.offloadLayers = defaultVal("--gpu-layers", 0, -1);
        options.useMmap = applyRecommended("--mmap");
        // options.threadCount = cpuCores; // TODO ambigious gotta fix later
        options.flashAttention = applyRecommended("--flash-attention");
        options.contextLength = defaultVal("--context-size", 1, 1024);
        options.evalBatchSize = defaultVal("--batch-size", 1, 512);
        modelManager.loadModel(hasPrompt, options, [hasPrompt, &defaultVal, &localArgs, &modelManager, prompt]() {
            if (hasPrompt) {
                TextGenOptions textOpts;
                textOpts.maxTokens = defaultVal("--max-tokens", 0, -1);
                textOpts.minP = defaultVal("--min-p", 0.0f, 0.0f);
                textOpts.topK = defaultVal("--top-k", 0, 0);
                textOpts.temperature = defaultVal("--temperature", 0.0f, 0.0f);

                if (localArgs->is_used("--seed")) {
                    textOpts.seed = localArgs->get<int>("--seed");
                }

                std::string systemPrompt;
                bool hasSystemPrompt = false;
                if (localArgs->is_used("--system-prompt")) {
                    systemPrompt = localArgs->get<std::string>("--system-prompt");
                    hasSystemPrompt = true;
                }

                textOpts.onThinkStateChange = [](bool thinking) {
                    std::cout << (thinking ? "<think>" : "</think>") << std::flush;
                };
                
                textOpts.onToken = [](const std::string &token) {
                    std::cout << token << std::flush;
                };

                modelManager.sendMessage(localArgs->get<std::string>("--role"), prompt, hasSystemPrompt, systemPrompt, textOpts);
                exitCli();
            } else {
                interactive(false);
            }
        });
    } else if (imageMode) {
        // string to enum for SDPreviewMode
        SDPreviewMode previewMode;
        std::string previewStr = localArgs->get<std::string>("--preview");
        bool previewNoise = previewStr.ends_with("-noise");
        
        if (previewStr.starts_with("vae")) {
            previewMode = SDPreviewMode::VAE;
        } else if (previewStr.starts_with("tae")) {
            previewMode = SDPreviewMode::TAE;
        } else if (previewStr.starts_with("proj")) {
            previewMode = SDPreviewMode::Proj;
        } else {
            previewMode = SDPreviewMode::None;
        }
        

        TermSDModelManager modelManager(localArgs->get<std::string>("--model"));
        std::string posPrompt = "";
        std::string negPrompt = "";
        bool hasPrompts = false;

        if (localArgs->is_used("--positive")) {
            posPrompt = localArgs->get<std::string>("--positive");
            hasPrompts = true;
        }

        if (localArgs->is_used("--negative")) {
            negPrompt = localArgs->get<std::string>("--negative");
            hasPrompts = true;
        }

        if (!hasPrompts && !localArgs->get<bool>("--no-logo")) {
            ggml_time_init();
            
            if (!ggml_backend_reg_count()) {
                ggml_backend_load_all();
            }

            printLogo();
        }
        
        SDModelOptionsAsync options;
        options.useMmap = applyRecommended("--mmap");
        options.threadCount = cpuCores;
        options.flashAttention = applyRecommended("--flash-attention");
        options.backend = localArgs->get<std::string>("--backend");

        if (localArgs->is_used("--vae")) {
            options.vaePath = localArgs->get<std::string>("--vae");
        }
        
        if (localArgs->is_used("--clip-g")) {
            options.clipGPath = localArgs->get<std::string>("--clip-g");
        }
        
        if (localArgs->is_used("--clip-l")) {
            options.clipLPath = localArgs->get<std::string>("--clip-l");
        }
        
        if (localArgs->is_used("--t5xxl")) {
            options.t5xxlPath = localArgs->get<std::string>("--t5xxl");
        }

        modelManager.loadModel(hasPrompts, options, [hasPrompts, &defaultVal, previewMode, previewNoise, &localArgs, posPrompt, negPrompt, &modelManager]() {
            if (hasPrompts) {
                SDImageOptions imageOpts;
                imageOpts.stepCount = defaultVal("--step-count", 1, 1);
                imageOpts.cfgScale = defaultVal("--cfg-scale", 0.0f, 0.0f);
                imageOpts.width = defaultVal("--width", 64, 64);
                imageOpts.height = defaultVal("--height", 64, 64);

                if (localArgs->is_used("--seed")) {
                    imageOpts.seed = localArgs->get<int>("--seed");
                }

                modelManager.createImage(true, (!hasPrompts || localArgs->get<bool>("--progress")), previewMode, previewNoise, posPrompt, negPrompt, localArgs->present<std::string>("--output-path"), imageOpts);
                exitCli();
            } else {
                interactive(true);
            }
        });
    } else if (ttsMode) {
        std::string prompt = "";
        bool hasPrompt = false;

        try {
            auto rest = localArgs->get<std::vector<std::string>>("prompt");
            
            for (const auto &arg : rest) {
                if (prompt != "") {
                    prompt += " ";
                }
                
                prompt += arg;
            }

            hasPrompt = true;
        } catch (const std::logic_error &e) {
            // no prompt
        }


        TermTTSModelManager modelManager(localArgs->get<std::string>("--model"), localArgs->get<std::string>("--mmproj"));

        if (!hasPrompt && !localArgs->get<bool>("--no-logo")) {
            ggml_time_init();
            
            if (!ggml_backend_reg_count()) {
                ggml_backend_load_all();
            }

            printLogo();
        }
        
        TTSModelOptionsAsync options;
        options.device = device;
        options.threadCount = cpuCores;
        options.flashAttention = applyRecommended("--flash-attention");

        modelManager.loadModel(hasPrompt, options, {}, [hasPrompt, &defaultVal, &localArgs, prompt, &modelManager]() {
            if (hasPrompt) {
                TTSSynthesisOptions ttsOpts;
                ttsOpts.maxTokens = defaultVal("--max-tokens", 0, -1);
                ttsOpts.topP = defaultVal("--top-p", 0.0f, 0.0f);
                ttsOpts.topK = defaultVal("--top-k", 0, 0);
                ttsOpts.lang = localArgs->get<std::string>("--lang");
                ttsOpts.batchSize = defaultVal("--batch-size", 1, 512);
                
                if (localArgs->is_used("--voice-file")) {
                    ttsOpts.voice.referenceFile = localArgs->get<std::string>("--voice-file");
                } else if (localArgs->is_used("--voice-name")) {
                    ttsOpts.voice.builtInName = localArgs->get<std::string>("--voice-name");
                } else {
                    ttsOpts.voice.builtInIndex = localArgs->get<int>("--voice-index");
                }

                if (localArgs->is_used("--seed")) {
                    ttsOpts.seed = localArgs->get<int>("--seed");
                }
                
#ifdef AIONE_ENABLE_TTS_INSTRUCT
                if (localArgs->is_used("--instruction")) {
                    ttsOpts.instruction = localArgs->get<std::string>("--instruction");
                }
#endif

                modelManager.synthesize(true, (!hasPrompt || localArgs->get<bool>("--progress")), prompt, localArgs->present<std::string>("--output-path"), ttsOpts);
                exitCli();
            } else {
                interactive(true);
            }
        });
    }


    // wait until CTRL+C is pressed to exit
    // OR prompt is done in non-interactive mode
    {
        std::unique_lock<std::mutex> lock(cliExitMutex);
        cliExitCV.wait(lock, []{
            return cliExitBool;
        });
    }

    std::cout << ANSI::RESET << std::endl;

    return 0;
}

void interactive(bool imageMode) {
    // TODO interactive cli

    std::cout
        << std::endl
        << ANSI::BG_BLACK << ANSI::FG_BRIGHT_BLACK << ANSI::CLEAR_LINE << "> " << ANSI::FG_CYAN << ANSI::BOLD << "Hello world!";
}