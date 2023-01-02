// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "utility.h"
#include "validate.h"
#include <iostream>

#include <fmt/printf.h>


// -------------------------------------------------------------------------------------------------

namespace ktx {

/** @page ktxtools_validate ktx validate
@~English

Validates a KTX2 file.

@warning TODO Tools P5: This page is incomplete

@section ktxtools_validate_synopsis SYNOPSIS
    ktx validate [options] @e input_file

@section ktxtools_validate_description DESCRIPTION
    @b ktx @b validate validates and prints validation information about the KTX2 file provided as argument.

    @note @b ktx @b validate prints using UTF-8 encoding. If your console is not
    set for UTF-8 you will see incorrect characters in output of the file
    identifier on each side of the "KTX nn".

    The following options are available:
    <dl>
        <dt>--format &lt;text|json|mini-json&gt;</dt>
        <dd>Specifies the output format.
            @b text - Human readable text based format.
            @b json - Formatted JSON.
            @b mini-json - Minified JSON (Every optional formatting is skipped).
            The default format is @b text.
        </dd>
        <dt>-g, --gltf-basisu</dt>
        <dd>Check compatibility with KHR_texture_basisu glTF extension. Unset by default.</dd>
        <dt>-e, --warnings-as-errors</dt>
        <dd>Treat warnings as errors. Unset by default.</dd>
        <dt>-h, --help</dt>
        <dd>Print the usage message and exit.</dd>
        <dt>-v, --version</dt>
        <dd>Print the version number of this program and exit.</dd>
    </dl>

@section ktxtools_validate_exitstatus EXIT STATUS
    @b ktx @b validate exits
        0 on success,
        1 on command line errors and
        2 if the input file parsing failed.

@section ktxtools_validate_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_validate_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandValidate : public Command {
    struct Options {
        enum class OutputFormat {
            text,
            json,
            json_mini,
        };

        OutputFormat format = OutputFormat::text;
        bool gltfBasisu = false;
        bool warningsAsErrors = false;
        _tstring inputFilepath;
    };

    Options options;

public:
    void initializeOptions();
    virtual bool processOption(argparser& parser, int opt) override;
    void processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile);
    virtual int main(int argc, _TCHAR* argv[]) override;

    using Command::Command;
    virtual ~CommandValidate() {};

private:
    int validate(const _tstring& infile, bool warningsAsErrors, Options::OutputFormat format);
};

// -------------------------------------------------------------------------------------------------

void CommandValidate::initializeOptions() {
    option_list.emplace(option_list.begin(), "format", argparser::option::required_argument, nullptr, 'f');
    option_list.emplace(option_list.begin(), "gltf-basisu", argparser::option::no_argument, nullptr, 'g');
    option_list.emplace(option_list.begin(), "warnings-as-errors", argparser::option::no_argument, nullptr, 'e');
    short_opts += "f:ge";
}

bool CommandValidate::processOption(argparser& parser, int opt) {
    switch (opt) {
    case 'f':
        if (parser.optarg == "text") {
            options.format = Options::OutputFormat::text;
        } else if (parser.optarg == "json") {
            options.format = Options::OutputFormat::json;
        } else if (parser.optarg == "mini-json") {
            options.format = Options::OutputFormat::json_mini;
        } else {
            // TODO Tools P5: Print usage, Failure: unsupported format
            std::cerr << "Print usage, Failure: unsupported format" << std::endl;
            return false;
        }
        break;
    case 'g':
        options.gltfBasisu = true;
        break;
    case 'e':
        options.warningsAsErrors = true;
        break;
    default:
        return false;
    }

    return true;
}

void CommandValidate::processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile) {
    if (infiles.size() > 1) {
        // TODO Tools P5: Print usage, Failure: infiles.size() > 1
        std::cerr << "Print usage, Failure: infiles.size() > 1" << std::endl;
        // TODO Tools P1: Instead of std::exit handle argument parsing failures and stop execution
        std::exit(1);
        // return false;
    }

    options.inputFilepath = infiles[0];
    (void) outfile;
}

int CommandValidate::main(int argc, _TCHAR* argv[]) {
    initializeOptions();
    processCommandLine(argc, argv, StdinUse::eDisallowStdin, OutfilePos::eNone);
    processPositional(genericOptions.infiles, genericOptions.outfile);

    return validate(options.inputFilepath, options.warningsAsErrors, options.format);
}

int CommandValidate::validate(const _tstring& infile, bool warningsAsErrors, Options::OutputFormat format) {
    switch (format) {
    case Options::OutputFormat::text:
        return validateFile(infile, warningsAsErrors, [](const ValidationReport& issue) {
            fmt::print("{}-{:04}: {}\n", toString(issue.type), issue.id, issue.message);
            fmt::print("    {}", issue.details);
        });

    case Options::OutputFormat::json: [[fallthrough]];
    case Options::OutputFormat::json_mini: {
        const auto base_indent = format == Options::OutputFormat::json ? +0 : 0;
        const auto indent_width = format == Options::OutputFormat::json ? 4 : 0;
        const auto space = format == Options::OutputFormat::json ? " " : "";
        const auto nl = format == Options::OutputFormat::json ? "\n" : "";

        PrintIndent pi{base_indent, indent_width};

        pi(0, "{{{}", nl);
        bool first = true;
        const auto result = validateFile(infile, warningsAsErrors, [&](const ValidationReport& issue) {
            if (!std::exchange(first, false)) {
                pi(2, "}},{}", nl);
            } else {
                pi(1, "\"valid\":{}false,{}", space, nl);
                pi(1, "\"messages\":{}[{}", space, nl);
            }
            pi(2, "{{{}", nl);
            pi(3, "\"id\":{}{},{}", space, issue.id, nl);
            pi(3, "\"type\":{}\"{}\",{}", space, toString(issue.type), nl);
            pi(3, "\"message\":{}\"{}\",{}", space, replace_all_copy(issue.message, "\"", "\\\""), nl);
            pi(3, "\"details\":{}\"{}\"{}", space, replace_all_copy(issue.details, "\"", "\\\""), nl);
        });
        if (!first) {
            pi(2, "}}{}", nl);
            pi(1, "]{}", nl);
        } else {
            pi(1, "\"valid\":{}true,{}", space, nl);
            pi(1, "\"messages\":{}[]{}", space, nl);
        }
        pi(0, "}}{}", nl);

        return result;
    }
    }

    return 1;
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxValidate, ktx::CommandValidate)
