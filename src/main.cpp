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

    argparse::ArgumentParser textCommand("text");
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
    imageCommand.add_argument("--step-count", "--steps", "-s")
        .help("amount of steps to generate image")
        .scan<'i', int>()
        .default_value(25);
    imageCommand.add_argument("--cfg-cale", "-cfg")
        .help("the classifier-free guidance scale")
        .scan<'g', float>()
        .default_value(6.0f);
    imageCommand.add_argument("-pos", "--positive")
        .help("the positive prompt for image generation (non-interactive mode)")
        .required();
    imageCommand.add_argument("-neg", "--negative")
        .help("the negative prompt for image generation (non-interactive mode)")
        .required();
    imageCommand.add_argument("--output-path", "--output", "--filename", "-o")
        .help("the output path (.png)")
        .required();
    imageCommand.add_argument("--recommended", "-r")
        .help("automatically applies --flash-attention")
        .default_value(false)
        .implicit_value(true);


    argparse::ArgumentParser program("AIOneCLI", AIONE_CLI_VERSION.c_str());
    program.add_subparser(imageCommand);
    program.add_argument("-m", "--model")
        .help("path to the model file")
        .required();
    program.add_argument("--mmap")
        .help("enables mmap for model loading")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--flash-attn", "--flash-attention")
        .help("enables flash attention for inference")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--threads", "--cores", "-t")
        .help("how many CPU threads to use for inference")
        .scan<'i', int>()
        .default_value(-1);
    program.add_argument("--temperature", "--temp")
        .help("set temperature for inference")
        .scan<'g', float>()
        .default_value(0.7f);
    program.add_argument("--seed")
        .help("set seed for inference")
        .scan<'i', int>();
    program.add_argument("--no-logo", "--quiet", "-q")
        .help("hides the startup logo and info")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    
    const bool imageMode = program.is_subcommand_used("image");
    if (!imageMode && !program.is_subcommand_used("text")) {
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


    argparse::ArgumentParser *localArgs = (imageMode ? &imageCommand : &textCommand);

    auto defaultVal = [&](std::string opt, auto minVal, auto defVal) {
        argparse::ArgumentParser *which = localArgs;

        if (
            opt == "--model"
            || opt == "--mmap"
            || opt == "--flash-attention"
            || opt == "--threads"
            || opt == "--temperature"
            || opt == "--seed"
            || opt == "--no-logo"
        ) {
            which = &program;
        }

        using T = decltype(minVal);
        T val = which->get<T>(opt);

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

    auto applyRecommended = [&](std::string opt) {
        if (recommended) {
            return true;
        }

        argparse::ArgumentParser *which = localArgs;

        if (opt == "--mmap") {
            which = &program;
        }

        return which->get<bool>(opt);
    };


    if (!imageMode) {
        // string to enum for PreferredDevice
        PreferredDevice device;
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

        
        TermLLModelManager modelManager(program.get<std::string>("--model"));

        if (!hasPrompt && !program.get<bool>("--no-logo")) {
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
        modelManager.loadModel(hasPrompt, options, [&]() {
            if (hasPrompt) {
                TextGenOptions textOpts;
                textOpts.maxTokens = defaultVal("--max-tokens", 0, -1);
                textOpts.minP = defaultVal("--min-p", 0.0f, 0.0f);
                textOpts.topK = defaultVal("--top-k", 0, 0);
                textOpts.temperature = defaultVal("--temperature", 0.0f, 0.0f);

                if (program.is_used("--seed")) {
                    textOpts.seed = program.get<int>("--seed");
                }

                std::string systemPrompt;
                bool hasSystemPrompt = false;
                if (localArgs->is_used("--system-prompt")) {
                    systemPrompt = localArgs->get<std::string>("--system-prompt");
                    hasSystemPrompt = true;
                }

                textOpts.onThinkStateChange = [](bool thinking) {
                    std::cout << (thinking ? "<think>" : "</think>");
                };
                
                textOpts.onToken = [](const std::string &token) {
                    std::cout << token;
                };

                modelManager.sendMessage(localArgs->get<std::string>("--role"), prompt, hasSystemPrompt, systemPrompt, textOpts);
                exitCli();
            } else {
                interactive(false);
            }
        });
    } else {
        TermSDModelManager modelManager(program.get<std::string>("--model"));
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

        if (!hasPrompts && !program.get<bool>("--no-logo")) {
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
        modelManager.loadModel(hasPrompts, options, [&]() {
            if (hasPrompts) {
                SDImageOptions imageOpts;
                imageOpts.stepCount = defaultVal("--step-count", 1, 1);
                imageOpts.cfgScale = defaultVal("--cfg-scale", 0, 0);

                if (program.is_used("--seed")) {
                    imageOpts.seed = program.get<int>("--seed");
                }

                modelManager.createImage(true, posPrompt, negPrompt, localArgs->present<std::string>("--output-path"), imageOpts);
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