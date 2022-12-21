// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "stdafx.h"


// -------------------------------------------------------------------------------------------------

namespace ktx {

class FileGuard {
    FILE* file = nullptr;

public:
    explicit inline FileGuard(const char* tfilepath, const char* mode) {
#ifdef _WIN32
        _tfopen_s(&file, tfilepath, mode);
#else
        file = _tfopen(tfilepath, mode);
#endif
    }

    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) & = delete;
    FileGuard(FileGuard&&) = delete;
    FileGuard& operator=(FileGuard&&) & = delete;

    inline ~FileGuard() {
        if (file != nullptr)
            fclose(file);
    }

public:
    /*implicit*/ inline operator FILE*() {
        return file;
    }

    /*implicit*/ inline operator const FILE*() const {
        return file;
    }

    explicit inline operator bool() const {
        return file;
    }

    [[nodiscard]] inline bool operator!() const {
        return !file;
    }
};

} // namespace ktx
