// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "stdafx.h"
#include <cstdint>
#include <functional>
#include <stdexcept>
#include "validation_messages.h"


// -------------------------------------------------------------------------------------------------

namespace ktx {

struct ValidationReport {
    IssueType type;
    uint16_t id;
    std::string message;
    std::string details;
};

class FatalValidationError : public std::runtime_error {
public:
    ValidationReport report;

public:
    explicit FatalValidationError(ValidationReport report) :
        std::runtime_error(report.details),
        report(std::move(report)) {}
};

int validateFile(const _tstring& filepath, bool warningsAsErrors, std::function<void(const ValidationReport&)> callback);
int validateStream(FILE* file, bool warningsAsErrors, std::function<void(const ValidationReport&)> callback);
int validateMemory(const char* data, std::size_t size, bool warningsAsErrors, std::function<void(const ValidationReport&)> callback);

// -------------------------------------------------------------------------------------------------

} // namespace ktx
