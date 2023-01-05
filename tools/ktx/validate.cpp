// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include <fmt/format.h>
#include <fmt/printf.h>
#include <KHR/khr_df.h>
#include <ktx.h>
#include "ktxint.h"
#include <stdexcept>

#include "utility.h"
#include "validate.h"
#include "validation_messages.h"
#include "vkformat.h"


// -------------------------------------------------------------------------------------------------

namespace ktx {

struct SampleType {
    uint32_t bitOffset: 16;
    uint32_t bitLength: 8;
    uint32_t channelType: 8; // Includes qualifiers
    uint32_t samplePosition0: 8;
    uint32_t samplePosition1: 8;
    uint32_t samplePosition2: 8;
    uint32_t samplePosition3: 8;
    uint32_t lower;
    uint32_t upper;
};

struct BDFD {
    uint32_t vendorId: 17;
    uint32_t descriptorType: 15;
    uint32_t versionNumber: 16;
    uint32_t descriptorBlockSize: 16;
    uint32_t model: 8;
    uint32_t primaries: 8;
    uint32_t transfer: 8;
    uint32_t flags: 8;
    uint32_t texelBlockDimension0: 8;
    uint32_t texelBlockDimension1: 8;
    uint32_t texelBlockDimension2: 8;
    uint32_t texelBlockDimension3: 8;
    uint32_t bytesPlane0: 8;
    uint32_t bytesPlane1: 8;
    uint32_t bytesPlane2: 8;
    uint32_t bytesPlane3: 8;
    uint32_t bytesPlane4: 8;
    uint32_t bytesPlane5: 8;
    uint32_t bytesPlane6: 8;
    uint32_t bytesPlane7: 8;
    SampleType samples[6];
};

// -------------------------------------------------------------------------------------------------

class FatalValidationError : public std::runtime_error {
public:
    ValidationReport report;

public:
    explicit FatalValidationError(ValidationReport report) :
        std::runtime_error(report.details),
        report(std::move(report)) {}
};

// -------------------------------------------------------------------------------------------------

struct ValidationContext {
private:
    static constexpr int32_t MAX_NUM_KV_ENTRY = 100;

private:
    const char* fileData = nullptr;
    std::size_t fileSize = 0;
    const char* fileIt = nullptr;

private:
    std::function<void(const ValidationReport&)> callback;

    bool treatWarningsAsError = false;
    // bool checkGLTFBasisu = false; // TODO Tools P2: implement checkGLTFBasisu

    uint32_t numError = 0;
    uint32_t numWarning = 0;

private:
    KTX_header2 header{};

    uint32_t layerCount = 0;
    uint32_t levelCount = 0;
    uint32_t dimensionCount = 0;

private:
    bool foundKTXanimData = false;
    bool foundKTXcubemapIncomplete = false;
    bool foundKTXwriter = false;
    bool foundKTXwriterScParams = false;

public:
    ValidationContext(bool warningsAsErrors, std::function<void(const ValidationReport&)> callback) :
        callback(std::move(callback)),
        treatWarningsAsError(warningsAsErrors) {
        std::memset(&header, 0, sizeof(header));
    }

private:
    // warning, error and fatal functions are only used for validation readability
    template <typename... Args>
    void warning(const IssueWarning& issue, Args&&... args) {
        if (treatWarningsAsError) {
            ++numError;
            callback(ValidationReport{IssueType::error, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});

        } else {
            ++numWarning;
            callback(ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});
        }
    }

    template <typename... Args>
    void error(const IssueError& issue, Args&&... args) {
        ++numError;
        callback(ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});
    }

    template <typename... Args>
    void fatal(const IssueFatal& issue, Args&&... args) {
        ++numError;
        const auto report = ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)};
        callback(report);

        throw FatalValidationError(report);
    }

private:
    void seek_to(std::size_t target, std::string_view name) {
        // TODO Tools P5: Switch to a different seeking/reading pattern
        assert(fileIt <= fileData + target); // To ensure forward seeking only

        if (target > fileSize) // Seek to the end byte is allowed (but reading it is not allowed)
            fatal(IOError::UnexpectedEOFSeek, target, name, fileSize);

        fileIt = fileData + target;
    }

    void read(void* readDst, std::size_t readSize, std::string_view name) {
        if (fileIt + readSize > fileData + fileSize)
            fatal(IOError::UnexpectedEOF, readSize, name, fileData + fileSize - fileIt);

        std::memcpy(readDst, fileIt, readSize);
    }

private:
    template <typename... Args>
    void validatePaddingZeros(const void* ptr, const void* bufferEnd, std::size_t alignment, const IssueError& issue, Args&&... args) {
        const auto* begin = static_cast<const char*>(ptr);
        const auto* end = std::min(bufferEnd, align(ptr, alignment));

        for (auto it = begin; it != end; ++it)
            if (*it != 0)
                error(issue, *it, std::forward<Args>(args)...);
    }

public:
    int validateFile(const _tstring& filepath);
    int validateStream(FILE* file);
    int validateMemory(const char* data, std::size_t size);

private:
    int validate();
    void validateHeader();
    void validateIndices();
    // void validateLevelIndex();
    // void validate...();
    // void validateDFD();
    void validateKVD();

    void validateKVCubemapIncomplete(std::string_view key, const uint8_t* data, uint32_t size);
    void validateKVOrientation(std::string_view key, const uint8_t* data, uint32_t size);
    void validateKVGlFormat(std::string_view key, const uint8_t* data, uint32_t size);
    void validateKVDxgiFormat(std::string_view key, const uint8_t* data, uint32_t size);
    void validateKVMetalPixelFormat(std::string_view key, const uint8_t* data, uint32_t size);
    void validateKVSwizzle(std::string_view key, const uint8_t* data, uint32_t size);
    void validateKVWriter(std::string_view key, const uint8_t* data, uint32_t size);
    void validateKVWriterScParams(std::string_view key, const uint8_t* data, uint32_t size);
    void validateKVAstcDecodeMode(std::string_view key, const uint8_t* data, uint32_t size);
    void validateKVAnimData(std::string_view key, const uint8_t* data, uint32_t size);
};

// -------------------------------------------------------------------------------------------------

int validateFile(const _tstring& filepath, bool warningsAsErrors, std::function<void(const ValidationReport&)> callback) {
    ValidationContext ctx{warningsAsErrors, std::move(callback)};
    try {
        return ctx.validateFile(filepath);
    } catch (const FatalValidationError&) {
        return 3; // During fatal error, skip the rest of the validation and return
    }
}

int validateStream(FILE* file, bool warningsAsErrors, std::function<void(const ValidationReport&)> callback) {
    ValidationContext ctx{warningsAsErrors, std::move(callback)};
    try {
        return ctx.validateStream(file);
    } catch (const FatalValidationError&) {
        return 3; // During fatal error, skip the rest of the validation and return
    }
}

int validateMemory(const char* data, std::size_t size, bool warningsAsErrors, std::function<void(const ValidationReport&)> callback) {
    ValidationContext ctx{warningsAsErrors, std::move(callback)};
    try {
        return ctx.validateMemory(data, size);
    } catch (const FatalValidationError&) {
        return 3; // During fatal error, skip the rest of the validation and return
    }
}

// -------------------------------------------------------------------------------------------------

int ValidationContext::validateFile(const _tstring& filepath) {
    FileGuard file(filepath.c_str(), "rb");
    if (!file) {
        const auto ec = std::make_error_code(static_cast<std::errc>(errno));
        fatal(IOError::FileOpen, filepath, ec.message());
    }

    return validateStream(file);
}

