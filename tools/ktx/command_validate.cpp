// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"
#include <iostream>


// -------------------------------------------------------------------------------------------------

namespace ktx {

// TODO: Proper doxygen documentation
// ktx validate <options> <input_file>
//          --format: text | json | mini-json
//              Output format. Defaults to text.
//          --gltf-basisu
//              Check compatibility with KHR_texture_basisu glTF extension. Unset by default.
//          --warnings-as-errors
//              Treat warnings as errors. Unset by default.
class CommandValidate : public Command {
public:
    void initializeOptions() {
    }

    virtual bool processOption(argparser& parser, int opt) override {
        (void) parser;
        (void) opt;
        return false;
    }

    void processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile) {
        (void) infiles;
        (void) outfile;
    }

    virtual int main(int argc, _TCHAR* argv[]) override {
        std::cout << "Hello, Validate" << std::endl;

        initializeOptions();
        processCommandLine(argc, argv, StdinUse::eAllowStdin, OutfilePos::eNone);
        processPositional(genericOptions.infiles, genericOptions.outfile);

        return EXIT_SUCCESS;
    }

    using Command::Command;
    virtual ~CommandValidate() {};
};

std::unique_ptr<Command> createCommandValidate() {
    return std::make_unique<CommandValidate>();
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxValidate, ktx::CommandValidate)
