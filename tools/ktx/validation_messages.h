// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string_view>


// -------------------------------------------------------------------------------------------------

namespace ktx {

using IssueID = uint16_t;

enum class IssueType : uint8_t {
    warning,
    error,
    fatal,
};

[[nodiscard]] constexpr inline std::string_view toString(IssueType value) noexcept {
    switch (value) {
    case IssueType::warning:
        return "warning";
    case IssueType::error:
        return "error";
    case IssueType::fatal:
        return "fatal";
    }
    assert(false && "Invalid IssueType enum value");
    return "<<invalid>>";
}

struct Issue {
    IssueType type;
    IssueID id;
    std::string_view message;
    std::string_view detailsFmt;

    constexpr inline Issue(IssueType type, IssueID id, std::string_view message, std::string_view detailsFmt) :
            type(type),
            id(id),
            message(message),
            detailsFmt(detailsFmt) {}
};

struct IssueWarning : Issue {
    constexpr inline IssueWarning(IssueID id, std::string_view message, std::string_view detailsFmt) :
            Issue(IssueType::warning, id, message, detailsFmt) {}
};

struct IssueError : Issue {
    constexpr inline IssueError(IssueID id, std::string_view message, std::string_view detailsFmt) :
            Issue(IssueType::error, id, message, detailsFmt) {}
};

struct IssueFatal : Issue {
    constexpr inline IssueFatal(IssueID id, std::string_view message, std::string_view detailsFmt) :
            Issue(IssueType::fatal, id, message, detailsFmt) {}
};

// -------------------------------------------------------------------------------------------------

struct IOError {
    static constexpr IssueFatal FileOpen{
        1001, "Failed to open file.",
        "Failed to open file {}: {}."
    };
    static constexpr IssueFatal FileRead{
        1002, "Failed to read the file.",
        "File read failed: {}."
    };
    static constexpr IssueFatal UnexpectedEOF{
        1003, "Unexpected end of file.",
        "Unexpected end of file. Expected {} more byte for {} but only found {}."
    };
    static constexpr IssueFatal FileSeekEndFailure{
        1004, "Failed to seek to the end of the file.",
        "Failed to seek to the end of the file: {}."
    };
    static constexpr IssueFatal FileTellFailure{
        1005, "Failed to determine the size of the file.",
        "Failed to determine the size of the file: {}."
    };
    static constexpr IssueFatal RewindFailure{
        1006, "Failed to seek to the start of the file.",
        "Failed to seek to the start of the file: {}."
    };
};

struct FileError {
    static constexpr IssueFatal NotKTX2{
        2001, "Not a KTX2 file.", "Not a KTX2 file. Expected file identifier \"«KTX 20»\\r\\n\\x1A\\n\" was not found"
    };
    // static constexpr IssueFatal CreateFailure{
    //     2002, "ktxTexture2 creation failed: {}."
    // };
    // static constexpr IssueFatal IncorrectDataSize{
    //     2003, "Size of image data in file does not match size calculated from levelIndex."
    // };
};

struct HeaderData {
    static constexpr IssueError ProhibitedFormat{
        3001, "Prohibited VkFormat.",
        "Prohibited VkFormat {} cannot be used in a KTX2 file."
    };
    static constexpr IssueError InvalidFormat{
        3002, "Invalid VkFormat.",
        "Invalid VkFormat {}."
    };
    static constexpr IssueWarning UnknownFormat{
        3003, "Unknown VkFormat.",
        "Unknown VkFormat {}, possibly an extension format."
    };

    static constexpr IssueError VkFormatAndBasis{
        3004, "Invalid VkFormat. VkFormat must be VK_FORMAT_UNDEFINED for BASIS_LZ supercompression.",
        "VkFormat is {} but for supercompressionScheme BASIS_LZ it must be VK_FORMAT_UNDEFINED."
    };
    static constexpr IssueError TypeSizeNotOne{
        3005, "Invalid typeSize. typeSize must be 1 for block-compressed or supercompressed formats.",
        "typeSize is {} but for block-compressed or supercompressed format {} it must be 1."
    };

