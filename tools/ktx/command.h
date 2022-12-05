// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "argparser.h"
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
public:
    _tstring processName;
    const bool hasOutputFile;

public:
    virtual void initializeOptions(std::vector<argparser::option>& long_opts, _tstring& short_opts) = 0;
    virtual bool processOption(argparser& parser, int opt) = 0;
    virtual void processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile) = 0;
    virtual int main(int argc, _TCHAR* argv[]) = 0;

    explicit inline Command(bool hasOutputFile) noexcept : hasOutputFile(hasOutputFile) {}
    virtual ~Command() = default;
};

[[nodiscard]] std::unique_ptr<Command> createCommandInfo();
[[nodiscard]] std::unique_ptr<Command> createCommandValidate();
// [[nodiscard]] std::unique_ptr<Command> createCommandTranscode();
// [[nodiscard]] std::unique_ptr<Command> createCommandEncode();
// [[nodiscard]] std::unique_ptr<Command> createCommandExtract();
// [[nodiscard]] std::unique_ptr<Command> createCommandCreate();

} // namespace ktx
