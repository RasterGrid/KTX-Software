// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "ktxapp.h"
#include "utility.h"
#include "version.h"

// #include <algorithm>
// #include <functional>
#include <iostream>
#include <map>
#include <string_view>

#include "command.hpp"


// -------------------------------------------------------------------------------------------------

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

std::string myversion(STR(KTX_VERSION));
std::string mydefversion(STR(KTX_DEFAULT_VERSION));

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
//
// ktx info <options> <input_file>
//          --format: text | json
//              Output format. Defaults to text.
//
// ktx transcode <options> <input_file> <output_file>
//          --target: etc-rgb | etc-rgba | eac-r11 | eac-rg11 | bc1 | bc3 | bc4 | bc5 | bc7 | astc | r8 | rg8 | rgb8 | rgba8
//              Target format. Case-insensitive.
//
// ktx encode <options> <input_file> <output_file>
//          --codec: basis-lz | uastc <codec_options>
//              Target format followed by codec specific options. Case-insensitive.
//
// ktx extract <options> <input_file> <output>
//          --transcode: <target>
//              Target transcode format. Case-insensitive.
//          --level: [0-9]+ | all
//              Level to extract. Defaults to 0. Case-insensitive.
//          --layer: [0-9]+ | all
//              Layer to extract. Defaults to 0. Case-insensitive.
//          --face: [0-5] | all
//              Face to extract. Defaults to 0. Case-insensitive.
//          --depth: [0-9]+ | all
//              Depth slice to extract. Defaults to 0. Case-insensitive.
//          --all
//              Extract all image slices. Unset by default.
//          --raw
//              Extract raw image data. Unset by default.
//          --uri <ktx_fragment_uri>
//              TODO: URI Fragment support https://github.khronos.org/KTX-Specification/ktx_frag.html
//
// ktx create <options> <input_files...> <output_file>
//          --1d
//              Create a 1D texture. Unset by default.
//          --width, --height, --depth: [0-9]+
//              Base level dimensions in pixels.
//          --layers: [0-9]+
//              Number of layers.
//          --cubemap
//              Create a cubemap texture. Unset by default.
//          --levels: [0-9]+
//              Number of mip levels.
//          --mipmap: runtime | generate <filtering_options>
//              Mipmap generation mode followed by filtering options. Unset by default.
//          --format: <enum> <encoding_options>
//              KTX format enum optionally followed by encoding options. Case insensitive.
//          --raw
//              Create from raw image data. Unset by default.
//          --encode: <codec> <codec_options>
//              Encode the created KTX file. Unset by default.
//          --swizzle: [rgba01]{4}
//              KTX swizzle metadata. Unset by default.
//          --input-swizzle: [rgba01]{4}
//              Pre-swizzle input channels. Unset by default.
//          --assign-oetf: <oetf>
//          --assign-primaries: <primaries>
//          --convert-oetf: <oetf>
//          --convert-primaries: <primaries

class ktxTools : public ktxApp {
private:
    std::unique_ptr<Command> command;

public:
    explicit ktxTools();
    virtual ~ktxTools() {};

public:
    virtual int main(int argc, _TCHAR* argv[]) override;
    virtual bool processOption(argparser& parser, int opt) override {
        (void) parser;
        (void) opt;
        return true;
    }
};

ktxTools::ktxTools() : ktxApp(myversion, mydefversion, options) {
}

[[nodiscard]] std::unique_ptr<Command> createCommand(std::string_view name) {
    using createFn = std::unique_ptr<Command>(*)();
    std::map<std::string, createFn, std::less<>> commands;

    commands.emplace("info", createCommandInfo);
    commands.emplace("validate", createCommandValidate);
    // commands.emplace("transcode", createCommandTranscode);
    // commands.emplace("encode", createCommandEncode);
    // commands.emplace("extract", createCommandExtract);
    // commands.emplace("create", createCommandCreate);

    const auto it = commands.find(name);
    return it == commands.end() ? nullptr : (it->second)();
}

int ktxTools::main(int argc, _TCHAR* argv[]) {
    std::cout << "Hello, KTX Tools" << std::endl;

    if (argc < 2) {
        // TODO: Print usage, Failure: missing sub command
        std::cerr << "Print usage, Failure: missing sub command" << std::endl;
        return EXIT_FAILURE;
    }

    const auto commandName = trim(argv[1]);
    command = createCommand(commandName);
    if (!command) {
        // TODO: Print usage, Failure: incorrect sub command {commandName}
        std::cerr << "Print usage, Failure: incorrect sub command {commandName}" << std::endl;
        return EXIT_FAILURE;
    }

    return command->main(argc, argv);
}

} // namespace ktx ---------------------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[]) {
    ktx::ktxTools theApp;
    return theApp.main(argc, argv);
}