int ValidationContext::validateStream(FILE* file) {
    int64_t fileResult;
    fileResult = fseek(file, 0, SEEK_END);
    if (fileResult != 0) {
        const auto ec = std::make_error_code(static_cast<std::errc>(errno));
        fatal(IOError::FileSeekEndFailure, ec.message());
    }

    fileResult = ftell(file);
    if (fileResult == -1L) {
        const auto ec = std::make_error_code(static_cast<std::errc>(errno));
        fatal(IOError::FileTellFailure, ec.message());
    }
    const auto length = static_cast<std::size_t>(fileResult);

    fileResult = fseek(file, 0, SEEK_SET);
    if (fileResult != 0) {
        const auto ec = std::make_error_code(static_cast<std::errc>(errno));
        fatal(IOError::RewindFailure, ec.message());
    }

    // C++20 make_unique_for_overwrite
    const auto buffer = std::make_unique<char[]>(length);
    const auto bytesRead = fread(buffer.get(), 1, length, file);

    if (bytesRead != length) {
        const auto ec = std::make_error_code(static_cast<std::errc>(errno));
        fatal(IOError::FileRead, ec.message());
    }

    return validateMemory(buffer.get(), length);
}

int ValidationContext::validateMemory(const char* data_, std::size_t size_) {
    fileData = data_;
    fileSize = size_;
    fileIt = data_;
    return validate();
}

// -------------------------------------------------------------------------------------------------

int ValidationContext::validate() {
    validateHeader();
    validateIndices();
    // parseDFD();
    // validateLevelIndex();
    // validate...();
    validateKVD();
    // validate...();

    // TODO Tools P3: Verify alignment (4) padding zeros between levelIndex and DFD
    // TODO Tools P3: Verify alignment (4) padding zeros between DFD and KVD
    // TODO Tools P3: Verify alignment (8) padding zeros between KVD and SGD
    // TODO Tools P3: Verify alignment (?) padding zeros between SGD and image levels
    // TODO Tools P3: Verify alignment (?) padding zeros between image levels

    return numError > 0 ? 3 : 0;
}

void ValidationContext::validateHeader() {
    static constexpr uint8_t ktx2_identifier_reference[12] = KTX2_IDENTIFIER_REF;

    read(&header, sizeof(KTX_header2), "the header");
    const auto vkFormat = static_cast<VkFormat>(header.vkFormat);
    const auto supercompressionScheme = static_cast<ktxSupercmpScheme>(header.supercompressionScheme);

    // Validate file identifier
    if (std::memcmp(&header.identifier, ktx2_identifier_reference, 12) != 0)
        fatal(FileError::NotKTX2);

    // Validate vkFormat
    if (isProhibitedFormat(vkFormat)) {
        error(HeaderData::ProhibitedFormat, toStringVkFormat(vkFormat));

    } else if (!isValidFormat(vkFormat)) {
        if (vkFormat <= VK_FORMAT_MAX_STANDARD_ENUM)
            error(HeaderData::InvalidFormat, toStringVkFormat(vkFormat));
        if (VK_FORMAT_MAX_STANDARD_ENUM < vkFormat && vkFormat < 1000001000)
            error(HeaderData::InvalidFormat, toStringVkFormat(vkFormat));
        if (1000001000 <= vkFormat)
            warning(HeaderData::UnknownFormat, toStringVkFormat(vkFormat));
    }
    if (vkFormat < 0)
        error(HeaderData::InvalidFormat, toStringVkFormat(vkFormat));

    if (header.supercompressionScheme == KTX_SS_BASIS_LZ) {
        if (header.vkFormat != VK_FORMAT_UNDEFINED)
            error(HeaderData::VkFormatAndBasis, toStringVkFormat(vkFormat));
    }

    // Validate typeSize
    if (header.vkFormat == VK_FORMAT_UNDEFINED) {
        if (header.typeSize != 1)
            error(HeaderData::TypeSizeNotOne, header.typeSize, toStringVkFormat(vkFormat));

    } else if (isFormatBlockCompressed(vkFormat)) {
        if (header.typeSize != 1)
            error(HeaderData::TypeSizeNotOne, header.typeSize, toStringVkFormat(vkFormat));
    }
    // Additional checks are performed on typeSize after the DFD is parsed

    // Validate image dimensions
    if (header.pixelWidth == 0)
        error(HeaderData::WidthZero);

    if (isFormatBlockCompressed(vkFormat))
        if (header.pixelHeight == 0)
            error(HeaderData::BlockCompressedNoHeight, toStringVkFormat(vkFormat));
    if (isSupercompressionBlockCompressed(supercompressionScheme))
        if (header.pixelHeight == 0)
            error(HeaderData::BlockCompressedNoHeight, toStringKTXSupercmpScheme(supercompressionScheme));
    // Additional block-compressed formats (like UASTC) are detected after the DFD is parsed to validate pixelHeight

    if (header.faceCount == 6)
        if (header.pixelWidth != header.pixelHeight)
            error(HeaderData::CubeHeightWidthMismatch, header.pixelWidth, header.pixelHeight);

    if (header.pixelDepth != 0 && header.pixelHeight == 0)
        error(HeaderData::DepthNoHeight, header.pixelDepth);

    if (isFormat3DBlockCompressed(vkFormat))
        if (header.pixelDepth == 0)
            error(HeaderData::DepthBlockCompressedNoDepth, toStringVkFormat(vkFormat));

    if (isFormatDepth(vkFormat) || isFormatStencil(vkFormat))
        if (header.pixelDepth != 0)
            error(HeaderData::DepthStencilFormatWithDepth, header.pixelDepth, toStringVkFormat(vkFormat));

    if (header.faceCount == 6)
        if (header.pixelDepth != 0)
            error(HeaderData::CubeWithDepth, header.pixelDepth);

    // Detect dimension counts
    if (header.pixelDepth != 0) {
        if (header.layerCount != 0) {
            warning(HeaderData::ThreeDArray); // Warning on 3D Array textures
            dimensionCount = 4;
        } else {
            dimensionCount = 3;
        }
    } else if (header.pixelHeight != 0) {
        dimensionCount = 2;
    } else {
        dimensionCount = 1;
    }

    // Validate layerCount to actual number of layers.
    layerCount = std::max(header.layerCount, 1u);

    // Validate faceCount
    if (header.faceCount != 6 && header.faceCount != 1)
        error(HeaderData::InvalidFaceCount, header.faceCount);

    // Cube map faces are validated 2D by checking: CubeHeightWidthMismatch and CubeWithDepth

    // Validate levelCount
    if (isFormatBlockCompressed(vkFormat))
        if (header.levelCount == 0)
            error(HeaderData::BlockCompressedNoLevel, toStringVkFormat(vkFormat));
    if (isSupercompressionBlockCompressed(supercompressionScheme))
        if (header.levelCount == 0)
            error(HeaderData::BlockCompressedNoLevel, toStringKTXSupercmpScheme(supercompressionScheme));
    // Additional block-compressed formats (like UASTC) are detected after the DFD is parsed to validate levelCount

    levelCount = std::max(header.levelCount, 1u);

    // This test works for arrays too because height or depth will be 0.
    const auto max_dim = std::max(std::max(header.pixelWidth, header.pixelHeight), header.pixelDepth);
    // TODO Tools P4: Verify 'log2'
    if (max_dim < ((uint32_t) 1 << (levelCount - 1))) {
        // Can't have more mip levels than 1 + log2(max(width, height, depth))
        error(HeaderData::TooManyMipLevels, levelCount, max_dim);
    }

    // Validate supercompressionScheme
    if (KTX_SS_BEGIN_VENDOR_RANGE <= header.supercompressionScheme && header.supercompressionScheme <= KTX_SS_END_VENDOR_RANGE)
        warning(HeaderData::VendorSupercompression, header.supercompressionScheme);
    else if (header.supercompressionScheme < KTX_SS_BEGIN_RANGE || KTX_SS_END_RANGE < header.supercompressionScheme)
        error(HeaderData::InvalidSupercompression, header.supercompressionScheme);
}

