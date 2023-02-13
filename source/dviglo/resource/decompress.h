// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../resource/image.h"

namespace dviglo
{

/// Decompress a DXT compressed image to RGBA.
DV_API void
    DecompressImageDXT(unsigned char* rgba, const void* blocks, int width, int height, int depth, CompressedFormat format);
/// Decompress an ETC1/ETC2 compressed image to RGBA.
DV_API void DecompressImageETC(unsigned char* dstImage, const void* blocks, int width, int height, bool hasAlpha);
/// Decompress a PVRTC compressed image to RGBA.
DV_API void DecompressImagePVRTC(unsigned char* rgba, const void* blocks, int width, int height, CompressedFormat format);
/// Flip a compressed block vertically.
DV_API void FlipBlockVertical(unsigned char* dest, const unsigned char* src, CompressedFormat format);
/// Flip a compressed block horizontally.
DV_API void FlipBlockHorizontal(unsigned char* dest, const unsigned char* src, CompressedFormat format);

}
