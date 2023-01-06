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
        "Unexpected end of file. Expected {} more byte for {} but only found {} byte."
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
    static constexpr IssueFatal UnexpectedEOFSeek{
        1007, "Unexpected end of file. Requested seek position is not in the file.",
        "Requested seek position is {} for accessing {}, but the file is only {} byte long."
    };
};

struct FileError {
    static constexpr IssueFatal NotKTX2{
        2001, "Not a KTX2 file.",
        "Not a KTX2 file. The beginning of the file does not matches the expected file identifier \"«KTX 20»\\r\\n\\x1A\\n\"."
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
        3003, "Unknown VkFormat. Possibly an extension format.",
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
    // static constexpr IssueError TypeSizeMismatch{
    //     30, "typeSize, {}, does not match data described by the DFD."
    // };

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
        3010, "Invalid pixelDepth. pixelDepth cannot be 0 for block-compressed formats with non-zero block depth.",
        "pixelDepth is 0, but for format {} (which is a block-compressed format with non-zero block depth) it cannot be 0."
    };
    static constexpr IssueError DepthStencilFormatWithDepth{
        3011, "Invalid pixelDepth. pixelDepth must be 0 for depth or stencil formats.",
        "pixelDepth is {} but for depth or stencil format {} it must be 0."
    };

    // 3012 Unused