    static constexpr IssueError WidthZero{
        3006, "Invalid pixelWidth. pixelWidth cannot be 0.",
        "pixelWidth is 0, but textures must have width."
    };
    static constexpr IssueError BlockCompressedNoHeight{
        3007, "Invalid pixelHeight. pixelHeight cannot be 0 for a block compressed formats.",
        "pixelHeight is 0, but for block-compressed format {} it cannot be 0."
    };
    static constexpr IssueError CubeHeightWidthMismatch{
        3008, "Mismatching pixelWidth and pixelHeight for a cube map.",
        "pixelWidth is {} and pixelHeight is {}, but for a cube map they must be equal."
    };
    static constexpr IssueError DepthNoHeight{
        3009, "Invalid pixelHeight. pixelHeight cannot be 0 if pixelDepth is not also 0.",
        "pixelHeight is 0 and pixelDepth is {}, but pixelHeight cannot be 0 if pixelDepth is not 0 as well."
    };
    static constexpr IssueError DepthBlockCompressedNoDepth{
        3010, "Invalid pixelDepth. pixelDepth cannot be 0 for block-compressed formats with not zero block depth.",
        "pixelDepth is 0, but for format {} (which is block-compressed formats with not zero block depth) it cannot be 0."
    };
    static constexpr IssueError DepthFormatWithDepth{
        3011, "Invalid pixelDepth. pixelDepth must be 0 for depth formats.",
        "pixelDepth is {} but for depth format {} it must be 0."
    };
    static constexpr IssueError StencilFormatWithDepth{
        3012, "Invalid pixelDepth. pixelDepth must be 0 for stencil formats.",
        "pixelDepth is {} but for stencil format {} it must be 0."
    };
    static constexpr IssueError CubeWithDepth{
        3013, "Invalid pixelDepth. pixelDepth must be 0 for cube maps.",
        "pixelDepth is {} but for cube maps it must be 0. (Cube map faces must be 2D.)"
    };
    static constexpr IssueWarning ThreeDArray{
        3014, "File contains a 3D array texture.",
        "File contains a 3D array texture. No APIs support these."
    };
    static constexpr IssueError InvalidFaceCount{
        3015, "Invalid faceCount. faceCount must be 6 for and only for Cubemaps and Cubemap Arrays and 1 otherwise.",
        "faceCount is {} but it must be either 6 for Cubemaps and Cubemap Arrays or 1 otherwise."
    };
    static constexpr IssueError TooManyMipLevels{
        3016, "Too many mip levels",
        "levelCount is {} for the largest image dimension {} is too many levels."
    };
    static constexpr IssueError BlockCompressedNoLevel{
        3017, "Invalid levelCount. levelCount cannot be 0 for block-compressed formats.",
        "levelCount is 0 but for block-compressed format {} in cannot be 0."
    };
    static constexpr IssueWarning VendorSupercompression{
        3018, "Using vendor supercompressionScheme. Cannot validate.",
        "supercompressionScheme is 0x{:X} which falls into the reserved vendor range. Cannot validate."
    };
    static constexpr IssueError InvalidSupercompression{
        3019, "Invalid supercompressionScheme.",
        "Invalid supercompressionScheme: 0x{:X}."
    };

    // Index related issues:

