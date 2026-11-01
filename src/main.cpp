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

#include "CLIModelManager.hpp"
#include "ANSI.hpp"
#include "Version.hpp"

std::condition_variable cliExitCV;
std::mutex cliExitMutex;
bool cliExitBool = false;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::lock_guard<std::mutex> lock(cliExitMutex);
        cliExitBool = true;
        cliExitCV.notify_all();
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    
#ifdef _WIN32
    // set to UTF-8 cuz Windows doesn't do that by default ofc (for ASCII art)
    SetConsoleOutputCP(CP_UTF8);
#endif


    argparse::ArgumentParser program("AIOneCLI", AIONE_CLI_VERSION.c_str());

    program.add_argument("-m", "--model")
        .help("path to the model file")
        .required();
    program.add_argument("-d", "--device")
        .help("preferred device to use for inference")
        .choices("any", "cpu", "igpu", "integrated-gpu", "dgpu", "dedicated-gpu", "accel", "accelerator")
        .default_value(std::string("any"));
    program.add_argument("--mmap")
        .help("enables mmap for model loading")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--flash-attn", "--flash-attention")
        .help("enables flash attention for inference")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--gpu-layers", "--offload-layers", "--offload", "-ngl", "-l")
        .help("how many layers to offload to gpu, -1 for all")
        .scan<'i', int>()
        .default_value(-1);
    program.add_argument("--threads", "--cores", "-t")
        .help("how many CPU threads to use for inference")
        .scan<'i', int>()
        .default_value(-1);
    program.add_argument("--context-size", "--context", "-ctx")
        .help("the context size for the model")
        .scan<'i', int>()
        .default_value(1024);
    program.add_argument("--batch-size", "--batch", "-bs")
        .help("evaluation batch size for prompt processing")
        .scan<'i', int>()
        .default_value(512);
    program.add_argument("--recommended", "-r")
        .help("automatically applies --mmap and --flash-attention")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--no-logo", "--quiet", "-q")
        .help("hides the startup logo and info")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("rest")
        .remaining();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    
    bool recommended = program.get<bool>("--recommended");

    auto applyRecommended = [&](std::string opt) {
        return program.get<bool>(opt) || recommended;
    };

    auto defaultVal = [&](std::string opt, int minVal, int defVal) {
        int val = program.get<int>(opt);

        return val < minVal ? defVal : val;
    };


    // string to enum for PreferredDevice
    PreferredDevice device;
    std::string deviceStr = program.get<std::string>("--device");
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


    std::string prompt = "";

    try {
        auto rest = program.get<std::vector<std::string>>("rest");
        
        for (const auto &arg : rest) {
            if (prompt != "") {
                prompt += " ";
            }
            
            prompt += arg;
        }
    } catch (const std::logic_error &e) {
        // no prompt
    }


    CLIModelManager modelManager(program.get<std::string>("--model"));

    if (!program.get<bool>("--no-logo")) {
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
    
    LLModelOptionsAsync options;
    options.device = device;
    options.offloadLayers = defaultVal("--gpu-layers", 0, -1);
    options.useMmap = applyRecommended("--mmap");
    // options.threadCount = cpuCores; // TODO ambigious gotta fix later
    options.flashAttention = applyRecommended("--flash-attention");
    options.contextLength = defaultVal("--context-size", 1, 1024);
    options.evalBatchSize = defaultVal("--batch-size", 1, 512);
    modelManager.loadModel(options, [&prompt]() {
        
    });


    // wait until CTRL+C is pressed to exit
    {
        std::unique_lock<std::mutex> lock(cliExitMutex);
        cliExitCV.wait(lock, []{
            return cliExitBool;
        });
    }

    return 0;
}