void ValidationContext::validateIndices() {
    const auto supercompressionScheme = static_cast<ktxSupercmpScheme>(header.supercompressionScheme);

    // Validate dataFormatDescriptor index
    if (header.dataFormatDescriptor.byteOffset == 0)
        error(HeaderData::IndexDFDZeroOffset);

    if (header.dataFormatDescriptor.byteOffset % 4 != 0)
        error(HeaderData::IndexDFDAlignment, header.dataFormatDescriptor.byteOffset);

    if (header.dataFormatDescriptor.byteLength == 0)
        error(HeaderData::IndexDFDZeroLength);

    if (header.dataFormatDescriptor.byteOffset + header.dataFormatDescriptor.byteLength > fileSize)
        error(HeaderData::IndexDFDInvalid, header.dataFormatDescriptor.byteOffset, header.dataFormatDescriptor.byteLength, fileSize);

    // Validate keyValueData index
    if (header.keyValueData.byteLength == 0)
        if (header.keyValueData.byteOffset != 0)
            error(HeaderData::IndexKVDOffsetWithoutLength, header.keyValueData.byteOffset);

    if (header.keyValueData.byteOffset % 4 != 0)
        error(HeaderData::IndexKVDAlignment, header.keyValueData.byteOffset);

    if (header.keyValueData.byteOffset + header.keyValueData.byteLength > fileSize)
        error(HeaderData::IndexKVDInvalid, header.keyValueData.byteOffset, header.keyValueData.byteLength, fileSize);

    // Validate supercompressionGlobalData index
    if (header.supercompressionGlobalData.byteLength == 0)
        if (header.supercompressionGlobalData.byteOffset != 0)
            error(HeaderData::IndexSGDOffsetWithoutLength, header.supercompressionGlobalData.byteOffset);

    if (header.supercompressionGlobalData.byteOffset % 8 != 0)
        error(HeaderData::IndexSGDAlignment, header.supercompressionGlobalData.byteOffset);

    if (isSupercompressionWithGlobalData(supercompressionScheme)) {
        if (header.supercompressionGlobalData.byteLength == 0)
            error(HeaderData::IndexSGDMissing, toStringKTXSupercmpScheme(supercompressionScheme));
    } else {
        if (header.supercompressionGlobalData.byteLength != 0)
            error(HeaderData::IndexSGDExists, header.supercompressionGlobalData.byteLength, toStringKTXSupercmpScheme(supercompressionScheme));
    }

    if (header.supercompressionGlobalData.byteOffset + header.supercompressionGlobalData.byteLength > fileSize)
        error(HeaderData::IndexSGDInvalid, header.supercompressionGlobalData.byteOffset, header.supercompressionGlobalData.byteLength, fileSize);

    // Validate region positioning and continuity
    const auto levelIndexSize = sizeof(ktxLevelIndexEntry) * levelCount;
    std::size_t expectedOffset = KTX2_HEADER_SIZE + levelIndexSize;

    expectedOffset = align(expectedOffset, std::size_t{4});
    if (expectedOffset != header.dataFormatDescriptor.byteOffset)
        error(HeaderData::IndexDFDContinuity, header.dataFormatDescriptor.byteOffset, expectedOffset);
    expectedOffset += header.dataFormatDescriptor.byteLength;

    if (header.keyValueData.byteLength != 0) {
        expectedOffset = align(expectedOffset, std::size_t{4});
        if (expectedOffset != header.keyValueData.byteOffset)
            error(HeaderData::IndexKVDContinuity, header.keyValueData.byteOffset, expectedOffset);
        expectedOffset += header.keyValueData.byteLength;
    }

    if (header.supercompressionGlobalData.byteLength != 0) {
        expectedOffset = align(expectedOffset, std::size_t{8});
        if (expectedOffset != header.supercompressionGlobalData.byteOffset)
            error(HeaderData::IndexSGDContinuity, header.supercompressionGlobalData.byteOffset, expectedOffset);
        expectedOffset += header.supercompressionGlobalData.byteLength;
    }
}