   static constexpr IssueError CubeWithDepth{
        3013, "Invalid pixelDepth. pixelDepth must be 0 for cube maps.",
        "pixelDepth is {} but for cube maps it must be 0 (cube map faces must be 2D)."
    };
    static constexpr IssueWarning ThreeDArray{
        3014, "File contains a 3D array texture.",
        "File contains a 3D array texture. No APIs support these."
    };
    static constexpr IssueError InvalidFaceCount{
        3015, "Invalid faceCount. faceCount must be either 6 for Cubemaps and Cubemap Arrays or 1 otherwise.",
        "faceCount is {} but it must be either 6 for Cubemaps and Cubemap Arrays or 1 otherwise."
    };
    static constexpr IssueError TooManyMipLevels{
        3016, "Too many mip levels",
        "levelCount is {} for the largest image dimension {} is too many levels."
    };
    static constexpr IssueError BlockCompressedNoLevel{
        3017, "Invalid levelCount. levelCount cannot be 0 for block-compressed formats.",
        "levelCount is 0 but for block-compressed format {} it cannot be 0."
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

    static constexpr IssueError IndexDFDZeroOffset{
        3020, "Invalid dataFormatDescriptor.byteOffset. byteOffset cannot be 0.",
        "dataFormatDescriptor.byteOffset is 0, but the file must have a dataFormatDescriptor."
    };
    static constexpr IssueError IndexDFDAlignment{
        3021, "Invalid dataFormatDescriptor.byteOffset. Defined region must be aligned to 4 byte.",
        "dataFormatDescriptor.byteOffset is {}, but the byteOffset must be aligned to 4 byte."
    };
    static constexpr IssueError IndexDFDZeroLength{
        3022, "Invalid dataFormatDescriptor.byteLength. byteLength cannot be 0.",
        "dataFormatDescriptor.byteLength is 0, but the file must have a dataFormatDescriptor."
    };
    static constexpr IssueError IndexDFDInvalid{
        3023, "Invalid dataFormatDescriptor index. Defined region cannot exceed the size of the file.",
        "dataFormatDescriptor.byteOffset is {} and dataFormatDescriptor.byteLength is {}, but the file is only {} byte long."
    };

    static constexpr IssueError IndexKVDOffsetWithoutLength{
        3024, "Invalid keyValueData.byteOffset. byteOffset must be 0 if the byteLength is 0.",
        "keyValueData.byteOffset is {}, but if the byteLength is 0 it must also be 0."
    };
    static constexpr IssueError IndexKVDAlignment{
        3025, "Invalid keyValueData.byteOffset. Defined region must be aligned to 4 byte.",
        "keyValueData.byteOffset is {}, but the byteOffset must be aligned to 4 byte."
    };
    static constexpr IssueError IndexKVDInvalid{
        3026, "Invalid keyValueData index. Defined region cannot exceed the size of the file.",
        "keyValueData.byteOffset is {} and keyValueData.byteLength is {}, but the file is only {} byte long."
    };

    static constexpr IssueError IndexSGDOffsetWithoutLength{
        3027, "Invalid supercompressionGlobalData.byteOffset. byteOffset must be 0 if the byteLength is 0.",
        "supercompressionGlobalData.byteOffset is {}, but if the byteLength is 0 it must also be 0."
    };
    static constexpr IssueError IndexSGDAlignment{
        3028, "Invalid supercompressionGlobalData.byteOffset. Defined region must be aligned to 8 byte.",
        "supercompressionGlobalData.byteOffset is {}, but the byteOffset must be aligned to 8 byte."
    };
    static constexpr IssueError IndexSGDMissing{
        3029, "Invalid supercompressionGlobalData.byteLength. byteLength cannot be 0 for supercompression schemes with global data.",
        "supercompressionGlobalData.byteLength is 0, but for supercompression scheme {} (which has global data) it cannot be 0."
    };
    static constexpr IssueError IndexSGDExists{
        3030, "Invalid supercompressionGlobalData.byteLength. byteLength must be 0 for supercompression schemes without global data.",
        "supercompressionGlobalData.byteLength is {}, but for supercompression scheme {} (which has no global data) it must be 0."
    };
    static constexpr IssueError IndexSGDInvalid{
        3031, "Invalid supercompressionGlobalData index. Defined region cannot exceed the size of the file.",
        "supercompressionGlobalData.byteOffset is {} and supercompressionGlobalData.byteLength is {}, but the file is only {} byte long."
    };

    static constexpr IssueError IndexDFDContinuity{
        3032, "Invalid dataFormatDescriptor.byteOffset. DFD region must immediately follow the level index.",
        "dataFormatDescriptor.byteOffset is {}, but DFD region must immediately follow (with 4 byte alignment) the level index so it must {}."
    };
    static constexpr IssueError IndexKVDContinuity{
        3033, "Invalid keyValueData.byteOffset. KVD region must immediately follow the DFD region.",
        "keyValueData.byteOffset is {}, but KVD region must immediately follow (with 4 byte alignment) the DFD region so it must {}."
    };
    static constexpr IssueError IndexSGDContinuity{
        3034, "Invalid supercompressionGlobalData.byteOffset. SGD region must immediately follow the KVD region.",
        "supercompressionGlobalData.byteOffset is {}, but SGD region must immediately follow (with 8 byte alignment) the KVD region so it must {}."
    };
};

struct LevelIndex {
    // static constexpr IssueError IncorrectByteLength{
    //     40, "Level {} byteLength 0x{:X} does not match expected value 0x{:X}."
    // };
    // static constexpr IssueError ByteOffsetTooSmall{
    //     40, "Level {} byteOffset 0x{:X} is smaller than expected value 0x{:X}."
    // };
    // static constexpr IssueError IncorrectByteOffset{
    //     40, "Level {} byteOffset 0x{:X} does not match expected value 0x{:X}."
    // };
    // static constexpr IssueError IncorrectUncompressedByteLength{
    //     40, "Level {} uncompressedByteLength 0x{:X} does not match expected value 0x{:X}."
    // };
    // static constexpr IssueError UnequalByteLengths{
    //     40, "Level {} uncompressedByteLength does not match byteLength."
    // };
    // static constexpr IssueError UnalignedOffset{
    //     40, "Level {} byteOffset is not aligned to required {} byte alignment."
    // };
    // static constexpr IssueError ExtraPadding{
    //     40, "Level {} has disallowed extra padding."
    // };
    // static constexpr IssueError ZeroOffsetOrLength{
    //     40, "Level {}'s byteOffset or byteLength is 0."
    // };
    // static constexpr IssueError ZeroUncompressedLength{
    //     40, "Level {}'s uncompressedByteLength is 0."
    // };
    // static constexpr IssueError IncorrectLevelOrder{
    //     40, "Larger mip levels are before smaller."
    // };
};

struct ValidatorError {
    // static constexpr IssueFatal CreateDfdFailure{
    //     60, "Creation of DFD matching {} failed."
    // };
    // static constexpr IssueFatal IncorrectDfd{
    //     60, "DFD created for {} confused interpretDFD()."
    // };
    // static constexpr IssueFatal DfdValidationFailure{
    //     60, "DFD validation passed a DFD which extactFormatInfo() could not handle."
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

struct Metadata {
    static constexpr IssueError TooManyEntry{
        7001, "Invalid keyValueData. The number of key-value entries exceeds the maximum allowed.",
        "The number of key-value entries exceeds the maximum allowed {}."
    };
    static constexpr IssueError NotEnoughDataForAnEntry{
        7002, "Invalid keyValueData. Not enough data left in keyValueData to process another key-value entry",
        "KeyValueData has {} byte unprocessed, but for a key value entry at least 6 byte is required (4 byte size + 1 byte key + 1 byte \\0)."
    };
    static constexpr IssueError KeyValuePairSizeTooBig{
        7003, "Invalid keyAndValueByteLength. The value is bigger than the amount of bytes left in the keyValueData.",
        "keyAndValueByteLength is {}, but the keyValueData only has {} byte left for the key-value pair."
    };
    static constexpr IssueError KeyValuePairSizeTooSmall{
        7004, "Invalid keyAndValueByteLength. keyAndValueByteLength must be at least 2.",
        "keyAndValueByteLength is {}, but it must be at least 2 (1 byte key + 1 byte \\0)."
    };
    static constexpr IssueError KeyMissingNullTerminator{
        7005, "Invalid keyValueData entry is missing the NULL terminator. Every key-value entry must have a NULL terminator separating the key from the value.",
        "The key-value entry \"{}\" is missing the NULL terminator, but every key-value entry must have a NULL terminator separating the key from the value."
    };
    static constexpr IssueError KeyForbiddenBOM{
        7006, "Invalid key in keyValueData. Key cannot contain BOM.",
        "The beginning of the key \"{}\" has forbidden BOM."
    };
    static constexpr IssueError KeyInvalidUTF8{
        7007, "Invalid key in keyValueData. Key must be a valid UTF8 string.",
        "Key is \"{}\", which contains an invalid UTF8 character at position: {}."
    };

