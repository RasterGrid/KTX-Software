// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include <fmt/format.h>
#include <fmt/printf.h>
#include <KHR/khr_df.h>
#include <ktx.h>
#include "ktxint.h"

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

struct ValidationContext {
private:
    const char* data = nullptr;
    std::size_t size = 0;
    const char* it = nullptr;

private:
    std::function<void(const ValidationReport&)> callback;

    bool treatWarningsAsError = false;
    // bool checkGLTFBasisu = false; // TODO Tools P2: implement checkGLTFBasisu

    uint32_t numError = 0;
    uint32_t numWarning = 0;

public:
    KTX_header2 header{};

    uint32_t layerCount = 0;
    uint32_t levelCount = 0;
    uint32_t dimensionCount = 0;

public:
    ValidationContext(bool warningsAsErrors, std::function<void(const ValidationReport&)> callback) :
        callback(std::move(callback)),
        treatWarningsAsError(warningsAsErrors) {
        std::memset(&header, 0, sizeof(header));
    }

private:
    template <typename... Args>
    void warning(const IssueWarning& issue, Args&&... args) {
        if (treatWarningsAsError) {
            ++numError;
            callback(ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});

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
    void read(void* readDst, std::size_t readSize, std::string_view name) {
        if (it + readSize >= data + size)
            fatal(IOError::UnexpectedEOF, readSize, readSize - (data + size - it), name);

        std::memcpy(readDst, it, readSize);
    }

public:
    int validateFile(const _tstring& filepath);
    int validateStream(FILE* file);
    int validateMemory(const char* data, std::size_t size);

private:
    int validate();
    void validateHeader();
    void validateIndices();
    // void validate...();
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
    data = data_;
    size = size_;
    it = data_;
    return validate();
}

// -------------------------------------------------------------------------------------------------

int ValidationContext::validate() {
    validateHeader();
    validateIndices();
    // validate...();

    return numError > 0 ? 3 : 0;
}

void ValidationContext::validateHeader() {
    static constexpr uint8_t ktx2_identifier_reference[12] = KTX2_IDENTIFIER_REF;

    read(&header, sizeof(KTX_header2), "the header");
    const auto vkFormat = static_cast<VkFormat>(header.vkFormat);

    // Validate file identifier
    if (std::memcmp(&header.identifier, ktx2_identifier_reference, 12) != 0)
        fatal(FileError::NotKTX2);

    // Validate vkFormat
    if (isProhibitedFormat(vkFormat))
        error(HeaderData::ProhibitedFormat, toStringVkFormat(vkFormat));

    if (!isValidFormat(vkFormat)) {
        if (header.vkFormat <= VK_FORMAT_MAX_STANDARD_ENUM || header.vkFormat > 0x10010000)
            error(HeaderData::InvalidFormat, toStringVkFormat(vkFormat));
        else
            warning(HeaderData::UnknownFormat, toStringVkFormat(vkFormat));
    }

    if (header.supercompressionScheme == KTX_SS_BASIS_LZ) {
        if (header.vkFormat != VK_FORMAT_UNDEFINED)
            error(HeaderData::VkFormatAndBasis, toStringVkFormat(vkFormat));
    }

    // Validate typeSize
    if (header.vkFormat == VK_FORMAT_UNDEFINED) {
        if (header.typeSize != 1)
            error(HeaderData::TypeSizeNotOne, header.typeSize, toStringVkFormat(vkFormat));

    } else {
        if (header.supercompressionScheme != KTX_SS_BASIS_LZ) {
            // TODO Tools P4: Clarify and implement these conditions

            // createDfd4Format();
            // if (pDfd4Format == nullptr) {
            //     fatal(ValidatorError::CreateDfdFailure, vkFormatString(vkFormat));
            // } else if (!extractFormatInfo(pDfd4Format)) {
            //     error(ValidatorError::IncorrectDfd, vkFormatString(vkFormat));
            // }
            //
            // if (formatInfo.isBlockCompressed) {
            //     if (header.typeSize != 1)
            //         error(HeaderData::TypeSizeNotOne);
            //     if (header.levelCount == 0)
            //         error(HeaderData::ZeroLevelCountForBC);
            // } else {
            //     if (header.typeSize != formatInfo.wordSize)
            //         error(HeaderData::TypeSizeMismatch, header.typeSize);
            // }
        }
    }

    // Validate image dimensions
    if (header.pixelWidth == 0)
        error(HeaderData::WidthZero);

    if (isFormatBlockCompressed(vkFormat))
        if (header.pixelHeight == 0)
            error(HeaderData::BlockCompressedNoHeight, toStringVkFormat(vkFormat));

    if (header.faceCount == 6)
        if (header.pixelWidth != header.pixelHeight)
            error(HeaderData::CubeHeightWidthMismatch, header.pixelWidth, header.pixelHeight);

    if (header.pixelDepth != 0 && header.pixelHeight == 0)
        error(HeaderData::DepthNoHeight, header.pixelDepth);

    if (isFormat3DBlockCompressed(vkFormat))
        if (header.pixelDepth == 0)
            error(HeaderData::DepthBlockCompressedNoDepth, toStringVkFormat(vkFormat));

    if (isFormatDepth(vkFormat))
        if (header.pixelDepth != 0)
            error(HeaderData::DepthFormatWithDepth, header.pixelDepth, toStringVkFormat(vkFormat));

    if (isFormatStencil(vkFormat))
        if (header.pixelDepth != 0)
            error(HeaderData::StencilFormatWithDepth, header.pixelDepth, toStringVkFormat(vkFormat));

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
    // Cube map 2D faces are validated by: CubeHeightWidthMismatch and CubeWithDepth

    // Validate levelCount
    levelCount = std::max(header.levelCount, 1u);

    // This test works for arrays too because height or depth will be 0.
    const auto max_dim = std::max(std::max(header.pixelWidth, header.pixelHeight), header.pixelDepth);
    // TODO Tools P4: Verify 'log2'
    if (max_dim < ((uint32_t) 1 << (levelCount - 1))) {
        // Can't have more mip levels than 1 + log2(max(width, height, depth))
        error(HeaderData::TooManyMipLevels, levelCount, max_dim);
    }

    if (isFormatBlockCompressed(vkFormat))
        if (levelCount == 0)
            error(HeaderData::BlockCompressedNoLevel, toStringVkFormat(vkFormat));

    // Validate supercompressionScheme
    if (KTX_SS_BEGIN_VENDOR_RANGE < header.supercompressionScheme && header.supercompressionScheme <= KTX_SS_END_VENDOR_RANGE)
        warning(HeaderData::VendorSupercompression, header.supercompressionScheme);
    else if (header.supercompressionScheme < KTX_SS_BEGIN_RANGE || KTX_SS_END_RANGE < header.supercompressionScheme)
        error(HeaderData::InvalidSupercompression, header.supercompressionScheme);
}

void ValidationContext::validateIndices() {
    // Validate indices

    // TODO Tools P2: Validate indices

    // if (header.dataFormatDescriptor.byteOffset == 0 || header.dataFormatDescriptor.byteLength == 0)
    //     error(HeaderData::InvalidRequiredIndexEntry, "dfd");
    //
    // if (header.keyValueData.byteOffset == 0 != header.keyValueData.byteLength == 0)
    //     error(HeaderData::InvalidOptionalIndexEntry, "kvd");
    //
    // if (header.supercompressionScheme == KTX_SS_BASIS_LZ) {
    //     if (header.supercompressionGlobalData.byteOffset == 0 || header.supercompressionGlobalData.byteLength == 0)
    //         error(HeaderData::InvalidRequiredIndexEntry, "sgd");
    // } else {
    //     if (header.supercompressionGlobalData.byteOffset == 0 != header.supercompressionGlobalData.byteLength == 0)
    //         error(HeaderData::InvalidOptionalIndexEntry, "sgd");
    // }
    //
    // levelIndexSize = sizeof(ktxLevelIndexEntry) * levelCount;
    // uint64_t offset = KTX2_HEADER_SIZE + levelIndexSize;
    // if (offset != header.dataFormatDescriptor.byteOffset)
    //     error(HeaderData::InvalidDFDOffset);
    // offset += header.dataFormatDescriptor.byteLength;
    //
    // if (header.keyValueData.byteOffset != 0) {
    //     if (offset != header.keyValueData.byteOffset)
    //         error(HeaderData::InvalidKVDOffset);
    //     offset += header.keyValueData.byteLength;
    //     if (header.supercompressionGlobalData.byteOffset != 0)
    //         // Pad before SGD.
    //         offset = padn(8, offset);
    // }
    //
    // if (header.supercompressionGlobalData.byteOffset != 0)
    //     if (offset != header.supercompressionGlobalData.byteOffset)
    //         error(HeaderData::InvalidSGDOffset);
}

// =================================================================================================
// TODO Tools P2: Validate the rest of the file

// void
// ktxValidator::validateLevelIndex(validationContext& ctx)
// {
//     ktxLevelIndexEntry* levelIndex = new ktxLevelIndexEntry[ctx.levelCount];
//     ctx.inp->read((char *)levelIndex, ctx.levelIndexSize);
//     if (ctx.inp->fail())
//         addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
//     else if (ctx.inp->eof())
//         addIssue(logger::eFatal, IOError.UnexpectedEOF);
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
//     size_t expectedOffset = 0;
//     size_t lastByteLength = 0;
//     switch (ctx.header.supercompressionScheme) {
//       case KTX_SS_NONE:
//       case KTX_SS_ZSTD:
//         expectedOffset = padn(requiredLevelAlignment, ctx.kvDataEndOffset());
//         break;
//       case KTX_SS_BASIS_LZ:
//         ktxIndexEntry64 sgdIndex = ctx.header.supercompressionGlobalData;
//         // No padding here.
//         expectedOffset = sgdIndex.byteOffset + sgdIndex.byteLength;
//         break;
//     }
//     expectedOffset = padn(requiredLevelAlignment, expectedOffset);
//     // Last mip level is first in the file. Count down so we can check the
//     // distance between levels for the UNDEFINED and SUPERCOMPRESSION cases.
//     for (int32_t level = ctx.levelCount-1; level >= 0; level--) {
//         if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED
//             && ctx.header.supercompressionScheme == KTX_SS_NONE) {
//             ktx_size_t actualUBL = levelIndex[level].uncompressedByteLength;
//             ktx_size_t expectedUBL = ctx.calcLevelSize(level);
//             if (actualUBL != expectedUBL)
//                 addIssue(logger::eError,
//                          LevelIndex.IncorrectUncompressedByteLength,
//                          level, actualUBL, expectedUBL);
//
//             if (levelIndex[level].byteLength !=
//                 levelIndex[level].uncompressedByteLength)
//                 addIssue(logger::eError, LevelIndex.UnequalByteLengths, level);
//
//             ktx_size_t expectedByteOffset = ctx.calcLevelOffset(level);
//             ktx_size_t actualByteOffset = levelIndex[level].byteOffset;
//             if (actualByteOffset != expectedByteOffset) {
//                 if (actualByteOffset % requiredLevelAlignment != 0)
//                     addIssue(logger::eError, LevelIndex.UnalignedOffset,
//                              level, requiredLevelAlignment);
//                 if (levelIndex[level].byteOffset > expectedByteOffset)
//                     addIssue(logger::eError, LevelIndex.ExtraPadding, level);
//                 else
//                     addIssue(logger::eError, LevelIndex.ByteOffsetTooSmall,
//                              level, actualByteOffset, expectedByteOffset);
//             }
//         } else {
//             // Can only do minimal validation as we have no idea what the
//             // level sizes are so we have to trust the byteLengths. We do
//             // at least know where the first level must be in the file and
//             // we can calculate how much padding, if any, there must be
//             // between levels.
//             if (levelIndex[level].byteLength == 0
//                 || levelIndex[level].byteOffset == 0) {
//                  addIssue(logger::eError, LevelIndex.ZeroOffsetOrLength, level);
//                  continue;
//             }
//             if (levelIndex[level].byteOffset != expectedOffset) {
//                 addIssue(logger::eError,
//                          LevelIndex.IncorrectByteOffset,
//                          level, levelIndex[level].byteOffset, expectedOffset);
//             }
//             if (ctx.header.supercompressionScheme == KTX_SS_NONE) {
//                 if (levelIndex[level].byteLength < lastByteLength)
//                     addIssue(logger.eError, LevelIndex.IncorrectLevelOrder);
//                 if (levelIndex[level].byteOffset % requiredLevelAlignment != 0)
//                     addIssue(logger::eError, LevelIndex.UnalignedOffset,
//                              level, requiredLevelAlignment);
//                 if (levelIndex[level].uncompressedByteLength == 0) {
//                     addIssue(logger::eError, LevelIndex.ZeroUncompressedLength,
//                              level);
//                 }
//                 lastByteLength = levelIndex[level].byteLength;
//             }
//             expectedOffset += padn(requiredLevelAlignment,
//                                    levelIndex[level].byteLength);
//             if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED) {
//                 // We can validate the uncompressedByteLength.
//                 ktx_size_t actualUBL = levelIndex[level].uncompressedByteLength;
//                 ktx_size_t expectedUBL = ctx.calcLevelSize(level);
//                 if (actualUBL != expectedUBL)
//                     addIssue(logger::eError,
//                              LevelIndex.IncorrectUncompressedByteLength,
//                              level, actualUBL, expectedUBL);
//             }
//         }
//         ctx.dataSizeFromLevelIndex += padn(ctx.requiredLevelAlignment(),
//                                            levelIndex[level].byteLength);
//     }
//     delete[] levelIndex;
// }
//
// void
// ktxValidator::validateDfd(validationContext& ctx)
// {
//     if (ctx.header.dataFormatDescriptor.byteLength == 0)
//         return;
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
// void
// ktxValidator::validateKvd(validationContext& ctx)
// {
//     uint32_t kvdLen = ctx.header.keyValueData.byteLength;
//     uint32_t lengthCheck = 0;
//     bool allKeysNulTerminated = true;
//
//     if (kvdLen == 0)
//         return;
//
//     uint8_t* kvd = new uint8_t[kvdLen];
//     ctx.inp->read((char *)kvd, kvdLen);
//     if (ctx.inp->fail())
//         addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
//     else if (ctx.inp->eof())
//         addIssue(logger::eFatal, IOError.UnexpectedEOF);
//
//     // Check all kv pairs have valuePadding and it's included in kvdLen;
//     uint8_t* pCurKv = kvd;
//     uint32_t safetyCount;
//     // safetyCount ensures we don't get stuck in an infinite loop in the event
//     // the kv data is completely bogus and the "lengths" never add up to kvdLen.
// #define MAX_KVPAIRS 75
//     for (safetyCount = 0; lengthCheck < kvdLen && safetyCount < MAX_KVPAIRS; safetyCount++) {
//         uint32_t curKvLen = *(uint32_t *)pCurKv;
//         lengthCheck += sizeof(uint32_t); // Add keyAndValueByteLength to total.
//         pCurKv += sizeof(uint32_t); // Move pointer past keyAndValueByteLength.
//         uint8_t* p = pCurKv;
//         uint8_t* pCurKvEnd = pCurKv + curKvLen;
//
//         // Check for BOM.
//         bool bom = false;
//         if (*p == 0xEF && *(p+1) == 0xBB && *(p+2) == 0xBF) {
//             bom = true;
//             p += 3;
//         }
//         for (; p < pCurKvEnd; p++) {
//             if (*p == '\0')
//               break;
//         }
//         bool noNul = (p == pCurKvEnd);
//         if (noNul) {
//             addIssue(logger::eError, Metadata.MissingNulTerminator, pCurKv);
//             allKeysNulTerminated = false;
//         }
//         if (bom) {
//             if (noNul)
//                 addIssue(logger::eError, Metadata.ForbiddenBOM1, pCurKv);
//             else
//                 addIssue(logger::eError, Metadata.ForbiddenBOM2, pCurKv);
//         }
//         curKvLen = (uint32_t)padn(4, curKvLen);
//         lengthCheck += curKvLen;
//         pCurKv += curKvLen;
//     }
//     if (safetyCount == 75)
//         addIssue(logger::eError, Metadata.InvalidStructure, MAX_KVPAIRS);
//     else if (lengthCheck != kvdLen)
//         addIssue(logger::eError, Metadata.MissingFinalPadding);
//
//     ktxHashList kvDataHead = 0;
//     ktxHashListEntry* entry;
//     char* prevKey;
//     uint32_t prevKeyLen;
//     KTX_error_code result;
//     bool writerFound = false;
//     bool writerScParamsFound = false;
//
//     if (allKeysNulTerminated) {
//         result = ktxHashList_Deserialize(&kvDataHead, kvdLen, kvd);
//         if (result != KTX_SUCCESS) {
//             addIssue(logger::eError, System.OutOfMemory);
//             return;
//         }
//
//         // Check the entries are sorted
//         ktxHashListEntry_GetKey(kvDataHead, &prevKeyLen, &prevKey);
//         entry = ktxHashList_Next(kvDataHead);
//         for (; entry != NULL; entry = ktxHashList_Next(entry)) {
//             uint32_t keyLen;
//             char* key;
//
//             ktxHashListEntry_GetKey(entry, &keyLen, &key);
//             if (strcmp(prevKey, key) > 0) {
//                 addIssue(logger::eError, Metadata.OutOfOrder);
//                 break;
//             }
//         }
//
//         for (entry = kvDataHead; entry != NULL; entry = ktxHashList_Next(entry)) {
//             uint32_t keyLen, valueLen;
//             char* key;
//             uint8_t* value;
//
//             ktxHashListEntry_GetKey(entry, &keyLen, &key);
//             ktxHashListEntry_GetValue(entry, &valueLen, (void**)&value);
//             if (strncasecmp(key, "KTX", 3) == 0) {
//                 if (!validateMetadata(ctx, key, value, valueLen)) {
//                     addIssue(logger::eError, Metadata.IllegalMetadata, key);
//                 }
//                 if (strncmp(key, "KTXwriter", 9) == 0)
//                     writerFound = true;
//                 if (strncmp(key, "KTXwriterScParams", 17) == 0)
//                     writerScParamsFound = true;
//             } else {
//                 addIssue(logger::eWarning, Metadata.CustomMetadata, key);
//             }
//         }
//         if (!writerFound) {
//             if (writerScParamsFound)
//                 addIssue(logger::eError, Metadata.NoRequiredKTXwriter);
//             else
//                 addIssue(logger::eWarning, Metadata.NoKTXwriter);
//         }
//     }
// }
//
// bool
// ktxValidator::validateMetadata(validationContext& ctx, const char* key,
//                                const uint8_t* pValue, uint32_t valueLen)
// {
// #define CALL_MEMBER_FN(object,ptrToMember)  ((object)->*(ptrToMember))
//     vector<metadataValidator>::const_iterator it;
//
//     for (it = metadataValidators.begin(); it < metadataValidators.end(); it++) {
//         if (!it->name.compare(key)) {
//             //validateMetadataFunc vf = it->validateFunc;
//             CALL_MEMBER_FN(this, it->validateFunc)(ctx, key, pValue, valueLen);
//             break;
//         }
//     }
//     if (it == metadataValidators.end())
//         return false; // Unknown KTX-prefixed and therefore illegal metadata.
//     else
//         return true;
// }
//
// void
// ktxValidator::validateCubemapIncomplete(validationContext& ctx,
//                                         const char* key,
//                                         const uint8_t*,
//                                         uint32_t valueLen)
// {
//     ctx.cubemapIncompleteFound = true;
//     if (valueLen != 1)
//         addIssue(logger::eError, Metadata.InvalidValue, key);
// }
//
// void
// ktxValidator::validateOrientation(validationContext& ctx,
//                                   const char* key,
//                                   const uint8_t* value,
//                                   uint32_t valueLen)
// {
//     if (valueLen == 0) {
//         addIssue(logger::eError, Metadata.MissingValue, key);
//         return;
//     }
//
//     string orientation;
//     const char* pOrientation = reinterpret_cast<const char*>(value);
//     if (value[valueLen - 1] != '\0') {
//         // regex_match on some platforms will fail to match an otherwise
//         // valid swizzle due to lack of a NUL terminator even IF there is
//         // no '$' at the end of the regex. Make a copy to avoid this.
//         orientation.assign(pOrientation, valueLen);
//         pOrientation = orientation.c_str();
//         addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
//     }
//
//     if (valueLen != ctx.dimensionCount + 1)
//         addIssue(logger::eError, Metadata.InvalidValue, key);
//
//     switch (ctx.dimensionCount) {
//       case 1:
//         if (!regex_match (pOrientation, regex("^[rl]$") ))
//             addIssue(logger::eError, Metadata.InvalidValue, key);
//         break;
//       case 2:
//         if (!regex_match(pOrientation, regex("^[rl][du]$")))
//             addIssue(logger::eError, Metadata.InvalidValue, key);
//         break;
//       case 3:
//         if (!regex_match(pOrientation, regex("^[rl][du][oi]$")))
//             addIssue(logger::eError, Metadata.InvalidValue, key);
//         break;
//     }
// }
//
// void
// ktxValidator::validateGlFormat(validationContext& /*ctx*/,
//                                const char* key,
//                                const uint8_t* /*value*/,
//                                uint32_t valueLen)
// {
//     if (valueLen != sizeof(uint32_t) * 3)
//         addIssue(logger::eError, Metadata.InvalidValue, key);
// }
//
// void
// ktxValidator::validateDxgiFormat(validationContext& /*ctx*/,
//                                  const char* key,
//                                  const uint8_t* /*value*/,
//                                  uint32_t valueLen)
//                             {
//     if (valueLen != sizeof(uint32_t))
//         addIssue(logger::eError, Metadata.InvalidValue, key);}
//
// void
// ktxValidator::validateMetalPixelFormat(validationContext& /*ctx*/,
//                                        const char* key,
//                                        const uint8_t* /*value*/,
//                                        uint32_t valueLen)
// {
//     if (valueLen != sizeof(uint32_t))
//         addIssue(logger::eError, Metadata.InvalidValue, key);
// }
//
// void
// ktxValidator::validateSwizzle(validationContext& /*ctx*/,
//                               const char* key,
//                               const uint8_t* value,
//                               uint32_t valueLen)
// {
//     string swizzle;
//     const char* pSwizzle = reinterpret_cast<const char*>(value);
//     if (value[valueLen - 1] != '\0') {
//         addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
//         // See comment in validateOrientation.
//         swizzle.assign(pSwizzle, valueLen);
//         pSwizzle = swizzle.c_str();
//     }
//     if (!regex_match(pSwizzle, regex("^[rgba01]{4}$")))
//         addIssue(logger::eError, Metadata.InvalidValue, key);
// }
//
// void
// ktxValidator::validateWriter(validationContext& /*ctx*/,
//                              const char* key,
//                              const uint8_t* value,
//                              uint32_t valueLen)
// {
//     if (value[valueLen-1] != '\0')
//         addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
// }
//
// void
// ktxValidator::validateWriterScParams(validationContext& /*ctx*/,
//                                      const char* key,
//                                      const uint8_t* value,
//                                      uint32_t valueLen)
// {
//     if (value[valueLen-1] != '\0')
//         addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
// }
//
// void
// ktxValidator::validateAstcDecodeMode(validationContext& ctx,
//                                      const char* key,
//                                      const uint8_t* value,
//                                      uint32_t valueLen)
// {
//     if (valueLen == 0) {
//         addIssue(logger::eError, Metadata.MissingValue, key);
//         return;
//     }
//
//     if (!regex_match((char*)value, regex("rgb9e5"))
//        && !regex_match((char*)value, regex("unorm8")))
//          addIssue(logger::eError, Metadata.InvalidValue, key);
//
//     if (!ctx.pActualDfd)
//         return;
//
//     uint32_t* bdb = ctx.pDfd4Format + 1;
//     if (KHR_DFDVAL(bdb, MODEL) != KHR_DF_MODEL_ASTC) {
//          addIssue(logger::eError, Metadata.NotAllowed, key,
//                   "for non-ASTC texture formats");
//     }
//     if (KHR_DFDVAL(bdb, TRANSFER) == KHR_DF_TRANSFER_SRGB) {
//          addIssue(logger::eError, Metadata.NotAllowed, key,
//                   "with sRGB transfer function");
//     }
// }
//
// void
// ktxValidator::validateAnimData(validationContext& ctx,
//                                const char* key,
//                                const uint8_t* /*value*/,
//                                uint32_t valueLen)
// {
//     if (ctx.cubemapIncompleteFound) {
//          addIssue(logger::eError, Metadata.NotAllowed, key,
//                   "together with KTXcubemapIncomplete");
//     }
//     if (ctx.layerCount == 0)
//         addIssue(logger::eError, Metadata.NotAllowed, key,
//                  "except with array textures");
//
//     if (valueLen != sizeof(uint32_t) * 3)
//         addIssue(logger::eError, Metadata.InvalidValue, key);
// }
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

} // namespace ktx
