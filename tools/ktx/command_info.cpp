// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.hpp"
#include <iostream>


// -------------------------------------------------------------------------------------------------

namespace ktx {

// TODO: Proper doxygen documentation
// ktx info <options> <input_file>
//          --format: text | json
//              Output format. Defaults to text.
class CommandInfo : public Command {
public:
    virtual int main(int argc, _TCHAR* argv[]) override {
        std::cout << "Hello, Info" << std::endl;
        (void) argc;
        (void) argv;
        return EXIT_SUCCESS;
    }
    virtual ~CommandInfo() {};
};

std::unique_ptr<Command> createCommandInfo() {
    return std::make_unique<CommandInfo>();
}

} // namespace ktx
