// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"

#include <ktx.h>

#include <iostream>

#include "stdafx.h"
#include "argparser.h"


// -------------------------------------------------------------------------------------------------

namespace ktx {

// TODO: Proper doxygen documentation
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
    // TODO: Support --version
    // TODO: Support --help with proper usage
    virtual void initializeOptions(std::vector<argparser::option>& long_opts, _tstring& short_opts) override;
    virtual bool processOption(argparser& parser, int opt) override;
    virtual void processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile) override;
    virtual int main(int argc, _TCHAR* argv[]) override;

    inline CommandInfo() noexcept : Command(false) {}
    virtual ~CommandInfo() {};

private:
    int printInfo(const _tstring& infile);
};

std::unique_ptr<Command> createCommandInfo() {
    return std::make_unique<CommandInfo>();
}

// -------------------------------------------------------------------------------------------------

void CommandInfo::initializeOptions(std::vector<argparser::option>& long_opts, _tstring& short_opts) {
    long_opts.emplace(long_opts.begin(), "format", argparser::option::required_argument, nullptr, 'f');
    short_opts += "f:";
}

bool CommandInfo::processOption(argparser& parser, int opt) {
    switch (opt) {
    case 'f':
        if (parser.optarg == "text") {
            options.format = Options::OutputFormat::text;
        } else if (parser.optarg == "json") {
            options.format = Options::OutputFormat::json;
        } else if (parser.optarg == "json-mini") {
            options.format = Options::OutputFormat::json_mini;
        } else {
            // TODO: Print usage, Failure: unsupported format
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
        // TODO: Print usage, Failure: infile is missing
        std::cerr << "Print usage, Failure: infile is missing" << std::endl;
        // return EXIT_FAILURE;
        return;
    }

    options.inputFilepath = infiles[0];
    (void) outfile;
}

int CommandInfo::main(int argc, _TCHAR* argv[]) {
    (void) argc;
    (void) argv;

    // std::cout << "processName: " << processName << std::endl;
    // std::cout << "options.format: " << static_cast<int>(options.format) << std::endl;
    // std::cout << "options.inputFilepath: " << options.inputFilepath << std::endl;

    return printInfo(options.inputFilepath);
}

int CommandInfo::printInfo(const _tstring& infile) {
    const bool isStdin = infile == _T("-");
    FILE* inf;

    if (isStdin) {
        inf = stdin;
#if defined(_WIN32)
        // Set "stdin" to have binary mode
        (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
    } else {
        // TODO: Is _tfopen depricated? fclose?
        inf = _tfopen(infile.c_str(), "rb");
    }

    if (inf) {
        KTX_error_code result;

        result = ktxPrintInfoForStdioStream(inf);
        if (result ==  KTX_FILE_UNEXPECTED_EOF) {
            std::cerr << processName << ": Unexpected end of file reading \""
                    << (isStdin ? "stdin" : infile) << "\"." << std::endl;
            return 2;
        }
        if (result == KTX_UNKNOWN_FILE_FORMAT) {
            std::cerr << processName << ": " << (isStdin ? "stdin" : infile)
                    << " is not a KTX or KTX2 file." << std::endl;
            return 2;
        }
    } else {
        std::cerr << processName << ": Could not open input file \""
                // TODO: Is strerror depricated?
                << (isStdin ? "stdin" : infile) << "\". " << strerror(errno) << std::endl;
        return 2;
    }

    return EXIT_SUCCESS;
}

} // namespace ktx
