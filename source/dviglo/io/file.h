// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../containers/array_ptr.h"
#include "../core/object.h"
#include "abstract_file.h"

namespace dviglo
{

/// File open mode.
enum FileMode
{
    FILE_READ = 0,
    FILE_WRITE,
    FILE_READWRITE
};

class PackageFile;

/// %File opened either through the filesystem or from within a package file.
class DV_API File : public Object, public AbstractFile
{
    DV_OBJECT(File);

public:
    /// Construct.
    explicit File();
    /// Construct and open a filesystem file.
    File(const String& fileName, FileMode mode = FILE_READ);
    /// Construct and open from a package file.
    File(PackageFile* package, const String& fileName);
    /// Destruct. Close the file if open.
    ~File() override;

    /// Read bytes from the file. Return number of bytes actually read.
    i32 Read(void* dest, i32 size) override;
    /// Set position from the beginning of the file.
    i64 Seek(i64 position) override;
    /// Write bytes to the file. Return number of bytes actually written.
    i32 Write(const void* data, i32 size) override;

    /// Return a checksum of the file contents using the SDBM hash algorithm.
    hash32 GetChecksum() override;

    /// Open a filesystem file. Return true if successful.
    bool Open(const String& fileName, FileMode mode = FILE_READ);
    /// Open from within a package file. Return true if successful.
    bool Open(PackageFile* package, const String& fileName);
    /// Close the file.
    void Close();
    /// Flush any buffered output to the file.
    void Flush();

    /// Return the open mode.
    FileMode GetMode() const { return mode_; }

    /// Return whether is open.
    bool IsOpen() const;

    /// Return the file handle.
    void* GetHandle() const { return handle_; }

    /// Return whether the file originates from a package.
    bool IsPackaged() const { return offset_ != 0; }

private:
    /// Open file internally using either C standard IO functions. Return true if successful
    bool OpenInternal(const String& fileName, FileMode mode, bool fromPackage = false);
    /// Perform the file read internally using either C standard IO functions. Return true if successful. This does not handle compressed package file reading
    bool ReadInternal(void* dest, i32 size);
    /// Seek in file internally using either C standard IO functions
    void SeekInternal(i64 newPosition);

    /// Open mode.
    FileMode mode_;
    /// File handle.
    FILE* handle_;
    /// Read buffer for compressed file loading.
    SharedArrayPtr<u8> readBuffer_;
    /// Decompression input buffer for compressed file loading.
    SharedArrayPtr<u8> inputBuffer_;
    /// Read buffer position.
    i32 readBufferOffset_;
    /// Bytes in the current read buffer.
    i32 readBufferSize_;
    /// Start position within a package file, 0 for regular files.
    i64 offset_;
    /// Content checksum.
    hash32 checksum_;
    /// Compression flag.
    bool compressed_;
    /// Synchronization needed before read -flag.
    bool readSyncNeeded_;
    /// Synchronization needed before write -flag.
    bool writeSyncNeeded_;
};

}
