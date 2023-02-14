// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../container/array_ptr.h"
#include "../container/ref_counted.h"
#include "../core/thread.h"
#include "../io/deserializer.h"

#include <mutex>

namespace dviglo
{

/// HTTP connection state.
enum HttpRequestState
{
    HTTP_INITIALIZING = 0,
    HTTP_ERROR,
    HTTP_OPEN,
    HTTP_CLOSED
};

/// An HTTP connection with response data stream.
class DV_API HttpRequest : public RefCounted, public Deserializer, public Thread
{
public:
    /// Construct with parameters.
    HttpRequest(const String& url, const String& verb, const Vector<String>& headers, const String& postData);
    /// Destruct. Release the connection object.
    ~HttpRequest() override;

    /// Process the connection in the worker thread until closed.
    void ThreadFunction() override;

    /// Read response data from the HTTP connection and return number of bytes actually read. While the connection is open, will block while trying to read the specified size. To avoid blocking, only read up to as many bytes as GetAvailableSize() returns.
    i32 Read(void* dest, i32 size) override;
    /// Set position from the beginning of the stream. Not supported.
    i64 Seek(i64 position) override;
    /// Return whether all response data has been read.
    bool IsEof() const override;

    /// Return URL used in the request.
    const String& GetURL() const { return url_; }

    /// Return verb used in the request. Default GET if empty verb specified on construction.
    const String& GetVerb() const { return verb_; }

    /// Return error. Only non-empty in the error state.
    String GetError() const;
    /// Return connection state.
    HttpRequestState GetState() const;
    /// Return amount of bytes in the read buffer.
    i32 GetAvailableSize() const;

    /// Return whether connection is in the open state.
    bool IsOpen() const { return GetState() == HTTP_OPEN; }

private:
    /// Check for available read data in buffer and whether end has been reached. Must only be called when the mutex is held by the main thread.
    Pair<i32, bool> CheckAvailableSizeAndEof() const;

    /// URL.
    String url_;
    /// Verb.
    String verb_;
    /// Error string. Empty if no error.
    String error_;
    /// Headers.
    Vector<String> headers_;
    /// POST data.
    String postData_;
    /// Connection state.
    HttpRequestState state_;
    /// Mutex for synchronizing the worker and the main thread.
    mutable std::mutex mutex_;
    /// Read buffer for the worker thread.
    SharedArrayPtr<u8> httpReadBuffer_;
    /// Read buffer for the main thread.
    SharedArrayPtr<u8> readBuffer_;
    /// Read buffer read cursor.
    i32 readPosition_;
    /// Read buffer write cursor.
    i32 writePosition_;
};

}