// =================================================================================================
// TODO Tools P2: Validate Level Index
//
// void ValidationContext::validateLevelIndex() {
//     std::vector<ktxLevelIndexEntry> levelIndices(levelCount);
//
//     const auto levelIndexSize = sizeof(ktxLevelIndexEntry) * levelCount;
//     read(levelIndices.data(), levelIndexSize, "the level index");
//
//     validateDfd(ctx);
//     if (!ctx.pDfd4Format) {
//         // VK_FORMAT_UNDEFINED so we have to get info from the actual DFD.
//         // Not hugely robust but validateDfd does check known undefineds such
//         // as UASTC.
//         if (!ctx.extractFormatInfo(ctx.pActualDfd)) {
//             addIssue(logger::eError, ValidatorError.DfdValidationFailure);
//         }
//     }
//
//     uint32_t requiredLevelAlignment = ctx.requiredLevelAlignment();
//     // size_t expectedOffset = 0;
//     // size_t lastByteLength = 0;
//     //
//     // switch (ctx.header.supercompressionScheme) {
//     //   case KTX_SS_NONE:
//     //   case KTX_SS_ZSTD:
//     //     expectedOffset = padn(requiredLevelAlignment, ctx.kvDataEndOffset());
//     //     break;
//     //   case KTX_SS_BASIS_LZ:
//     //     ktxIndexEntry64 sgdIndex = ctx.header.supercompressionGlobalData;
//     //     // No padding here.
//     //     expectedOffset = sgdIndex.byteOffset + sgdIndex.byteLength;
//     //     break;
//     // }
//     //
//     // expectedOffset = padn(requiredLevelAlignment, expectedOffset);
//
//     // Last mip level is first in the file. Count down, so we can check the
//     // distance between levels for the UNDEFINED and SUPERCOMPRESSION cases.
//     for (int32_t levelIt = static_cast<int32_t>(levelCount) - 1; levelIt >= 0; --levelIt) {
//         const auto& level = levelIndices[static_cast<std::size_t>(levelIt)];
//
//         const auto knownLevelSizes =
//                 header.vkFormat != VK_FORMAT_UNDEFINED &&
//                 header.supercompressionScheme == KTX_SS_NONE;
//
//         if (knownLevelSizes) {
//             ktx_size_t expectedUBL = ctx.calcLevelSize(level);
//             if (level.uncompressedByteLength != expectedUBL)
//                 error(LevelIndex::IncorrectUncompressedByteLength, level, level.uncompressedByteLength, expectedUBL);
//
//             if (level.byteLength != level.uncompressedByteLength)
//                 error(LevelIndex::UnequalByteLengths, level);
//
//             ktx_size_t expectedOffset = ctx.calcLevelOffset(level);
//             if (level.byteOffset != expectedOffset) {
//                 if (level.byteOffset % requiredLevelAlignment != 0)
//                     error(LevelIndex::UnalignedOffset, level, requiredLevelAlignment);
//
//                 if (level.byteOffset > expectedOffset)
//                     error(LevelIndex::ExtraPadding, level);
//
//                 else
//                     error(LevelIndex::ByteOffsetTooSmall, level, level.byteOffset, expectedOffset);
//             }
//
//         } else {
//             // Can only do minimal validation as we have no idea what the
//             // level sizes are, so we have to trust the byteLengths. We do
//             // at least know where the first level must be in the file, and
//             // we can calculate how much padding, if any, there must be
//             // between levels.
//             // if (level.byteLength == 0 || level.byteOffset == 0) {
//             //      error(LevelIndex::ZeroOffsetOrLength, level);
//             //      continue;
//             // }
//             //
//             // if (level.byteOffset != expectedOffset) {
//             //     error(LevelIndex::IncorrectByteOffset, level, level.byteOffset, expectedOffset);
//             // }
//             //
//             // if (header.supercompressionScheme == KTX_SS_NONE) {
//             //     if (level.byteLength < lastByteLength)
//             //         addIssue(logger.eError, LevelIndex::IncorrectLevelOrder);
//             //     if (level.byteOffset % requiredLevelAlignment != 0)
//             //         error(LevelIndex::UnalignedOffset, level, requiredLevelAlignment);
//             //     if (level.uncompressedByteLength == 0) {
//             //         error(LevelIndex::ZeroUncompressedLength, level);
//             //     }
//             //     lastByteLength = level.byteLength;
//             // }
//             //
//             // expectedOffset += padn(requiredLevelAlignment, level.byteLength);
//             // if (header.vkFormat != VK_FORMAT_UNDEFINED) {
//             //     // We can validate the uncompressedByteLength.
//             //     ktx_size_t level.uncompressedByteLength = level.uncompressedByteLength;
//             //     ktx_size_t expectedUBL = ctx.calcLevelSize(level);
//             //
//             //     if (level.uncompressedByteLength != expectedUBL)
//             //         error(LevelIndex::IncorrectUncompressedByteLength, level, level.uncompressedByteLength, expectedUBL);
//             // }
//         }
//
//         // ctx.dataSizeFromLevelIndex += padn(ctx.requiredLevelAlignment(), level.byteLength);
//     }
// }
//
// =================================================================================================
// TODO Tools P2: Validate DFD
//
// void ktxValidator::validateDfd(validationContext& ctx)
// {
//     if (ctx.header.dataFormatDescriptor.byteLength == 0)
//         return;
//
//         Check that are deferred during header parsing so the DFD is available
//         if (header.supercompressionScheme != KTX_SS_BASIS_LZ) {
//            // TODO Tools P4: Clarify and implement these conditions
//
//            // createDfd4Format();
//            // if (pDfd4Format == nullptr) {
//            //     fatal(ValidatorError::CreateDfdFailure, vkFormatString(vkFormat));
//            // } else if (!extractFormatInfo(pDfd4Format)) {
//            //     error(ValidatorError::IncorrectDfd, vkFormatString(vkFormat));
//            // }
//            //
//            // if (formatInfo.isBlockCompressed) {
//            //     if (header.typeSize != 1)
//            //         error(HeaderData::TypeSizeNotOne);
//            //     if (header.levelCount == 0)
//            //         error(HeaderData::ZeroLevelCountForBC);
//            // } else {
//            //     if (header.typeSize != formatInfo.wordSize)
//            //         error(HeaderData::TypeSizeMismatch, header.typeSize);
//            // }
//         }
//
//     // We are right after the levelIndex. We've already checked that
//     // header.dataFormatDescriptor.byteOffset points to this location.
//     ctx.pActualDfd = new uint32_t[ctx.header.dataFormatDescriptor.byteLength
//                                / sizeof(uint32_t)];
//     ctx.inp->read((char *)ctx.pActualDfd,
//                   ctx.header.dataFormatDescriptor.byteLength);
//     if (ctx.inp->fail())
//         addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
//     else if (ctx.inp->eof())
//         addIssue(logger::eFatal, IOError.UnexpectedEOF);
//
//     if (ctx.header.dataFormatDescriptor.byteLength != *ctx.pActualDfd)
//         addIssue(logger::eError, DFD.SizeMismatch);
//
//     uint32_t* bdb = ctx.pActualDfd + 1; // Basic descriptor block.
//
//     uint32_t xferFunc;
//     if ((xferFunc = KHR_DFDVAL(bdb, TRANSFER)) != KHR_DF_TRANSFER_SRGB
//         && xferFunc != KHR_DF_TRANSFER_LINEAR)
//         addIssue(logger::eError, DFD.InvalidTransferFunction);
//
//     bool analyze = false;
//     uint32_t numSamples = KHR_DFDSAMPLECOUNT(bdb);
//     switch (ctx.header.supercompressionScheme) {
//       case KTX_SS_NONE:
//       case KTX_SS_ZSTD:
//         if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED) {
//             if (ctx.header.supercompressionScheme != KTX_SS_ZSTD) {
//                 // Do a simple comparison with the expected DFD.
//                 analyze = memcmp(ctx.pActualDfd, ctx.pDfd4Format,
//                                   *ctx.pDfd4Format);
//             } else {
//                 // Compare up to BYTESPLANE.
//                 analyze = memcmp(ctx.pActualDfd, ctx.pDfd4Format,
//                                   KHR_DF_WORD_BYTESPLANE0 * 4);
//                 // Check for unsized.
//                 if (bdb[KHR_DF_WORD_BYTESPLANE0]  != 0
//                     || bdb[KHR_DF_WORD_BYTESPLANE4]  != 0)
//                     addIssue(logger::eError, DFD.NotUnsized);
//                 // Compare the sample information.
//                 if (!analyze) {
//                     analyze = memcmp(&ctx.pActualDfd[KHR_DF_WORD_SAMPLESTART+1],
//                                     &ctx.pDfd4Format[KHR_DF_WORD_SAMPLESTART+1],
//                                     numSamples * KHR_DF_WORD_SAMPLEWORDS);
//                 }
//             }
//         } else {
//             if (KHR_DFDVAL(bdb, MODEL) == KHR_DF_MODEL_UASTC) {
//                 // Validate UASTC
//                 if (numSamples == 0)
//                     addIssue(logger::eError, DFD.ZeroSamples, "UASTC");
//                 if (numSamples > 1)
//                     addIssue(logger::eError, DFD.InvalidSampleCount,
//                              "UASTC", "1");
//                 if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) != 3
//                     && KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) != 3
//                     && (bdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] & 0xffff0000) != 0)
//                     addIssue(logger::eError, DFD.InvalidTexelBlockDimension,
//                              4, 4, "UASTC");
//                 uint32_t bytesPlane0 = KHR_DFDVAL(bdb, BYTESPLANE0);
//                 if (ctx.header.supercompressionScheme == KTX_SS_NONE) {
//                     if (bytesPlane0 != 16) {
//                         addIssue(logger::eError, DFD.BytesPlane0Mismatch,
//                                  bytesPlane0, 16);
//                     }
//                 } else {
//                      if (bytesPlane0 != 0) {
//                           addIssue(logger::eError, DFD.NotUnsized, "UASTC");
//                      }
//                 }
//                 uint8_t channelID = KHR_DFDSVAL(bdb, 0, CHANNELID);
//                 if (channelID != KHR_DF_CHANNEL_UASTC_RGB
//                     && channelID != KHR_DF_CHANNEL_UASTC_RGBA
//                     && channelID != KHR_DF_CHANNEL_UASTC_RRR
//                     && channelID != KHR_DF_CHANNEL_UASTC_RRRG)
//                     addIssue(logger::eError, DFD.InvalidChannelForUASTC);
//                 if (KHR_DFDSVAL(bdb, 0, BITOFFSET) != 0)
//                     addIssue(logger::eError, DFD.InvalidBitOffsetForUASTC);
//                 if (KHR_DFDSVAL(bdb, 0, BITLENGTH) != 127)
//                     addIssue(logger::eError, DFD.InvalidBitLength,
//                              "UASTC", 127);
//                 if (KHR_DFDSVAL(bdb, 0, SAMPLELOWER) != 0
//                     && KHR_DFDSVAL(bdb, 0, SAMPLEUPPER) != UINT32_MAX)
//                     addIssue(logger::eError, DFD.InvalidLowerOrUpper, "UASTC");
//             } else {
//                 // Check the basics
//                 if (KHR_DFDVAL(bdb, VENDORID) != KHR_DF_VENDORID_KHRONOS
//                     || KHR_DFDVAL(bdb, DESCRIPTORTYPE) != KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT
//                     || KHR_DFDVAL(bdb, VERSIONNUMBER) < KHR_DF_VERSIONNUMBER_1_3)
//                     addIssue(logger::eError, DFD.IncorrectBasics);
//
//                 // Ensure there are at least some samples
//                 if (KHR_DFDSAMPLECOUNT(bdb) == 0)
//                     addIssue(logger::eError, DFD.ZeroSamples,
//                              "non-supercompressed texture with VK_FORMAT_UNDEFINED");
//                 // Check for properly sized format
//                 // This checks texelBlockDimension[0-3] and bytesPlane[0-7]
//                 // as each is a byte and bdb is unit32_t*.
//                 if (bdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] == 0)
//                     addIssue(logger::eError, DFD.TexelBlockDimensionZeroForUndefined);
//                 if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION3) != 0)
//                     addIssue(logger::eError, DFD.FourDimensionalTexturesNotSupported);
//                 if (ctx.header.supercompressionScheme == KTX_SS_NONE) {
//                     if (KHR_DFDVAL(bdb, BYTESPLANE0) == 0)
//                         addIssue(logger::eError, DFD.BytesPlane0Zero,
//                                  "VK_FORMAT_UNDEFINED");
//                 } else {
//                      if (KHR_DFDVAL(bdb, BYTESPLANE0) != 0) {
//                           addIssue(logger::eError, DFD.NotUnsized);
//                      }
//                 }
//                 if ((bdb[KHR_DF_WORD_BYTESPLANE0] & KHR_DF_MASK_BYTESPLANE0) != 0
//                     || bdb[KHR_DF_WORD_BYTESPLANE4] != 0)
//                     addIssue(logger::eError, DFD.MultiplaneFormatsNotSupported);
//             }
//         }
//         break;
//
//       case KTX_SS_BASIS_LZ:
//           // validateHeader has already checked if vkFormat is the required
//           // VK_FORMAT_UNDEFINED so no check here.
//
//           // The colorModel must be ETC1S, currently the only format supported
//           // with BasisLZ.
//           if (KHR_DFDVAL(bdb, MODEL) != KHR_DF_MODEL_ETC1S)
//               addIssue(logger::eError, DFD.IncorrectModelForBLZE);
//           // This descriptor should have 1 or 2 samples with bitLength 63
//           // and bitOffsets 0 and 64.
//           if (numSamples == 0)
//               addIssue(logger::eError, DFD.ZeroSamples, "BasisLZ/ETC1S");
//           if (numSamples > 2)
//               addIssue(logger::eError, DFD.InvalidSampleCount, "BasisLZ/ETC1S", "1 or 2");
//           if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) != 3
//               && KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) != 3
//               && (bdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] & 0xffff0000) != 0)
//               addIssue(logger::eError, DFD.InvalidTexelBlockDimension,
//                        4, 4, "BasisLZ/ETC1S");
//           // Check for unsized.
//           if (bdb[KHR_DF_WORD_BYTESPLANE0]  != 0
//               || bdb[KHR_DF_WORD_BYTESPLANE4]  != 0)
//               addIssue(logger::eError, DFD.NotUnsized);
//
//           for (uint32_t sample = 0; sample < numSamples; sample++) {
//               uint8_t channelID = KHR_DFDSVAL(bdb, sample, CHANNELID);
//               if (channelID != KHR_DF_CHANNEL_ETC1S_RGB
//                   && channelID != KHR_DF_CHANNEL_ETC1S_RRR
//                   && channelID != KHR_DF_CHANNEL_ETC1S_GGG
//                   && channelID != KHR_DF_CHANNEL_ETC1S_AAA)
//                   addIssue(logger::eError, DFD.InvalidChannelForBLZE);
//               int bo = KHR_DFDSVAL(bdb, sample, BITOFFSET);
//               //if (KHR_DFDSVAL(bdb, sample, BITOFFSET) != sample == 0 ? 0 : 64)
//               if (bo != (sample == 0 ? 0 : 64))
//                   addIssue(logger::eError, DFD.InvalidBitOffsetForBLZE);
//               if (KHR_DFDSVAL(bdb, sample, BITLENGTH) != 63)
//                   addIssue(logger::eError, DFD.InvalidBitLength,
//                            "BasisLZ/ETC1S", 63);
//               if (KHR_DFDSVAL(bdb, sample, SAMPLELOWER) != 0
//                   && KHR_DFDSVAL(bdb, sample, SAMPLEUPPER) != UINT32_MAX)
//                   addIssue(logger::eError, DFD.InvalidLowerOrUpper,
//                            "BasisLZ/ETC1S");
//           }
//           break;
//
//       default:
//         break;
//     }
//
//     if (analyze) {
//         // ctx.pActualDfd differs from what is expected. To help developers, do
//         // a more in depth analysis.
//
//         string vkFormatStr(vkFormatString(static_cast<VkFormat>(ctx.header.vkFormat)));
//         uint32_t* expBdb = ctx.pDfd4Format + 1; // Expected basic block.
//
//         if (KHR_DFDVAL(bdb, VENDORID) != KHR_DF_VENDORID_KHRONOS
//             || KHR_DFDVAL(bdb, DESCRIPTORTYPE) != KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT
//             || KHR_DFDVAL(bdb, VERSIONNUMBER) < KHR_DF_VERSIONNUMBER_1_3)
//             addIssue(logger::eError, DFD.IncorrectBasics);
//
//         khr_df_primaries_e aPrim, ePrim;
//         aPrim = (khr_df_primaries_e)KHR_DFDVAL(bdb, PRIMARIES);
//         ePrim = (khr_df_primaries_e)KHR_DFDVAL(expBdb, PRIMARIES);
//         if (aPrim != ePrim) {
//             // Okay. Any valid PRIMARIES value can be used. Check validity.
//             if (aPrim < 0 || aPrim > KHR_DF_PRIMARIES_ADOBERGB)
//                  addIssue(logger::eError, DFD.InvalidPrimaries, aPrim);
//         }
//
//         // Don't check flags because all the expected DFDs we create have
//         // ALPHA_STRAIGHT but ALPHA_PREMULTIPLIED is also valid.
//
//         int aVal, eVal;
//         if (KHR_DFDSAMPLECOUNT(bdb) == 0) {
//             addIssue(logger::eError, DFD.ZeroSamples, vkFormatStr.c_str());
//         } else {
//             aVal = KHR_DFDSAMPLECOUNT(bdb);
//             eVal = KHR_DFDSAMPLECOUNT(expBdb);
//             if (aVal != eVal)
//                 addIssue(logger::eError, DFD.SampleCountMismatch, aVal, eVal);
//         }
//
//         if (ctx.header.supercompressionScheme == KTX_SS_NONE) {
//             // bP0 for supercompressed has already been checked.
//             aVal = KHR_DFDVAL(bdb, BYTESPLANE0);
//             eVal = KHR_DFDVAL(expBdb, BYTESPLANE0);
//             if (aVal != eVal) {
//                 if (aVal == 0)
//                     addIssue(logger::eError, DFD.BytesPlane0Zero,
//                              vkFormatStr.c_str());
//                 else
//                     addIssue(logger::eError, DFD.BytesPlane0Mismatch, aVal, eVal);
//             }
//         }
//
//         if (ctx.formatInfo.isBlockCompressed) {
//             // _BLOCK formats.
//             if (KHR_DFDVAL(bdb, MODEL) < KHR_DF_MODEL_DXT1A)
//               addIssue(logger::eError, DFD.IncorrectModelForBlock);
//         } else {
//             InterpretedDFDChannel r, g, b, a;
//             uint32_t componentByteLength;
//             InterpretDFDResult result;
//
//             result = interpretDFD(ctx.pActualDfd, &r, &g, &b, &a, &componentByteLength);
//             if (result > i_UNSUPPORTED_ERROR_BIT) {
//                 switch (result) {
//                   case i_UNSUPPORTED_CHANNEL_TYPES:
//                     addIssue(logger::eError, DFD.InvalidColorModel);
//                     break;
//                   case i_UNSUPPORTED_MULTIPLE_PLANES:
//                     addIssue(logger::eError, DFD.MultiplePlanes);
//                     break;
//                   case i_UNSUPPORTED_MIXED_CHANNELS:
//                     addIssue(logger::eError, DFD.MixedChannels);
//                     break;
//                   case i_UNSUPPORTED_MULTIPLE_SAMPLE_LOCATIONS:
//                     addIssue(logger::eError, DFD.Multisample);
//                     break;
//                   case i_UNSUPPORTED_NONTRIVIAL_ENDIANNESS:
//                     addIssue(logger::eError, DFD.NonTrivialEndianness);
//                     break;
//                   default:
//                     break;
//                 }
//             } else {
//                 if ((result & i_FLOAT_FORMAT_BIT) && !(result & i_SIGNED_FORMAT_BIT))
//                     addIssue(logger::eWarning, DFD.UnsignedFloat);
//
//                 if (result & i_SRGB_FORMAT_BIT) {
//                     if (vkFormatStr.find("SRGB") == string::npos)
//                         addIssue(logger::eError, DFD.sRGBMismatch);
//                 } else {
//                     string findStr;
//                     if (result & i_SIGNED_FORMAT_BIT)
//                         findStr += 'S';
//                     else
//                         findStr += 'U';
//
//                     if (result & i_FLOAT_FORMAT_BIT)
//                         findStr += "FLOAT";
//                     // else here because Vulkan format names do not reflect
//                     // both normalized and float. E.g, BC6H is just
//                     // VK_FORMAT_BC6H_[SU]FLOAT_BLOCK.
//                     else if (result & i_NORMALIZED_FORMAT_BIT)
//                         findStr += "NORM";
//                     else
//                         findStr += "INT";
//
//                     if (vkFormatStr.find(findStr) == string::npos)
//                         addIssue(logger::eError, DFD.FormatMismatch);
//                 }
//             }
//         }
//     }
// }
//
// =================================================================================================

