// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../containers/array_ptr.h"
#include "compression.h"
#include "deserializer.h"
#include "serializer.h"
#include "vector_buffer.h"

#include <lz4/lz4.h>
#include <lz4/lz4hc.h>

namespace dviglo
{

unsigned estimate_compress_bound(unsigned srcSize)
{
    return (unsigned)LZ4_compressBound(srcSize);
}

unsigned compress_data(void* dest, const void* src, unsigned srcSize)
{
    if (!dest || !src || !srcSize)
        return 0;
    else
        return (unsigned)LZ4_compress_HC((const char*)src, (char*)dest, srcSize, LZ4_compressBound(srcSize), 0);
}

unsigned decompress_data(void* dest, const void* src, unsigned destSize)
{
    if (!dest || !src || !destSize)
        return 0;
    else
        return (unsigned)LZ4_decompress_fast((const char*)src, (char*)dest, destSize);
}

bool compress_stream(Serializer& dest, Deserializer& src)
{
    unsigned srcSize = src.GetSize() - src.GetPosition();
    // Prepend the source and dest. data size in the stream so that we know to buffer & uncompress the right amount
    if (!srcSize)
    {
        dest.WriteU32(0);
        dest.WriteU32(0);
        return true;
    }

    auto maxDestSize = (unsigned)LZ4_compressBound(srcSize);
    SharedArrayPtr<unsigned char> srcBuffer(new unsigned char[srcSize]);
    SharedArrayPtr<unsigned char> destBuffer(new unsigned char[maxDestSize]);

    if (src.Read(srcBuffer, srcSize) != srcSize)
        return false;

    auto destSize = (unsigned)LZ4_compress_HC((const char*)srcBuffer.Get(), (char*)destBuffer.Get(), srcSize, LZ4_compressBound(srcSize), 0);
    bool success = true;
    success &= dest.WriteU32(srcSize);
    success &= dest.WriteU32(destSize);
    success &= dest.Write(destBuffer, destSize) == destSize;
    return success;
}

bool decompress_stream(Serializer& dest, Deserializer& src)
{
    if (src.IsEof())
        return false;

    unsigned destSize = src.ReadU32();
    unsigned srcSize = src.ReadU32();
    if (!srcSize || !destSize)
        return true; // No data

    if (srcSize > src.GetSize())
        return false; // Illegal source (packed data) size reported, possibly not valid data

    SharedArrayPtr<unsigned char> srcBuffer(new unsigned char[srcSize]);
    SharedArrayPtr<unsigned char> destBuffer(new unsigned char[destSize]);

    if (src.Read(srcBuffer, srcSize) != srcSize)
        return false;

    LZ4_decompress_fast((const char*)srcBuffer.Get(), (char*)destBuffer.Get(), destSize);
    return dest.Write(destBuffer, destSize) == destSize;
}

VectorBuffer compress_vector_buffer(VectorBuffer& src)
{
    VectorBuffer ret;
    src.Seek(0);
    compress_stream(ret, src);
    ret.Seek(0);
    return ret;
}

VectorBuffer decompress_vector_buffer(VectorBuffer& src)
{
    VectorBuffer ret;
    src.Seek(0);
    decompress_stream(ret, src);
    ret.Seek(0);
    return ret;
}

}