    // static constexpr IssueError InvalidOptionalIndexEntry{
    //     30, "Invalid {} index entry. Only 1 of offset & length != 0."
    // };
    // static constexpr IssueError InvalidRequiredIndexEntry{
    //     30, "Index for required entry has offset or length == 0."
    // };
    // static constexpr IssueError InvalidDFDOffset{
    //     30, "Invalid dfdByteOffset. DFD must immediately follow level index."
    // };
    // static constexpr IssueError InvalidKVDOffset{
    //     30, "Invalid kvdByteOffset. KVD must immediately follow DFD."
    // };
    // static constexpr IssueError InvalidSGDOffset{
    //     30, "Invalid sgdByteOffset. SGD must follow KVD."
    // };
    // static constexpr IssueError TypeSizeMismatch{
    //     30, "typeSize, {}, does not match data described by the DFD."
    // };
    // static constexpr IssueError ZeroLevelCountForBC{
    //     30, "levelCount must be > 0 for block-compressed formats."
    // };
};

struct ValidatorError {
    // static constexpr IssueFatal CreateDfdFailure{
    //     40, "Creation of DFD matching {} failed."
    // };
    // static constexpr IssueFatal IncorrectDfd{
    //     40, "DFD created for {} confused interpretDFD()."
    // };
    // static constexpr IssueFatal DfdValidationFailure{
    //     40, "DFD validation passed a DFD which extactFormatInfo() could not handle."
    // };
};

// struct DFD {
//     static constexpr IssueError InvalidTransferFunction{
//         50, "Transfer function is not KHR_DF_TRANSFER_LINEAR or KHR_DF_TRANSFER_SRGB"
//     };
//     static constexpr IssueError IncorrectBasics{
//         50, "DFD format is not the correct type or version."
//     };
//     static constexpr IssueError IncorrectModelForBlock{
//         50, "DFD color model is not that of a block-compressed texture."
//     };
//     static constexpr IssueError MultiplePlanes{
//         50, "DFD is for a multiplane format. These are not supported."
//     };
//     static constexpr IssueError sRGBMismatch{
//         50, "DFD says sRGB but vkFormat is not an sRGB format."
//     };
//     static constexpr IssueError UnsignedFloat{
//         50, "DFD says data is unsigned float but there are no such texture formats."
//     };
//     static constexpr IssueError FormatMismatch{
//         50, "DFD does not match VK_FORMAT w.r.t. sign, float or normalization."
//     };
//     static constexpr IssueError ZeroSamples{
//         50, "DFD for a {} texture must have sample information."
//     };
//     static constexpr IssueError TexelBlockDimensionZeroForUndefined{
//         50, "DFD texel block dimensions must be non-zero for non-supercompressed texture"
//                         " with VK_FORMAT_UNDEFINED."
//     };
//     static constexpr IssueError FourDimensionalTexturesNotSupported{
//         50, "DFD texelBlockDimension3 is non-zero indicating an unsupported four-dimensional texture."
//     };
//     static constexpr IssueError BytesPlane0Zero{
//         50, "DFD bytesPlane0 must be non-zero for non-supercompressed {} texture."
//     };
//     static constexpr IssueError MultiplaneFormatsNotSupported{
//         50, "DFD has non-zero value in bytesPlane[1-7] indicating unsupported multiplane format."
//     };
//     static constexpr IssueError InvalidSampleCount{
//         50, "DFD for a {} texture must have {} sample(s)."
//     };
//     static constexpr IssueError IncorrectModelForBLZE{
//         50, "DFD colorModel for BasisLZ/ETC1S must be KHR_DF_MODEL_ETC1S."
//     };
//     static constexpr IssueError InvalidTexelBlockDimension{
//         50, "DFD texel block dimension must be {}x{} for {} textures."
//     };
//     static constexpr IssueError NotUnsized{
//         50, "DFD bytes/plane must be 0 for a supercompressed texture."
//     };
//     static constexpr IssueError InvalidChannelForBLZE{
//         50, "Only ETC1S_RGB (0), ETC1S_RRR (3), ETC1S_GGG (4) or ETC1S_AAA (15)"
//                         " channels allowed for BasisLZ/ETC1S textures."
//     };
//     static constexpr IssueError InvalidBitOffsetForBLZE{
//         50, "DFD sample bitOffsets for BasisLZ/ETC1S textures must be 0 and 64."
//     };
//     static constexpr IssueError InvalidBitLength{
//         50, "DFD sample bitLength for {} textures must be {}."
//     };
//     static constexpr IssueError InvalidLowerOrUpper{
//         50, "All DFD samples' sampleLower must be 0 and sampleUpper must be 0xFFFFFFFF for"
//                         "{} textures."
//     };
//     static constexpr IssueError InvalidChannelForUASTC{
//         50, "Only UASTC_RGB (0), UASTC_RGBA (3), UASTC_RRR (4) or UASTC_RRRG (5) channels"
//                         " allowed for UASTC textures."
//     };
//     static constexpr IssueError InvalidBitOffsetForUASTC{
//         50, "DFD sample bitOffset for UASTC textures must be 0."
//     };
//     static constexpr IssueError SizeMismatch{
//         50, "DFD totalSize differs from header's dfdByteLength."
//     };
//     static constexpr IssueError InvalidColorModel{
//         50, "DFD colorModel for non block-compressed textures must be RGBSDA."
//     };
//     static constexpr IssueError MixedChannels{
//         50, "DFD has channels with differing flags, e.g. some float, some integer."
//     };
//     static constexpr IssueError Multisample{
//         50, "DFD indicates multiple sample locations."
//     };
//     static constexpr IssueError NonTrivialEndianness{
//         50, "DFD describes non little-endian data."
//     };
//     static constexpr IssueError InvalidPrimaries{
//         50, "DFD primaries value, {}, is invalid."
//     };
//     static constexpr IssueError SampleCountMismatch{
//         50, "DFD sample count {} differs from expected {}."
//     };
//     static constexpr IssueError BytesPlane0Mismatch{
//         50, "DFD bytesPlane0 value {} differs from expected {}."
//     };
// };
//
// struct LevelIndex {
//     static constexpr IssueError IncorrectByteLength{
//         60, "Level {} byteLength 0x{:X} does not match expected value 0x{:X}."
//     };
//     static constexpr IssueError ByteOffsetTooSmall{
//         60, "Level {} byteOffset 0x{:X} is smaller than expected value 0x{:X}."
//     };
//     static constexpr IssueError IncorrectByteOffset{
//         60, "Level {} byteOffset 0x{:X} does not match expected value 0x{:X}."
//     };
//     static constexpr IssueError IncorrectUncompressedByteLength{
//         60, "Level {} uncompressedByteLength 0x{:X} does not match expected value 0x{:X}."
//     };
//     static constexpr IssueError UnequalByteLengths{
//         60, "Level {} uncompressedByteLength does not match byteLength."
//     };
//     static constexpr IssueError UnalignedOffset{
//         60, "Level {} byteOffset is not aligned to required {} byte alignment."
//     };
//     static constexpr IssueError ExtraPadding{
//         60, "Level {} has disallowed extra padding."
//     };
//     static constexpr IssueError ZeroOffsetOrLength{
//         60, "Level {}'s byteOffset or byteLength is 0."
//     };
//     static constexpr IssueError ZeroUncompressedLength{
//         60, "Level {}'s uncompressedByteLength is 0."
//     };
//     static constexpr IssueError IncorrectLevelOrder{
//         60, "Larger mip levels are before smaller."
//     };
// };
//
// struct Metadata {
//     static constexpr IssueError MissingNulTerminator{
//         70, "Required NUL terminator missing from metadata key beginning \"%5s\"."
//                         "Abandoning validation of individual metadata entries."
//     };
//     static constexpr IssueError ForbiddenBOM1{
//         70, "Metadata key beginning \"%5s\" has forbidden BOM."
//     };
//     static constexpr IssueError ForbiddenBOM2{
//         70, "Metadata key beginning \"{}\" has forbidden BOM."
//     };
//     static constexpr IssueError InvalidStructure{
//         70, "Invalid metadata structure? keyAndValueByteLengths failed to total kvdByteLength"
//                         " after {} KV pairs."
//     };
//     static constexpr IssueError MissingFinalPadding{
//         70, "Required valuePadding after last metadata value missing."
//     };
//     static constexpr IssueError OutOfOrder{
//         70, "Metadata keys are not sorted in codepoint order."
//     };
//     static constexpr IssueWarning CustomMetadata{
//         70, "Custom metadata \"{}\" found."
//     };
//     static constexpr IssueError IllegalMetadata{
//         70, "Unrecognized metadata \"{}\" found with KTX or ktx prefix found."
//     };
//     static constexpr IssueWarning ValueNotNulTerminated{
//         70, "{} value missing encouraged NUL termination."
//     };
//     static constexpr IssueError InvalidValue{
//         70, "{} has invalid value."
//     };
//     static constexpr IssueError NoRequiredKTXwriter{
//         70, "No KTXwriter key. Required when KTXwriterScParams is present."
//     };
//     static constexpr IssueError MissingValue{
//         70, "Missing required value for \"{}\" key."
//     };
//     static constexpr IssueError NotAllowed{
//         70, "\"{}\" key not allowed {}."
//     };
//     static constexpr IssueWarning NoKTXwriter{
//         70, "No KTXwriter key. Writers are strongly urged to identify themselves via this."
//     };
// };
//
// struct SGD {
//     static constexpr IssueError UnexpectedSupercompressionGlobalData{
//         80, "Supercompression global data found scheme that is not Basis."
//     };
//     static constexpr IssueError MissingSupercompressionGlobalData{
//         80, "Basis supercompression global data missing."
//     };
//     static constexpr IssueError InvalidImageFlagBit{
//         80, "Basis supercompression global data imageDesc.imageFlags has an invalid bit set."
//     };
//     static constexpr IssueError IncorrectGlobalDataSize{
//         80, "Basis supercompression global data has incorrect size."
//     };
//     static constexpr IssueError ExtendedByteLengthNotZero{
//         80, "extendedByteLength != 0 in Basis supercompression global data."
//     };
//     static constexpr IssueError DfdMismatchAlpha{
//         80, "supercompressionGlobalData indicates no alpha but DFD indicates alpha channel."
//     };
//     static constexpr IssueError DfdMismatchNoAlpha{
//         80, "supercompressionGlobalData indicates an alpha channel but DFD indicates no alpha channel."
//     };
// };
//
// struct Transcode {
//     static constexpr IssueError Failure{
//         90, "Transcode of BasisU payload failed: {}"
//     };
// };
//
// struct System {
//     static constexpr IssueError OutOfMemory{
//         100, "System out of memory."
//     };
// };

// -------------------------------------------------------------------------------------------------

} // namespace ktx