void ValidationContext::validateKVD() {
    seek_to(header.keyValueData.byteOffset, "the KVD");

    if (header.keyValueData.byteLength == 0)
        return; // There is no KVD block

    const auto buffer = std::make_unique<uint8_t[]>(header.keyValueData.byteLength);
    read(buffer.get(), header.keyValueData.byteLength, "the KVD");
    const auto* ptrKVD = buffer.get();
    const auto* ptrKVDEnd = ptrKVD + header.keyValueData.byteLength;

    struct KeyValueEntry {
        std::string key;
        const uint8_t* data;
        uint32_t size;

        KeyValueEntry(std::string_view key, const uint8_t* data, uint32_t size) :
                key(key), data(data), size(size) {}
    };
    std::vector<KeyValueEntry> entries;

    int numEntry = 0;
    // Process Key-Value entries {size, key, \0, value} until the end of the KVD block
    // Where size is an uint32_t, and it equals to: sizeof(key) + 1 + sizeof(value)
    const auto* ptrEntry = ptrKVD;
    while (ptrEntry < ptrKVDEnd) {
        ++numEntry;
        if (numEntry > MAX_NUM_KV_ENTRY) {
            error(Metadata::TooManyEntry, MAX_NUM_KV_ENTRY);
            break;
        }

        if (ptrKVDEnd - ptrEntry < 6) {
            error(Metadata::NotEnoughDataForAnEntry, ptrKVDEnd - ptrEntry);
            // The spec requires at least 6 byte per entry, but this validator is able to proceed with only 4
            // 4 byte size + 1 byte key + 1 byte \0
            if (ptrKVDEnd - ptrEntry < 4)
                break;
        }

        uint32_t sizeKeyValuePair;
        std::memcpy(&sizeKeyValuePair, ptrEntry, sizeof(uint32_t));

        const auto* ptrKeyValuePair = ptrEntry + sizeof(uint32_t);
        const auto* ptrKey = ptrKeyValuePair;

        if (ptrKeyValuePair + sizeKeyValuePair > ptrKVDEnd) {
            const auto bytesLeft = ptrKVDEnd - ptrKeyValuePair;
            error(Metadata::KeyValuePairSizeTooBig, sizeKeyValuePair, bytesLeft);
            sizeKeyValuePair = static_cast<uint32_t>(bytesLeft); // Attempt recovery to read out at least the key
        }

        if (sizeKeyValuePair < 2)
            error(Metadata::KeyValuePairSizeTooSmall, sizeKeyValuePair);

        // Determine key, finding the null terminator
        uint32_t sizeKey = 0;
        while (sizeKey < sizeKeyValuePair && ptrKey[sizeKey] != '\0')
            ++sizeKey;

        const auto keyHasNullTerminator = sizeKey != sizeKeyValuePair;
        auto key = std::string_view(reinterpret_cast<const char*>(ptrKey), sizeKey);

        // Determine the value
        const auto* ptrValue = ptrKey + sizeKey + 1;
        const auto sizeValue = keyHasNullTerminator ? sizeKeyValuePair - sizeKey - 1 : 0;

        // Check for BOM
        if (starts_with(key, "\xEF\xBB\xBF")) {
            key.remove_prefix(3);
            error(Metadata::KeyForbiddenBOM, key);
        }

        if (auto invalidIndex = validateUTF8(key))
            error(Metadata::KeyInvalidUTF8, key, *invalidIndex);

        if (!keyHasNullTerminator)
            error(Metadata::KeyMissingNullTerminator, key);

        entries.emplace_back(key, ptrValue, sizeValue);

        // Finish entry
        ptrEntry += sizeof(uint32_t) + sizeKeyValuePair;
        validatePaddingZeros(ptrEntry, ptrKVDEnd, 4, Metadata::PaddingNotZero, "after a Key-Value entry");
        ptrEntry = align(ptrEntry, 4);
    }

    if (ptrEntry != ptrKVDEnd)
        // Being super explicit about the specs. This check might be overkill as other checks often cover this case
        error(Metadata::SizesDontAddUp, ptrEntry - ptrKVD, header.keyValueData.byteLength);

    if (header.supercompressionGlobalData.byteLength != 0)
        validatePaddingZeros(ptrEntry, ptrKVDEnd, 8, Metadata::PaddingNotZero, "between KVD and SGD");

    if (!is_sorted(entries, std::less<>{}, &KeyValueEntry::key)) {
        error(Metadata::OutOfOrder);
        sort(entries, std::less<>{}, &KeyValueEntry::key);
    }

    if (!is_unique(entries, &KeyValueEntry::key))
        error(Metadata::DuplicateKey);

    using MemberFN = void(ValidationContext::*)(std::string_view, const uint8_t*, uint32_t);
    std::unordered_map<std::string, MemberFN> kvValidators;

    kvValidators.emplace("KTXcubemapIncomplete", &ValidationContext::validateKVCubemapIncomplete);
    kvValidators.emplace("KTXorientation", &ValidationContext::validateKVOrientation);
    kvValidators.emplace("KTXglFormat", &ValidationContext::validateKVGlFormat);
    kvValidators.emplace("KTXdxgiFormat__", &ValidationContext::validateKVDxgiFormat);
    kvValidators.emplace("KTXmetalPixelFormat", &ValidationContext::validateKVMetalPixelFormat);
    kvValidators.emplace("KTXswizzle", &ValidationContext::validateKVSwizzle);
    kvValidators.emplace("KTXwriter", &ValidationContext::validateKVWriter);
    kvValidators.emplace("KTXwriterScParams", &ValidationContext::validateKVWriterScParams);
    kvValidators.emplace("KTXastcDecodeMode", &ValidationContext::validateKVAstcDecodeMode);
    kvValidators.emplace("KTXanimData", &ValidationContext::validateKVAnimData);

    for (const auto& entry : entries) {
        const auto it = kvValidators.find(entry.key);
        if (it == kvValidators.end()) {
            if (starts_with(entry.key, "KTX") || starts_with(entry.key, "ktx"))
                error(Metadata::UnknownReservedKey, entry.key);
            else
                warning(Metadata::CustomMetadata, entry.key);

            continue;
        }

        (this->*it->second)(entry.key, entry.data, entry.size);
    }

    // TODO Tools P5: error on KTXanimData with KTXcubemapIncomplete
    // if (foundKTXanimData && foundKTXcubemapIncomplete)
    //     error(Metadata.NotAllowed, key, "together with KTXcubemapIncomplete");

    if (!foundKTXwriter) {
        if (foundKTXwriterScParams)
            error(Metadata::NoRequiredKTXwriter);
        else
            warning(Metadata::NoKTXwriter);
    }
}

