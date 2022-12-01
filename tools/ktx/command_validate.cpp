// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.hpp"
#include <iostream>


// -------------------------------------------------------------------------------------------------

namespace ktx {

// TODO: Proper doxygen documentation
// ktx validate <options> <input_file>
//          --format: text | json
//              Output format. Defaults to text.
//          --gltf-basisu
//              Check compatibility with KHR_texture_basisu glTF extension. Unset by default.
//          --warnings-as-errors
//              Treat warnings as errors. Unset by default.
class CommandValidate : public Command {
public:
    virtual int main(int argc, _TCHAR* argv[]) override {
        std::cout << "Hello, Validate" << std::endl;
        (void) argc;
        (void) argv;
        return EXIT_SUCCESS;
    }
    virtual ~CommandValidate() {};
};

std::unique_ptr<Command> createCommandValidate() {
    return std::make_unique<CommandValidate>();
}

} // namespace ktx
