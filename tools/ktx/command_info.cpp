// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"

#include <ktx.h>

#include <iostream>

#include "stdafx.h"


// -------------------------------------------------------------------------------------------------

namespace ktx {

// TODO KTX Tools P2: Proper doxygen documentation
// ktx info <options> <input_file>
//          --format: text | json | mini-json
//              Output format. Defaults to text.
class CommandInfo : public Command {
    struct Options {
        enum class OutputFormat {
            text,
            json,
            json_mini,
        };

        OutputFormat format = OutputFormat::text;
        _tstring inputFilepath;
    };

    Options options;

public:
    // TODO KTX Tools P5: Support --version
    // TODO KTX Tools P5: Support --help with proper usage
    void initializeOptions();
    virtual bool processOption(argparser& parser, int opt) override;
    void processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile);
    virtual int main(int argc, _TCHAR* argv[]) override;

    using Command::Command;
    virtual ~CommandInfo() {};

private:
    int printInfoText(const _tstring& infile);
    int printInfoJSON(const _tstring& infile, bool minified);
};

// -------------------------------------------------------------------------------------------------

void CommandInfo::initializeOptions() {
    option_list.emplace(option_list.begin(), "format", argparser::option::required_argument, nullptr, 'f');
    short_opts += "f:";
}

bool CommandInfo::processOption(argparser& parser, int opt) {
    switch (opt) {
    case 'f':
        if (parser.optarg == "text") {
            options.format = Options::OutputFormat::text;
        } else if (parser.optarg == "json") {
            options.format = Options::OutputFormat::json;
        } else if (parser.optarg == "mini-json") {
            options.format = Options::OutputFormat::json_mini;
        } else {
            // TODO KTX Tools P5: Print usage, Failure: unsupported format
            std::cerr << "Print usage, Failure: unsupported format" << std::endl;
            return false;
        }
        break;
    default:
        return false;
    }

    return true;
}

void CommandInfo::processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile) {
    if (infiles.empty()) {
        // TODO KTX Tools P5: Print usage, Failure: infile is missing
        std::cerr << "Print usage, Failure: infile is missing" << std::endl;
        // return EXIT_FAILURE;
        return;
    }
    if (infiles.size() > 1) {
        // TODO KTX Tools P5: Print usage, Failure: infiles.size() > 1
        std::cerr << "Print usage, Failure: infiles.size() > 1" << std::endl;
        return;
    }

    options.inputFilepath = infiles[0];
    (void) outfile;
}

int CommandInfo::main(int argc, _TCHAR* argv[]) {
    initializeOptions();
    processCommandLine(argc, argv, StdinUse::eDisallowStdin, OutfilePos::eNone);
    processPositional(genericOptions.infiles, genericOptions.outfile);

    switch (options.format) {
    case Options::OutputFormat::text:
        return printInfoText(options.inputFilepath);

    case Options::OutputFormat::json:
        return printInfoJSON(options.inputFilepath, false);

    case Options::OutputFormat::json_mini:
        return printInfoJSON(options.inputFilepath, true);
    }

    assert(false && "Internal error");
    return EXIT_FAILURE;
}

int CommandInfo::printInfoText(const _tstring& infile) {
    FILE* inf;

    // TODO KTX Tools P5: fclose?
#ifdef _WIN32
    _tfopen_s(&inf, infile.c_str(), "rb");
#else
    inf = _tfopen(infile.c_str(), "rb");
#endif

    if (!inf) {
        // TODO KTX Tools P5: Is strerror depricated?
        std::cerr << processName << ": Could not open input file \"" << infile << "\". " << strerror(errno) << std::endl;
        return 2;
    }

    KTX_error_code result;

    result = ktxPrintKTX2InfoTextForStdioStream(inf);
    if (result ==  KTX_FILE_UNEXPECTED_EOF) {
        std::cerr << processName << ": Unexpected end of file reading \"" << infile << "\"." << std::endl;
        return 2;

    } else if (result == KTX_UNKNOWN_FILE_FORMAT) {
        std::cerr << processName << ": " << infile << " is not a KTX2 file." << std::endl;
        return 2;

    } else if (result != KTX_SUCCESS) {
        std::cerr << processName << ": " << infile << " failed to process KTX2 file: ERROR_CODE " << result << std::endl;
        return 2;
    }

    return EXIT_SUCCESS;
}

int CommandInfo::printInfoJSON(const _tstring& infile, bool minified) {
    const char* space = minified ? "" : " ";
    const char* nl = minified ? "" : "\n";

    FILE* inf;

    // TODO KTX Tools P5: fclose?
#ifdef _WIN32
    _tfopen_s(&inf, infile.c_str(), "rb");
#else
    inf = _tfopen(infile.c_str(), "rb");
#endif

    if (!inf) {
        // TODO KTX Tools P5: Is strerror depricated?
        std::cerr << processName << ": Could not open input file \"" << infile << "\". " << strerror(errno) << std::endl;
        return 2;
    }

    KTX_error_code result;

    std::cout << "{" << nl;
    // TODO KTX Tools P5: ktx-schema-url-1.0 will has to be replaced with the actual URL
    std::cout << (minified ? "" : "    ") << "\"$id\":" << space << "\"ktx-schema-url-1.0\"," << nl;

    // TODO KTX Tools P4: Call validate JSON print and include "valid" and "messages" in the JSON output
    // result = validateAndPrintJSON(inf, 1, 4, minified);
    // std::cout << "," << nl;
    result = ktxPrintKTX2InfoJSONForStdioStream(inf, 1, 4, minified);

    if (result ==  KTX_FILE_UNEXPECTED_EOF) {
        std::cerr << processName << ": Unexpected end of file reading \"" << infile << "\"." << std::endl;
        return 2;

    } else if (result == KTX_UNKNOWN_FILE_FORMAT) {
        std::cerr << processName << ": " << infile << " is not a KTX2 file." << std::endl;
        return 2;

    } else if (result != KTX_SUCCESS) {
        std::cerr << processName << ": " << infile << " failed to process KTX2 file: ERROR_CODE " << result << std::endl;
        return 2;
    }

    std::cout << "}" << nl;
    return EXIT_SUCCESS;
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxInfo, ktx::CommandInfo)