void ValidationContext::validateKVCubemapIncomplete(std::string_view key, const uint8_t* data, uint32_t size) {
    (void) key;
    foundKTXcubemapIncomplete = true;

    if (size != 1)
        error(Metadata::KTXcubemapIncompleteInvalidSize, size);

    if (size == 0)
        return;

    uint8_t value = *data;

    if (size > 0 && (value & 0b11000000u) != 0)
        error(Metadata::KTXcubemapIncompleteInvalidValue, value);

    value = value & 0b00111111u; // Error recovery

    const auto popCount = popcount(value & 0b11000000u);
    if (popCount == 6)
        warning(Metadata::KTXcubemapIncompleteAllBitSet);

    if (popCount == 0)
        error(Metadata::KTXcubemapIncompleteNoBitSet);

    if (popCount != 0 && (layerCount % popCount != 0))
        error(Metadata::KTXcubemapIncompleteIncompatibleLayerCount, header.layerCount, popCount);

    if (header.faceCount != 1)
        error(Metadata::KTXcubemapIncompleteWithFaceCountNot1, header.faceCount);

    if (header.pixelHeight != header.pixelWidth)
        error(HeaderData::CubeHeightWidthMismatch, header.pixelWidth, header.pixelHeight);

    if (header.pixelDepth != 0)
        error(HeaderData::CubeWithDepth, header.pixelDepth);
}

