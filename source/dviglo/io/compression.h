// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../common/config.h"

namespace dviglo
{

class Deserializer;
class Serializer;
class VectorBuffer;

/// Estimate and return worst case LZ4 compressed output size in bytes for given input size
DV_API unsigned EstimateCompressBound(unsigned srcSize);

/// Compress data using the LZ4 algorithm and return the compressed data size. The needed destination buffer worst-case size is given by EstimateCompressBound()
DV_API unsigned compress_data(void* dest, const void* src, unsigned srcSize);

/// Uncompress data using the LZ4 algorithm. The uncompressed data size must be known. Return the number of compressed data bytes consumed
DV_API unsigned decompress_data(void* dest, const void* src, unsigned destSize);

/// Compress a source stream (from current position to the end) to the destination stream using the LZ4 algorithm. Return true on success
DV_API bool compress_stream(Serializer& dest, Deserializer& src);

/// Decompress a compressed source stream produced using compress_stream() to the destination stream. Return true on success
DV_API bool decompress_stream(Serializer& dest, Deserializer& src);

/// Compress a VectorBuffer using the LZ4 algorithm and return the compressed result buffer
DV_API VectorBuffer compress_vector_buffer(VectorBuffer& src);

/// Decompress a VectorBuffer produced using compress_vector_buffer()
DV_API VectorBuffer decompress_vector_buffer(VectorBuffer& src);

} // namespace dviglo
