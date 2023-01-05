// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "vkformat_enum.h"
#include <fmt/format.h>


// -------------------------------------------------------------------------------------------------
// External Functions
// These are part of libktx but not part of its public API.

extern "C" {
    bool isProhibitedFormat(VkFormat format);
    bool isValidFormat(VkFormat format);
    const char* vkFormatString(VkFormat format);
}

// -------------------------------------------------------------------------------------------------

namespace ktx {

[[nodiscard]] inline std::string toStringVkFormat(VkFormat format) noexcept {
    std::string str = vkFormatString(format);

    if (str != "VK_UNKNOWN_FORMAT")
        return str;

    return fmt::format("(0x{:X})", static_cast<uint32_t>(format));
}

[[nodiscard]] inline std::string toStringKTXSupercmpScheme(ktxSupercmpScheme scheme) noexcept {
    std::string str = ktxSupercompressionSchemeString(scheme);

    if (str == "Invalid scheme value")
        str = fmt::format("(0x{:X})", static_cast<uint32_t>(scheme));

    else if (str == "Vendor scheme")
        str = fmt::format("Vendor (0x{:X})", static_cast<uint32_t>(scheme));

    return str;
}

[[nodiscard]] constexpr inline bool isSupercompressionWithGlobalData(ktxSupercmpScheme scheme) noexcept {
    switch (scheme) {
    case KTX_SS_BASIS_LZ:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isFormatStencil(VkFormat format) noexcept {
    switch (format) {
    // Stencil only formats:
    case VK_FORMAT_S8_UINT: [[fallthrough]];
    // Depth and Stencil mixed formats:
    case VK_FORMAT_D16_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D24_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isFormatDepth(VkFormat format) noexcept {
    switch (format) {
    // Depth only formats:
    case VK_FORMAT_D16_UNORM: [[fallthrough]];
    case VK_FORMAT_X8_D24_UNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT: [[fallthrough]];
    // Depth and Stencil mixed formats:
    case VK_FORMAT_D16_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D24_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isSupercompressionBlockCompressed(ktxSupercmpScheme scheme) noexcept {
    switch (scheme) {
    case KTX_SS_BASIS_LZ:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isFormatBlockCompressed(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC2_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC2_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC3_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC3_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC4_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC4_SNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC5_SNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC6H_UFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC6H_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC7_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC7_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11_SNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isFormat3DBlockCompressed(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT:
        return true;
    default:
        return false;
    }
}

} // namespace ktx
