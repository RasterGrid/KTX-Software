// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "stdafx.h"
#include <fmt/printf.h>
#include <optional>


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

// -------------------------------------------------------------------------------------------------

template <typename T>
[[nodiscard]] constexpr inline T align(const T value, const T alignment) noexcept {
    return (alignment - 1 + value) / alignment * alignment;
}

template <typename T>
[[nodiscard]] constexpr inline T* align(T* ptr, std::uintptr_t alignment) noexcept {
    return reinterpret_cast<T*>(align(reinterpret_cast<std::uintptr_t>(ptr), alignment));
}

// C++20 - std::popcount
template <typename T>
[[nodiscard]] constexpr inline int popcount(T value) noexcept {
    int count = 0;
    for (; value != 0; value >>= 1)
        if (value & 1)
            count++;
    return count;
}

// C++20 - string.starts_with(prefix)
[[nodiscard]] constexpr inline bool starts_with(std::string_view string, std::string_view prefix) noexcept {
    return prefix == string.substr(0, prefix.size());
}

// C++20 - std::identity
struct identity {
    using is_transparent = void;

    template <typename T>
    [[nodiscard]] constexpr inline T&& operator()(T&& arg) const noexcept {
        return std::forward<T>(arg);
    }
};

template <typename Range, typename Comp = std::less<>, typename Proj = identity>
[[nodiscard]] constexpr inline bool is_sorted(const Range& range, Comp&& comp = {}, Proj&& proj = {}) {
    return std::is_sorted(std::begin(range), std::end(range), [&](const auto& lhs, const auto& rhs) {
        return comp(std::invoke(proj, lhs), std::invoke(proj, rhs));
    });
}

/// \param range must be sorted
template <typename Range, typename Proj = identity>
[[nodiscard]] constexpr inline bool is_unique(const Range& range, Proj&& proj = {}) {
    const auto end = std::end(range);
    return end == std::adjacent_find(std::begin(range), end, [&](const auto& lhs, const auto& rhs) {
       return std::invoke(proj, lhs) == std::invoke(proj, rhs);
    });
}

template <typename Range, typename Comp = std::less<>, typename Proj = identity>
constexpr inline void sort(Range& range, Comp&& comp = {}, Proj&& proj = {}) {
    return std::sort(std::begin(range), std::end(range), [&](const auto& lhs, const auto& rhs) {
        return comp(std::invoke(proj, lhs), std::invoke(proj, rhs));
    });
}

inline void replace_all_inplace(std::string& string, std::string_view search, std::string_view replace) {
    auto pos = string.find(search);
    while (pos != std::string::npos) {
        string.replace(pos, search.size(), replace);
        pos = string.find(search, pos + replace.size());
    }
}

[[nodiscard]] inline std::string replace_all_copy(std::string string, std::string_view search, std::string_view replace) {
    replace_all_inplace(string, search, replace);
    return string;
}

[[nodiscard]] inline std::string escape_json_copy(std::string string) {
    replace_all_inplace(string, "\\", "\\\\");
    replace_all_inplace(string, "\"", "\\\"");
    replace_all_inplace(string, "\n", "\\n");
    return string;
}

// -------------------------------------------------------------------------------------------------

constexpr inline std::optional<std::size_t> validateUTF8(std::string_view string) noexcept {
    // TODO Tools P5: Implement validateUTF8
    (void) string;
    return std::nullopt;
}

// -------------------------------------------------------------------------------------------------

struct PrintIndent {
    int indentBase = 0;
    int indentWidth = 4;

public:
    template <typename Fmt, typename... Args>
    inline void operator()(int depth, Fmt&& fmt, Args&&... args) {
        fmt::print("{:{}}", "", indentWidth * (indentBase + depth));
        fmt::print(std::forward<Fmt>(fmt), std::forward<Args>(args)...);
    }
};

} // namespace ktx