    static constexpr IssueError SizesDontAddUp{
        7008, "Invalid keyValueData. keyValueData.byteLength must add up to sum of the key-value entries with paddings.",
        "The processed keyValueData length is {}, but keyValueData.byteLength is {}, but they must match."
    };
    static constexpr IssueError UnknownReservedKey{
        7009, "Invalid key in keyValueData. Keys with \"KTX\" or \"ktx\" prefix are reserved.",
        "The key is \"{}\", but its not recognized and every key with \"KTX\" or \"ktx\" prefix are reserved."
    };
    static constexpr IssueWarning CustomMetadata{
        7010, "Custom key in keyValueData.",
        "Custom key \"{}\" found in keyValueData."
    };
    static constexpr IssueError PaddingNotZero{
        7011, "Invalid padding byte value. Every padding byte's value must be 0.",
        "A padding byte value is {:d} {}, but it must be 0."
    };

    static constexpr IssueError OutOfOrder{
        7012, "Invalid keyValueData. Key-value entries must be sorted by their key.",
        "Key-value entries are not sorted, but they must be sorted by their key.",
    };
    static constexpr IssueError DuplicateKey{
        7013, "Invalid keyValueData. Keys must be unique.",
        "There is a duplicate key, but the keys must be unique."
    };

    static constexpr IssueError KTXcubemapIncompleteInvalidSize{
        7100, "Invalid KTXcubemapIncomplete metadata. The size of the value must be 1 byte.",
        "The size of the KTXcubemapIncomplete value is {}, but it must be 1 byte."
    };
    static constexpr IssueError KTXcubemapIncompleteInvalidValue{
        7101, "Invalid KTXcubemapIncomplete value. The two MSB must be 0.",
        "The value is {:08b} but the two MSB must be 0 (00XXXXXX)."
    };
    static constexpr IssueWarning KTXcubemapIncompleteAllBitSet{
        7102, "KTXcubemapIncomplete is not incomplete. All face is marked present.",
        "All face bit is set as present. Prefer using normal Cube maps instead."
    };
    static constexpr IssueError KTXcubemapIncompleteNoBitSet{
        7103, "Invalid KTXcubemapIncomplete value. No face is marked present.",
        "No face bit is set as present, but at least 1 face must be present."
    };
    static constexpr IssueError KTXcubemapIncompleteIncompatibleLayerCount{
        7104, "Incompatible KTXcubemapIncomplete and layerCount. layerCount must be the multiple of the number of faces present.",
        "layerCount is {} and KTXcubemapIncomplete indicates {} faces present, but layerCount must the multiple of the number of faces present."
    };
    static constexpr IssueError KTXcubemapIncompleteWithFaceCountNot1{
        7105, "Invalid faceCount. faceCount must be 1 if KTXcubemapIncomplete is present.",
        "faceCount is {}, but if KTXcubemapIncomplete is present it must be 1."
    };