void ValidationContext::validateKVOrientation(std::string_view key, const uint8_t* data, uint32_t size) {
    if (size < 3 || size > 5) {
        error(Metadata::KTXorientationInvalidSize, size);
        return;
    }

    (void) key;
    (void) data;
    // string orientation;
    // const char* pOrientation = reinterpret_cast<const char*>(value);
    // if (value[valueLen - 1] != '\0') {
    //     // regex_match on some platforms will fail to match an otherwise
    //     // valid swizzle due to lack of a NUL terminator even IF there is
    //     // no '$' at the end of the regex. Make a copy to avoid this.
    //     orientation.assign(pOrientation, valueLen);
    //     pOrientation = orientation.c_str();
    //     addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
    // }
    //
    // if (valueLen != ctx.dimensionCount + 1)
    //     addIssue(logger::eError, Metadata.InvalidValue, key);
    //
    // switch (ctx.dimensionCount) {
    //   case 1:
    //     if (!regex_match (pOrientation, regex("^[rl]$") ))
    //         addIssue(logger::eError, Metadata.InvalidValue, key);
    //     break;
    //   case 2:
    //     if (!regex_match(pOrientation, regex("^[rl][du]$")))
    //         addIssue(logger::eError, Metadata.InvalidValue, key);
    //     break;
    //   case 3:
    //     if (!regex_match(pOrientation, regex("^[rl][du][oi]$")))
    //         addIssue(logger::eError, Metadata.InvalidValue, key);
    //     break;
    // }
}

void ValidationContext::validateKVGlFormat(std::string_view key, const uint8_t* data, uint32_t size) {
    if (size != 12)
        error(Metadata::InvalidSizeKTXglFormat, size);

    (void) key;
    (void) data;
}

void ValidationContext::validateKVDxgiFormat(std::string_view key, const uint8_t* data, uint32_t size) {
    if (size != 4)
        error(Metadata::InvalidSizeKTXdxgiFormat, size);

    (void) key;
    (void) data;
}

void ValidationContext::validateKVMetalPixelFormat(std::string_view key, const uint8_t* data, uint32_t size) {
    if (size != 4)
        error(Metadata::InvalidSizeKTXmetalPixelFormat, size);

    (void) key;
    (void) data;
}

void ValidationContext::validateKVSwizzle(std::string_view key, const uint8_t* data, uint32_t size) {
    if (size != 5)
        error(Metadata::InvalidSizeKTXswizzle, size);

    (void) key;
    (void) data;
    // string swizzle;
    // const char* pSwizzle = reinterpret_cast<const char*>(value);
    // if (value[valueLen - 1] != '\0') {
    //     addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
    //     // See comment in validateOrientation.
    //     swizzle.assign(pSwizzle, valueLen);
    //     pSwizzle = swizzle.c_str();
    // }
    // if (!regex_match(pSwizzle, regex("^[rgba01]{4}$")))
    //     addIssue(logger::eError, Metadata.InvalidValue, key);
}

void ValidationContext::validateKVWriter(std::string_view key, const uint8_t* data, uint32_t size) {
    foundKTXwriter = true;

    (void) key;
    (void) data;
    (void) size;
    // if (data[size - 1] != '\0')
    //     addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
}

void ValidationContext::validateKVWriterScParams(std::string_view key, const uint8_t* data, uint32_t size) {
    foundKTXwriterScParams = true;

    (void) key;
    (void) data;
    (void) size;
    // if (value[valueLen-1] != '\0')
    //     addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
}

