#ifndef HOST_CONFIG_HPP
#define HOST_CONFIG_HPP

#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace litecode {

/**
 * @brief Parsed command-line and environment configuration for the CLI.
 *
 * The host config keeps runtime switches in one place so the driver can decide
 * between script mode, REPL mode, diagnostic output, and the bytecode VM path.
 */
struct CliOptions {
    bool dumpTokens = false;
    bool dumpAst = false;
    /** Parsed by the CLI; current diagnostic selection reads the environment directly. */
    bool structuredDiagnostics = false;
    bool useBytecode = false;
    size_t replAutoResetEvery = 0;
    /** Parsed by the CLI; current REPL batch limit is read from the environment directly. */
    size_t replMaxBatches = 32;
    std::string scriptPath;
    bool showHelp = false;
    bool showVersion = false;
    bool hasError = false;
    std::string errorMessage;
};

/** @brief Removes only surrounding spaces and tabs from a command-line value. */
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

/**
 * @brief Parses a complete decimal option value into the supplied size type.
 * @return False for empty, non-decimal, or trailing-junk input; otherwise stores a cast result.
 *
 * This helper does not impose a policy-specific upper bound or check whether the
 * parsed unsigned-long-long value fits in `size_t` before casting. On failure,
 * `out` is left unchanged.
 */
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

/**
 * @brief Parses CLI arguments and validates project-specific options.
 * @param argc Argument count passed to the process entrypoint.
 * @param argv Argument vector passed to the process entrypoint.
 * @return Normalized runtime configuration with any parse errors recorded.
 *
 * This function intentionally keeps the top-level command surface small: it
 * handles the script path, REPL toggles, debug output, and bytecode selection.
 * Invalid or unknown options stop parsing and are returned as `hasError`; help
 * and version suppress later validation because they exit before execution.
 */
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
        if (arg == "--vm" || arg == "--bytecode") {
            options.useBytecode = true;
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

/**
 * @brief Reads the diagnostic-format switch from `LITECODE_STRUCTURED_DIAGNOSTICS`.
 *
 * Missing, empty, or exactly `0` selects legacy output; any other nonempty value
 * enables structured diagnostics. This is environment-driven rather than taken
 * from the parsed CLI flag in the current runtime path.
 */
inline bool structuredDiagnosticsEnabled() {
    const char* env = std::getenv("LITECODE_STRUCTURED_DIAGNOSTICS");
    if (env == nullptr || std::string(env).empty()) {
        return false;
    }
    return std::string(env) != "0";
}

/** @brief Formats one diagnostic in the shared structured representation. */
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

/**
 * @brief Reads a positive decimal environment setting with default and upper-bound policy.
 *
 * Missing/empty values and parsed zero use `defaultValue`; values above the bound
 * clamp to `upperBound`. `strtoull` is called without an end pointer, so trailing
 * nonnumeric text is not rejected by this implementation. A leading minus sign
 * is accepted by `strtoull` as a large unsigned value and will normally clamp to
 * the supplied upper bound.
 */
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

/** @brief Returns the help text shared by the REPL `.help` command. */
inline std::string replHelpText() {
    return
        "Litecode REPL commands:\n"
        "  .help   Show this help\n"
        "  .reset  Clear the current interpreter state\n"
        "  .quit   Exit the REPL\n";
}

/** @brief Returns command-line usage text for help and invalid-option output. */
inline std::string usageText() {
    return
        "Usage: litecode [script.lox]\n"
        "       litecode --help\n"
        "       litecode --version\n"
        "       litecode [--dump-tokens] [--dump-ast] [--vm] [--diagnostics=structured|legacy]\n"
        "                [--repl-auto-reset=N] [--repl-max-batches=N] [script.lox]\n\n"
        "Litecode runs a script file or starts an interactive REPL when no file is provided.\n"
        "Use --vm to execute via the bytecode VM bootstrap path.\n"
        "Use --help to show this message.\n";
}

/** @brief Returns the product version banner. */
inline std::string versionText() {
    return
        "Litecode version 0.1.0\n"
        "A compact interpreter built from the ground up in C++.\n";
}

}  // namespace litecode

#endif  // HOST_CONFIG_HPP