    static constexpr IssueError KTXorientationInvalidSize{
        7106, "Invalid KTXorientation metadata. The size of the value must be 2 to 4 byte (including the NULL terminator).",
        "The size of the KTXorientation value is {}, but it must be 2 to 4 byte (including the NULL terminator)."
    };
    static constexpr IssueError KTXorientationMissingNull{
        7107, "Invalid KTXorientation metadata. The value is missing the NULL terminator.",
        "The last byte of the value must be a NULL terminator."
    };
    static constexpr IssueError KTXorientationIncorrectDimension{
        7108, "Invalid KTXorientation value. The number of dimensions specified must match the number of dimension in the texture type.",
        "The value has {} dimension, but the dimension of the texture type has {} and they must match."
    };
    static constexpr IssueError KTXorientationInvalidValue{
        7109, "Invalid KTXorientation value. The value must match /^[rl]$/ for 1D, /^[rl][du]$/ for 2D and /^[rl][du][oi]$/ for 3D texture types.",
        "Dimension {} is \"{}\", but for it must be either \"{}\" or \"{}\"."
    };

    static constexpr IssueError KTXglFormatInvalidSize{
        7110, "Invalid KTXglFormat metadata. The size of the value must be 12 byte.",
        "The size of KTXglFormat value is {}, but it must be 12 byte."
    };
    static constexpr IssueError KTXglFormatWithVkFormat{
        7111, "Incompatible KTXglFormatWithVkFormat with vkFormat. vkFormat must be VK_FORMAT_UNDEFINED if KTXglFormatWithVkFormat is present.",
        "vkFormat is {}, but if KTXglFormatWithVkFormat is present it must VK_FORMAT_UNDEFINED."
    };
    static constexpr IssueError KTXglFormatInvalidValueForCompressed{
        7112, "Invalid KTXglFormatInvalidValue value. glFormat and glType must be zero for compressed formats.",
        "glFormat is {} and glType is {}, but for compressed formats both must be zero."
    };

    static constexpr IssueError KTXdxgiFormatInvalidSize{
        7113, "Invalid KTXdxgiFormat__ metadata. The size of the value must be 4 byte.",
        "The size of KTXdxgiFormat__ value is {}, but it must be 4 byte."
    };
    static constexpr IssueError KTXdxgiFormatWithVkFormat{
        7114, "Incompatible KTXdxgiFormat__ with vkFormat. vkFormat must be VK_FORMAT_UNDEFINED if KTXdxgiFormat__ is present.",
        "vkFormat is {}, but if KTXdxgiFormat__ is present it must VK_FORMAT_UNDEFINED."
    };

