// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "argparser.h"

#include <iostream>
#include <memory>
#include <vector>


#if defined(_WIN32)
  #include <tchar.h>
#else
  #define _TCHAR char
  #define _T(x) x
#endif
#if defined(_UNICODE)
  #define _tstring std::wstring
#else
  #define _tstring std::string
#endif


// -------------------------------------------------------------------------------------------------

namespace ktx {

class Command {
protected:
    _tstring processName;

    struct CommandOptions {
        std::vector<_tstring> infiles;
        _tstring outfile;
        int test = false;
    };

    CommandOptions genericOptions;

    _tstring short_opts = _T("hv");

    std::vector<argparser::option> option_list{
            {"help", argparser::option::no_argument, nullptr, 'h'},
            {"version", argparser::option::no_argument, nullptr, 'v'},
            {"test", argparser::option::no_argument, &genericOptions.test, 1},
            // -NSDocumentRevisionsDebugMode YES is appended to the end
            // of the command by Xcode when debugging and "Allow debugging when
            // using document Versions Browser" is checked in the scheme. It
            // defaults to checked and is saved in a user-specific file not the
            // pbxproj file so it can't be disabled in a generated project.
            // Remove these from the arguments under consideration.
            {"-NSDocumentRevisionsDebugMode", argparser::option::required_argument, nullptr, 10000},
            {nullptr, argparser::option::no_argument, nullptr, 0}
    };

public:
    Command();
    virtual ~Command() = default;

public:
    virtual int main(int argc, _TCHAR* argv[]) = 0;
    virtual void usage() {
        std::cerr <<
                "  -h, --help    Print this usage message and exit.\n"
                "  -v, --version Print the version number of this program and exit.\n";
    };

protected:
    void error(const char* pFmt, ...);

    enum class StdinUse { eDisallowStdin, eAllowStdin };
    enum class OutfilePos { eNone, eFirst, eLast };

    void processCommandLine(int argc, _TCHAR* argv[],
            StdinUse stdinStat = StdinUse::eAllowStdin,
            OutfilePos outfilePos = OutfilePos::eNone,
            int startIndex = 1);
    bool loadFileList(const _tstring& f, bool relativize, std::vector<_tstring>& filenames);

    virtual void processOptions(argparser& parser);
    virtual bool processOption(argparser& parser, int opt) = 0;
    virtual void validateOptions() {}

    void writeId(std::ostream& dst, bool chktest);
    void printVersion();
};

[[nodiscard]] std::unique_ptr<Command> createCommandInfo();
[[nodiscard]] std::unique_ptr<Command> createCommandValidate();
// [[nodiscard]] std::unique_ptr<Command> createCommandTranscode();
// [[nodiscard]] std::unique_ptr<Command> createCommandEncode();
// [[nodiscard]] std::unique_ptr<Command> createCommandExtract();
// [[nodiscard]] std::unique_ptr<Command> createCommandCreate();
// [[nodiscard]] std::unique_ptr<Command> createCommandHelp();

} // namespace ktx
