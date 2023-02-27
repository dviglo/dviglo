// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/profiler.h"
#include "file.h"
#include "file_base.h"
#include "file_system.h"
#include "log.h"
#include "memory_buffer.h"
#include "package_file.h"

#include <cstdio>
#include <lz4/lz4.h>

#include "../common/debug_new.h"

namespace dviglo
{

static const String OPEN_MODE[] =
{
    "rb",
    "wb",
    "r+b",
    "w+b"
};

static constexpr i32 SKIP_BUFFER_SIZE = 1024;

File::File() :
    mode_(FILE_READ),
    handle_(nullptr),
    readBufferOffset_(0),
    readBufferSize_(0),
    offset_(0),
    checksum_(0),
    compressed_(false),
    readSyncNeeded_(false),
    writeSyncNeeded_(false)
{
}

File::File(const String& fileName, FileMode mode) :
    mode_(FILE_READ),
    handle_(nullptr),
    readBufferOffset_(0),
    readBufferSize_(0),
    offset_(0),
    checksum_(0),
    compressed_(false),
    readSyncNeeded_(false),
    writeSyncNeeded_(false)
{
    Open(fileName, mode);
}

File::File(PackageFile* package, const String& fileName) :
    mode_(FILE_READ),
    handle_(nullptr),
    readBufferOffset_(0),
    readBufferSize_(0),
    offset_(0),
    checksum_(0),
    compressed_(false),
    readSyncNeeded_(false),
    writeSyncNeeded_(false)
{
    Open(package, fileName);
}

File::~File()
{
    Close();
}

bool File::Open(const String& fileName, FileMode mode)
{
    return OpenInternal(fileName, mode);
}

bool File::Open(PackageFile* package, const String& fileName)
{
    if (!package)
        return false;

    const PackageEntry* entry = package->GetEntry(fileName);
    if (!entry)
        return false;

    bool success = OpenInternal(package->GetName(), FILE_READ, true);
    if (!success)
    {
        DV_LOGERROR("Could not open package file " + fileName);
        return false;
    }

    name_ = fileName;
    offset_ = entry->offset_;
    checksum_ = entry->checksum_;
    size_ = entry->size_;
    compressed_ = package->IsCompressed();

    // Seek to beginning of package entry's file data
    SeekInternal(offset_);
    return true;
}

i32 File::Read(void* dest, i32 size)
{
    assert(size >= 0);

    if (!IsOpen())
    {
        // If file not open, do not log the error further here to prevent spamming the stderr stream
        return 0;
    }

    if (mode_ == FILE_WRITE)
    {
        DV_LOGERROR("File not opened for reading");
        return 0;
    }

    if (size + position_ > size_)
        size = size_ - position_;
    if (!size)
        return 0;

    if (compressed_)
    {
        i32 sizeLeft = size;
        u8* destPtr = (u8*)dest;

        while (sizeLeft)
        {
            if (!readBuffer_ || readBufferOffset_ >= readBufferSize_)
            {
                u8 blockHeaderBytes[4];
                ReadInternal(blockHeaderBytes, sizeof blockHeaderBytes);

                MemoryBuffer blockHeader(&blockHeaderBytes[0], sizeof blockHeaderBytes);
                i32 unpackedSize = blockHeader.ReadU16();
                i32 packedSize = blockHeader.ReadU16();

                if (!readBuffer_)
                {
                    readBuffer_ = new u8[unpackedSize];
                    inputBuffer_ = new u8[LZ4_compressBound(unpackedSize)];
                }

                /// \todo Handle errors
                ReadInternal(inputBuffer_.Get(), packedSize);
                LZ4_decompress_fast((const char*)inputBuffer_.Get(), (char*)readBuffer_.Get(), unpackedSize);

                readBufferSize_ = unpackedSize;
                readBufferOffset_ = 0;
            }

            i32 copySize = Min((readBufferSize_ - readBufferOffset_), sizeLeft);
            memcpy(destPtr, readBuffer_.Get() + readBufferOffset_, copySize);
            destPtr += copySize;
            sizeLeft -= copySize;
            readBufferOffset_ += copySize;
            position_ += copySize;
        }

        return size;
    }

    // Need to reassign the position due to internal buffering when transitioning from writing to reading
    if (readSyncNeeded_)
    {
        SeekInternal(position_ + offset_);
        readSyncNeeded_ = false;
    }

    if (!ReadInternal(dest, size))
    {
        // Return to the position where the read began
        SeekInternal(position_ + offset_);
        DV_LOGERROR("Error while reading from file " + GetName());
        return 0;
    }

    writeSyncNeeded_ = true;
    position_ += size;
    return size;
}

i64 File::Seek(i64 position)
{
    assert(position >= 0);

    if (!IsOpen())
    {
        // If file not open, do not log the error further here to prevent spamming the stderr stream
        return 0;
    }

    // Allow sparse seeks if writing
    if (mode_ == FILE_READ && position > size_)
        position = size_;

    if (compressed_)
    {
        // Start over from the beginning
        if (position == 0)
        {
            position_ = 0;
            readBufferOffset_ = 0;
            readBufferSize_ = 0;
            SeekInternal(offset_);
        }
        // Skip bytes
        else if (position >= position_)
        {
            u8 skipBuffer[SKIP_BUFFER_SIZE];
            while (position > position_)
                Read(skipBuffer, Min(position - position_, SKIP_BUFFER_SIZE));
        }
        else
            DV_LOGERROR("Seeking backward in a compressed file is not supported");

        return position_;
    }

    SeekInternal(position + offset_);
    position_ = position;
    readSyncNeeded_ = false;
    writeSyncNeeded_ = false;
    return position_;
}

i32 File::Write(const void* data, i32 size)
{
    assert(size >= 0);

    if (!IsOpen())
    {
        // If file not open, do not log the error further here to prevent spamming the stderr stream
        return 0;
    }

    if (mode_ == FILE_READ)
    {
        DV_LOGERROR("File not opened for writing");
        return 0;
    }

    if (!size)
        return 0;

    // Need to reassign the position due to internal buffering when transitioning from reading to writing
    if (writeSyncNeeded_)
    {
        file_seek(handle_, position_ + offset_, SEEK_SET);
        writeSyncNeeded_ = false;
    }

    if (file_write(data, size, 1, handle_) != 1)
    {
        // Return to the position where the write began
        file_seek(handle_, position_ + offset_, SEEK_SET);
        DV_LOGERROR("Error while writing to file " + GetName());
        return 0;
    }

    readSyncNeeded_ = true;
    position_ += size;
    if (position_ > size_)
        size_ = position_;

    return size;
}

hash32 File::GetChecksum()
{
    if (offset_ || checksum_)
        return checksum_;

    if (!handle_ || mode_ == FILE_WRITE)
        return 0;

    DV_PROFILE(CalculateFileChecksum);

    i64 oldPos = position_;
    checksum_ = 0;

    Seek(0);
    while (!IsEof())
    {
        u8 block[1024];
        i32 readBytes = Read(block, 1024);
        for (i32 i = 0; i < readBytes; ++i)
            checksum_ = SDBMHash(checksum_, block[i]);
    }

    Seek(oldPos);
    return checksum_;
}

void File::Close()
{
    readBuffer_.Reset();
    inputBuffer_.Reset();

    if (handle_)
    {
        file_close(handle_);
        handle_ = nullptr;
        position_ = 0;
        size_ = 0;
        offset_ = 0;
        checksum_ = 0;
    }
}

void File::Flush()
{
    if (handle_)
        file_flush(handle_);
}

bool File::IsOpen() const
{
    return handle_ != nullptr;
}

bool File::OpenInternal(const String& fileName, FileMode mode, bool fromPackage)
{
    Close();

    compressed_ = false;
    readSyncNeeded_ = false;
    writeSyncNeeded_ = false;

    if (fileName.Empty())
    {
        DV_LOGERROR("Could not open file with empty name");
        return false;
    }

    handle_ = file_open(fileName, OPEN_MODE[mode]);

    // If file did not exist in readwrite mode, retry with write-update mode
    if (mode == FILE_READWRITE && !handle_)
        handle_ = file_open(fileName, OPEN_MODE[mode + 1]);

    if (!handle_)
    {
        DV_LOGERRORF("Could not open file %s", fileName.c_str());
        return false;
    }

    if (!fromPackage)
    {
        file_seek(handle_, 0, SEEK_END);
        i64 size = file_tell(handle_);
        file_seek(handle_, 0, SEEK_SET);
        if (size > M_MAX_UNSIGNED)
        {
            DV_LOGERRORF("Could not open file %s which is larger than 4GB", fileName.c_str());
            Close();
            size_ = 0;
            return false;
        }
        size_ = (unsigned)size;
        offset_ = 0;
    }

    name_ = fileName;
    mode_ = mode;
    position_ = 0;
    checksum_ = 0;

    return true;
}

bool File::ReadInternal(void* dest, i32 size)
{
    assert(size >= 0);
    return file_read(dest, size, 1, handle_) == 1;
}

void File::SeekInternal(i64 newPosition)
{
    assert(newPosition >= 0);
    file_seek(handle_, newPosition, SEEK_SET);
}

} // namespace dviglo