    static constexpr IssueError KTXmetalPixelFormatInvalidSize{
        7115, "Invalid KTXmetalPixelFormat metadata. The size of the value must be 4 byte.",
        "The size of KTXmetalPixelFormat value is {}, but it must be 4 byte."
    };
    static constexpr IssueError KTXmetalPixelFormatWithVkFormat{
        7116, "Incompatible KTXdxgiFormat__ with vkFormat. vkFormat must be VK_FORMAT_UNDEFINED if KTXdxgiFormat__ is present.",
        "vkFormat is {}, but if KTXdxgiFormat__ is present it must VK_FORMAT_UNDEFINED."
    };

    static constexpr IssueError KTXswizzleInvalidSize{
        7117, "Invalid KTXswizzle metadata. The size of the value must be 5 byte (including the NULL terminator).",
        "The size of KTXswizzle value is {}, but it must be 5 byte (including the NULL terminator)."
    };
    static constexpr IssueError KTXswizzleMissingNull{
        7118, "Invalid KTXswizzle metadata. The value is missing the NULL terminator.",
        "The last byte of the value must be a NULL terminator."
    };
    static constexpr IssueError KTXswizzleInvalidValue{
        7119, "Invalid KTXswizzle value. The value must match /^[rgba01]{4}$/.",
        "The character at position {} is \"{}\", but it must be one of \"rgba01\"."
    };
    static constexpr IssueError KTXswizzleInvalidValueMissingChannel{
        7120, "Invalid KTXswizzle value. The value must be \"0\" for color or \"1\" for alpha if the channels is not present.",
        "The character at position {} is \"{}\", but that {} channel is not present so the value must be \"{}\"."
    };
    static constexpr IssueWarning KTXswizzleWithDepthOrStencil{
        7121, "KTXswizzle has no effect on depth or stencil texture formats.",
        "KTXswizzle is present but for vkFormat {} it has effect."
    };

    static constexpr IssueError KTXwriterMissingNull{
        7122, "Invalid KTXwriter metadata. The value is missing the NULL terminator.",
        "The last byte of the value must be a NULL terminator."
    };
    static constexpr IssueWarning KTXwriterInvalidUTF8{
        7123, "Invalid KTXwriter value. The value must be a valid UTF8 string.",
        "The value contains an invalid UTF8 character at position: {}."
    };
    static constexpr IssueError KTXwriterRequiredButMissing{
        7124, "Missing KTXwriter metadata. When KTXwriterScParams is present KTXwriter must also be present",
        "KTXwriter metadata is missing. When KTXwriterScParams is present KTXwriter must also be present"
    };
    static constexpr IssueWarning KTXwriterMissing{
        7125, "Missing KTXwriter metadata. Writers are strongly urged to identify themselves via this.",
        "KTXwriter metadata is missing. Writers are strongly urged to identify themselves via this."
    };

    static constexpr IssueError KTXwriterScParamsMissingNull{
        7126, "Invalid KTXwriterScParams metadata. The value is missing the NULL terminator.",
        "The last byte of the value must be a NULL terminator."
    };
    static constexpr IssueWarning KTXwriterScParamsInvalidUTF8{
        7127, "Invalid KTXwriterScParams value. The value must be a valid UTF8 string.",
        "The value contains an invalid UTF8 character at position: {}."
    };


    static constexpr IssueError InvalidSizeKTXanimData{
        70, "Invalid KTXanimData metadata. The size of the value must be 12 byte.",
        "The size of KTXanimData value is {}, but it must be 12 byte."
    };


    // static constexpr IssueWarning ValueNotNulTerminated{
    //     70, "{} value missing encouraged NUL termination."
    // };
    // static constexpr IssueError InvalidValue{
    //     70, "{} has invalid value."
    // };
    // static constexpr IssueError MissingValue{
    //     70, "Missing required value for \"{}\" key."
    // };
    // static constexpr IssueError NotAllowed{
    //     70, "\"{}\" key not allowed {}."
    // };
};

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
