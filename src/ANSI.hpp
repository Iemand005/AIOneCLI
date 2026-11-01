#pragma once
#include <string>

class ANSI {
public:
    static inline const std::string CARRIAGE_RETURN   = "\r";
    static inline const std::string CLEAR_LINE        = "\033[2K";

    static inline const std::string FG_BLACK          = "\033[30m";
    static inline const std::string FG_RED            = "\033[31m";
    static inline const std::string FG_GREEN          = "\033[32m";
    static inline const std::string FG_YELLOW         = "\033[33m";
    static inline const std::string FG_BLUE           = "\033[34m";
    static inline const std::string FG_MAGENTA        = "\033[35m";
    static inline const std::string FG_CYAN           = "\033[36m";
    static inline const std::string FG_WHITE          = "\033[37m";

    static inline const std::string FG_BRIGHT_BLACK   = "\033[90m";
    static inline const std::string FG_BRIGHT_RED     = "\033[91m";
    static inline const std::string FG_BRIGHT_GREEN   = "\033[92m";
    static inline const std::string FG_BRIGHT_YELLOW  = "\033[93m";
    static inline const std::string FG_BRIGHT_BLUE    = "\033[94m";
    static inline const std::string FG_BRIGHT_MAGENTA = "\033[95m";
    static inline const std::string FG_BRIGHT_CYAN    = "\033[96m";
    static inline const std::string FG_BRIGHT_WHITE   = "\033[97m";

    static inline const std::string BG_BLACK          = "\033[40m";
    static inline const std::string BG_RED            = "\033[41m";
    static inline const std::string BG_GREEN          = "\033[42m";
    static inline const std::string BG_YELLOW         = "\033[43m";
    static inline const std::string BG_BLUE           = "\033[44m";
    static inline const std::string BG_MAGENTA        = "\033[45m";
    static inline const std::string BG_CYAN           = "\033[46m";
    static inline const std::string BG_WHITE          = "\033[47m";

    static inline const std::string BG_BRIGHT_BLACK   = "\033[100m";
    static inline const std::string BG_BRIGHT_RED     = "\033[101m";
    static inline const std::string BG_BRIGHT_GREEN   = "\033[102m";
    static inline const std::string BG_BRIGHT_YELLOW  = "\033[103m";
    static inline const std::string BG_BRIGHT_BLUE    = "\033[104m";
    static inline const std::string BG_BRIGHT_MAGENTA = "\033[105m";
    static inline const std::string BG_BRIGHT_CYAN    = "\033[106m";
    static inline const std::string BG_BRIGHT_WHITE   = "\033[107m";

    static inline const std::string RESET             = "\033[0m";
    static inline const std::string BOLD              = "\033[1m";
    static inline const std::string UNDERLINE         = "\033[4m";
    static inline const std::string INVERSE           = "\033[7m";
};