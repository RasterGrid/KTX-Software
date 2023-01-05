# Copyright 2022-2023 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0

find_package(PythonInterp 3 REQUIRED)

enable_testing()

set(CASELIST
    input/info/sample_text.json
    input/info/sample_json.json
    input/info/sample_mini.json
    input/validate/sample.json
    input/validate/fatal_1003_UnexpectedEOF.json
    input/validate/fatal_2001_NotKTX2.json
    input/validate/error_3001_ProhibitedFormat.json
    input/validate/error_3002_InvalidFormat.json
    input/validate/warning_3003_UnknownFormat.json
    input/validate/error_3004_VkFormatAndBasis.json
    input/validate/error_3005_TypeSizeNotOne.json
    input/validate/error_3006_WidthZero.json
    input/validate/error_3007_BlockCompressedNoHeight.json
    input/validate/error_3008_CubeHeightWidthMismatch.json
    input/validate/error_3009_DepthNoHeight.json
    input/validate/error_3013_CubeWithDepth.json)

set(CASE_REGEN_GOLDEN_COMMANDS)

foreach(CASE_FILE ${CASELIST})
    get_filename_component(CASE_DIR ${CASE_FILE} DIRECTORY)
    get_filename_component(CASE_NAME ${CASE_FILE} NAME_WE)
    string(REGEX REPLACE "^input/" "ktxToolTests/" CASE_TRIMMED_DIR "${CASE_DIR}/")
    string(REPLACE "/" "." CASE_GROUP "${CASE_TRIMMED_DIR}")
    set(FULL_CASE_NAME "${CASE_GROUP}${CASE_NAME}")

    list(APPEND CASE_REGEN_GOLDEN_COMMANDS
        COMMAND ${PYTHON_EXECUTABLE} clitest.py ${CASE_FILE} --executable-path $<TARGET_FILE:ktxtools> --regen-golden)

    add_test(NAME ${FULL_CASE_NAME}
        COMMAND ${PYTHON_EXECUTABLE} clitest.py ${CASE_FILE} --executable-path $<TARGET_FILE:ktxtools>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

add_custom_target(clitests_regen_golden
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    ${CASE_REGEN_GOLDEN_COMMANDS})
