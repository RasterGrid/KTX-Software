# Copyright 2022-2023 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0


add_executable(ktxtools
        command.cpp
        command.h
        command_info.cpp
        command_validate.cpp
        ktx_main.cpp
        utility.h
        validate.cpp
        validate.h
        validation_messages.h
        vkformat.h
)

create_version_header(tools/ktx ktxtools)

set_target_properties(
    ktxtools
    PROPERTIES
        OUTPUT_NAME ktx
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
)

target_include_directories(
    ktxtools
PRIVATE
    .
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
)

target_link_libraries(
    ktxtools
    ktx
    objUtil
    fmt::fmt
)

target_compile_definitions(
    ktxtools
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

set_tool_properties(ktxtools)
set_code_sign(ktxtools)
