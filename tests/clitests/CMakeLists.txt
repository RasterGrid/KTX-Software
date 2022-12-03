# Copyright 2022-2023 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0

find_package(PythonInterp 3 REQUIRED)

enable_testing()

set(CASELIST
    input/info/samplePass.json
    input/validate/sampleFail.json)

foreach(CASE_FILE ${CASELIST})
    get_filename_component(CASE_DIR ${CASE_FILE} DIRECTORY)
    get_filename_component(CASE_NAME ${CASE_FILE} NAME_WE)
    string(REGEX REPLACE "^input/" "ktxToolTests/" CASE_TRIMMED_DIR "${CASE_DIR}/")
    string(REPLACE "/" "." CASE_GROUP "${CASE_TRIMMED_DIR}")
    set(FULL_CASE_NAME "${CASE_GROUP}${CASE_NAME}")

    add_test(NAME ${FULL_CASE_NAME}
        COMMAND ${PYTHON_EXECUTABLE} clitest.py ${CASE_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # TODO: Replace these with the unified ktx tool binary directory
    get_target_property(KTXINFO_PATH ktxinfo BINARY_DIR)
    set_property(TEST ${FULL_CASE_NAME} PROPERTY ENVIRONMENT_MODIFICATION "PATH=path_list_append:${KTXINFO_PATH}")
endforeach()
