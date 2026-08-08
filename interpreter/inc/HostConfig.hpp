#ifndef HOST_CONFIG_HPP
#define HOST_CONFIG_HPP

#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace litecode {

struct CliOptions {
    bool dumpTokens = false;
    bool dumpAst = false;
    bool structuredDiagnostics = false;
    size_t replAutoResetEvery = 0;
    size_t replMaxBatches = 32;
    std::string scriptPath;
    bool showHelp = false;
    bool showVersion = false;
    bool hasError = false;
    std::string errorMessage;
};

inline std::string trimOptionValue(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t')) {
        ++start;
    }
    std::size_t end = value.size();
    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t')) {
        --end;
    }
    return value.substr(start, end - start);
}

inline bool parseOptionSizeValue(const std::string& value, size_t& out) {
    if (value.empty()) {
        return false;
    }
    std::string trimmed = trimOptionValue(value);
    if (trimmed.empty()) {
        return false;
    }
    char* end = nullptr;
    unsigned long long parsed = std::strtoull(trimmed.c_str(), &end, 10);
    if (end == trimmed.c_str() || *end != '\0') {
        return false;
    }
    out = static_cast<size_t>(parsed);
    return true;
}

inline CliOptions parseCliArguments(int argc, char** argv) {
    CliOptions options;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--help") {
            options.showHelp = true;
            continue;
        }
        if (arg == "--version") {
            options.showVersion = true;
            continue;
        }
        if (arg == "--dump-tokens") {
            options.dumpTokens = true;
            continue;
        }
        if (arg == "--dump-ast") {
            options.dumpAst = true;
            continue;
        }
        if (arg == "--diagnostics=structured") {
            options.structuredDiagnostics = true;
            continue;
        }
        if (arg == "--diagnostics=legacy") {
            options.structuredDiagnostics = false;
            continue;
        }
        if (arg.rfind("--repl-auto-reset=", 0) == 0) {
            std::string value = arg.substr(std::string("--repl-auto-reset=").size());
            size_t parsed = 0;
            if (!parseOptionSizeValue(value, parsed)) {
                options.hasError = true;
                options.errorMessage = "Invalid value for --repl-auto-reset";
                return options;
            }
            options.replAutoResetEvery = parsed;
            continue;
        }
        if (arg.rfind("--repl-max-batches=", 0) == 0) {
            std::string value = arg.substr(std::string("--repl-max-batches=").size());
            size_t parsed = 0;
            if (!parseOptionSizeValue(value, parsed)) {
                options.hasError = true;
                options.errorMessage = "Invalid value for --repl-max-batches";
                return options;
            }
            options.replMaxBatches = parsed;
            continue;
        }
        if (arg.rfind("--", 0) == 0) {
            options.hasError = true;
            options.errorMessage = "Unknown option: " + arg;
            return options;
        }

        if (!options.scriptPath.empty()) {
            options.hasError = true;
            options.errorMessage = "Too many positional arguments";
            return options;
        }
        options.scriptPath = arg;
    }

    if (options.showHelp || options.showVersion) {
        return options;
    }

    if (options.scriptPath.empty()) {
        return options;
    }

    return options;
}

enum class DiagnosticStage {
    Parse,
    Resolve,
    Runtime
};

inline bool structuredDiagnosticsEnabled() {
    const char* env = std::getenv("LITECODE_STRUCTURED_DIAGNOSTICS");
    if (env == nullptr || std::string(env).empty()) {
        return false;
    }
    return std::string(env) != "0";
}

inline std::string formatDiagnostic(DiagnosticStage stage,
                                    int line,
                                    const std::string& location,
                                    const std::string& message) {
    std::ostringstream out;
    const char* stageName = "unknown";
    switch (stage) {
        case DiagnosticStage::Parse: stageName = "parse"; break;
        case DiagnosticStage::Resolve: stageName = "resolve"; break;
        case DiagnosticStage::Runtime: stageName = "runtime"; break;
    }

    out << "[" << stageName << "] "
        << "[line " << line << "]"
        << " at '" << location << "': "
        << message;
    return out.str();
}

inline size_t parsePositiveSizeEnv(const char* envVarName,
                                  size_t defaultValue,
                                  size_t upperBound) {
    if (envVarName == nullptr) {
        return defaultValue;
    }

    const char* raw = std::getenv(envVarName);
    if (raw == nullptr || std::string(raw).empty()) {
        return defaultValue;
    }

    try {
        unsigned long long parsed = std::strtoull(raw, nullptr, 10);
        if (parsed == 0ULL) {
            return defaultValue;
        }
        if (parsed > static_cast<unsigned long long>(upperBound)) {
            return upperBound;
        }
        return static_cast<size_t>(parsed);
    } catch (...) {
        return defaultValue;
    }
}

inline std::string replHelpText() {
    return
        "Litecode REPL commands:\n"
        "  .help   Show this help\n"
        "  .reset  Clear the current interpreter state\n"
        "  .quit   Exit the REPL\n";
}

inline std::string usageText() {
    return
        "Usage: litecode [script.lox]\n"
        "       litecode --help\n"
        "       litecode --version\n"
        "       litecode [--dump-tokens] [--dump-ast] [--diagnostics=structured|legacy]\n"
        "                [--repl-auto-reset=N] [--repl-max-batches=N] [script.lox]\n\n"
        "Litecode runs a script file or starts an interactive REPL when no file is provided.\n"
        "Use --help to show this message.\n";
}

inline std::string versionText() {
    return
        "Litecode version 0.1.0\n"
        "A compact interpreter built from the ground up in C++.\n";
}

}  // namespace litecode

#endif  // HOST_CONFIG_HPP
