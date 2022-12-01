// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include <memory>


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
    virtual int main(int argc, _TCHAR* argv[]) = 0;
    virtual ~Command() = default;
};

[[nodiscard]] std::unique_ptr<Command> createCommandInfo();
[[nodiscard]] std::unique_ptr<Command> createCommandValidate();
// [[nodiscard]] std::unique_ptr<Command> createCommandTranscode();
// [[nodiscard]] std::unique_ptr<Command> createCommandEncode();
// [[nodiscard]] std::unique_ptr<Command> createCommandExtract();
// [[nodiscard]] std::unique_ptr<Command> createCommandCreate();

} // namespace ktx
