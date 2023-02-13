// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../container/array_ptr.h"
#include "../core/object.h"
#include "../io/abstract_file.h"

#ifdef __ANDROID__
#include <SDL3/SDL_rwops.h>
#endif

namespace dviglo
{

/// Named pipe for interprocess communication.
class DV_API NamedPipe : public Object, public AbstractFile
{
    DV_OBJECT(NamedPipe, Object);

public:
    /// Construct.
    explicit NamedPipe(Context* context);
    /// Construct and open in either server or client mode.
    NamedPipe(Context* context, const String& name, bool isServer);
    /// Destruct and close.
    ~NamedPipe() override;

    /// Read bytes from the pipe without blocking if there is less data available. Return number of bytes actually read.
    i32 Read(void* dest, i32 size) override;
    /// Set position. No-op for pipes.
    i64 Seek(i64 position) override;
    /// Write bytes to the pipe. Return number of bytes actually written.
    i32 Write(const void* data, i32 size) override;
    /// Return whether pipe has no data available.
    bool IsEof() const override;
    /// Not supported.
    void SetName(const String& name) override;

    /// Open the pipe in either server or client mode. If already open, the existing pipe is closed. For a client end to open successfully the server end must already to be open. Return true if successful.
    bool Open(const String& name, bool isServer);
    /// Close the pipe. Note that once a client has disconnected, the server needs to close and reopen the pipe so that another client can connect. At least on Windows this is not possible to detect automatically, so the communication protocol should include a "bye" message to handle this situation.
    void Close();

    /// Return whether is open.
    bool IsOpen() const;
    /// Return whether is in server mode.
    bool IsServer() const { return isServer_; }

private:
    /// Server mode flag.
    bool isServer_;
    /// Pipe handle.
#ifdef _WIN32
    void* handle_;
#else
    mutable int readHandle_;
    mutable int writeHandle_;
#endif
};

}