void ValidationContext::validateKVAstcDecodeMode(std::string_view key, const uint8_t* data, uint32_t size) {
    (void) key;
    (void) data;
    (void) size;
    // if (valueLen == 0) {
    //     addIssue(logger::eError, Metadata.MissingValue, key);
    //     return;
    // }
    //
    // if (!regex_match((char*)value, regex("rgb9e5"))
    //    && !regex_match((char*)value, regex("unorm8")))
    //      addIssue(logger::eError, Metadata.InvalidValue, key);
    //
    // if (!ctx.pActualDfd)
    //     return;
    //
    // uint32_t* bdb = ctx.pDfd4Format + 1;
    // if (KHR_DFDVAL(bdb, MODEL) != KHR_DF_MODEL_ASTC) {
    //      addIssue(logger::eError, Metadata.NotAllowed, key,
    //               "for non-ASTC texture formats");
    // }
    // if (KHR_DFDVAL(bdb, TRANSFER) == KHR_DF_TRANSFER_SRGB) {
    //      addIssue(logger::eError, Metadata.NotAllowed, key,
    //               "with sRGB transfer function");
    // }
}

void ValidationContext::validateKVAnimData(std::string_view key, const uint8_t* data, uint32_t size) {
    foundKTXanimData = true;

    if (size != 12)
        error(Metadata::InvalidSizeKTXanimData, size);

    (void) key;
    (void) data;

    // if (ctx.layerCount == 0)
    //     addIssue(logger::eError, Metadata.NotAllowed, key,
    //              "except with array textures");
}

// =================================================================================================
// TODO Tools P2: Validate SGD
//
// void
// ktxValidator::validateSgd(validationContext& ctx)
// {
//     uint64_t sgdByteLength = ctx.header.supercompressionGlobalData.byteLength;
//     if (ctx.header.supercompressionScheme == KTX_SS_BASIS_LZ) {
//         if (sgdByteLength == 0) {
//             addIssue(logger::eError, SGD.MissingSupercompressionGlobalData);
//             return;
//         }
//     } else {
//         if (sgdByteLength > 0)
//             addIssue(logger::eError, SGD.UnexpectedSupercompressionGlobalData);
//         return;
//     }
//
//     uint8_t* sgd = new uint8_t[sgdByteLength];
//     ctx.inp->read((char *)sgd, sgdByteLength);
//     if (ctx.inp->fail())
//         addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
//     else if (ctx.inp->eof())
//         addIssue(logger::eFatal, IOError.UnexpectedEOF);
//
//     // firstImages contains the indices of the first images for each level.
//     // The last array entry contains the total number of images which is what
//     // we need here.
//     uint32_t* firstImages = new uint32_t[ctx.levelCount+1];
//     // Temporary invariant value
//     uint32_t layersFaces = ctx.layerCount * ctx.header.faceCount;
//     firstImages[0] = 0;
//     for (uint32_t level = 1; level <= ctx.levelCount; level++) {
//         // NOTA BENE: numFaces * depth is only reasonable because they can't
//         // both be > 1. I.e there are no 3d cubemaps.
//         firstImages[level] = firstImages[level - 1]
//                            + layersFaces * MAX(ctx.header.pixelDepth >> (level - 1), 1);
//     }
//     uint32_t& imageCount = firstImages[ctx.levelCount];
//
//     ktxBasisLzGlobalHeader& bgdh = *reinterpret_cast<ktxBasisLzGlobalHeader*>(sgd);
//     uint32_t numSamples = KHR_DFDSAMPLECOUNT(ctx.pActualDfd + 1);
//
//     uint64_t expectedBgdByteLength = sizeof(ktxBasisLzGlobalHeader)
//                                    + sizeof(ktxBasisLzEtc1sImageDesc) * imageCount
//                                    + bgdh.endpointsByteLength
//                                    + bgdh.selectorsByteLength
//                                    + bgdh.tablesByteLength;
//
//     ktxBasisLzEtc1sImageDesc* imageDescs = BGD_ETC1S_IMAGE_DESCS(sgd);
//     ktxBasisLzEtc1sImageDesc* image = imageDescs;
//     for (; image < imageDescs + imageCount; image++) {
//       if (image->imageFlags & ~ETC1S_P_FRAME)
//             addIssue(logger::eError, SGD.InvalidImageFlagBit);
//         // Crosscheck the DFD.
//         if (image->alphaSliceByteOffset == 0 && numSamples == 2)
//             addIssue(logger::eError, SGD.DfdMismatchAlpha);
//         if (image->alphaSliceByteOffset > 0 && numSamples == 1)
//             addIssue(logger::eError, SGD.DfdMismatchNoAlpha);
//     }
//
//     if (sgdByteLength != expectedBgdByteLength)
//         addIssue(logger::eError, SGD.IncorrectGlobalDataSize);
//
//     if (bgdh.extendedByteLength != 0)
//         addIssue(logger::eError, SGD.ExtendedByteLengthNotZero);
//
//     // Can't do anymore as we have no idea how many endpoints, etc there
//     // should be.
//     // TODO: attempt transcode
// }
//
// void
// ktxValidator::validateDataSize(validationContext& ctx)
// {
//     // Expects to be called after validateSgd so current file offset is at
//     // the start of the data.
//     uint64_t dataSizeInFile;
//     off_t dataStart = (off_t)(ctx.inp->tellg());
//
//     ctx.inp->seekg(0, ios_base::end);
//     if (ctx.inp->fail())
//         addIssue(logger::eFatal, IOError.FileSeekEndFailure,
//                  strerror(errno));
//     off_t dataEnd = (off_t)(ctx.inp->tellg());
//     if (dataEnd < 0)
//         addIssue(logger::eFatal, IOError.FileTellFailure, strerror(errno));
//     dataSizeInFile = dataEnd - dataStart;
//     if (dataSizeInFile != ctx.dataSizeFromLevelIndex)
//         addIssue(logger::eError, FileError.IncorrectDataSize);
// }
//
// // Must be called last as it rewinds the file.
// bool
// ktxValidator::validateTranscode(validationContext& ctx)
// {
//     uint32_t* bdb = ctx.pActualDfd + 1; // Basic descriptor block.
//     uint32_t model = KHR_DFDVAL(bdb, MODEL);
//     if (model != KHR_DF_MODEL_UASTC && model != KHR_DF_MODEL_ETC1S) {
//         // Nothin to do. Not transcodable.
//         return true;
//     }
//
//     bool retval;
//     istream& is = *ctx.inp;
//     is.seekg(0);
//     streambuf* _streambuf = (is.rdbuf());
//     StreambufStream<streambuf*> ktx2Stream(_streambuf, ios::in);
//     KtxTexture<ktxTexture2> texture2;
//     ktx_error_code_e result = ktxTexture2_CreateFromStream(ktx2Stream.stream(),
//                                         KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
//                                         texture2.pHandle());
//     if (result != KTX_SUCCESS) {
//         addIssue(logger::eError, FileError.CreateFailure,
//                  ktxErrorString(result));
//         retval = false;
//     }
//
//     if (model == KHR_DF_MODEL_ETC1S)
//         result = ktxTexture2_TranscodeBasis(texture2.handle(),
//                                             KTX_TTF_ETC2_RGBA, 0);
//     else
//         result = ktxTexture2_TranscodeBasis(texture2.handle(),
//                                             KTX_TTF_ASTC_4x4_RGBA,  0);
//     if (result != KTX_SUCCESS) {
//         addIssue(logger::eError, Transcode.Failure, ktxErrorString(result));
//         retval = false;
//     } else {
//         retval = true;
//     }
//     return retval;
// }
//
// =================================================================================================

} // namespace ktx
