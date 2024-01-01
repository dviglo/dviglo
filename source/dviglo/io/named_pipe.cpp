// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "../core/profiler.h"
#include "named_pipe.h"
#include "log.h"

#ifdef _WIN32
#include "../common/win_wrapped.h"
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <csignal>
#include <unistd.h>
#endif

#include "../common/debug_new.h"

namespace dviglo
{

static constexpr i32 PIPE_BUFFER_SIZE = 65536;

NamedPipe::NamedPipe() :
    isServer_(false),
#ifdef _WIN32
    handle_(INVALID_HANDLE_VALUE)
#else
    readHandle_(-1),
    writeHandle_(-1)
#endif
{
}

NamedPipe::NamedPipe(const String& name, bool isServer) :
    isServer_(false),
#ifdef _WIN32
    handle_(INVALID_HANDLE_VALUE)
#else
    readHandle_(-1),
    writeHandle_(-1)
#endif
{
    Open(name, isServer);
}

NamedPipe::~NamedPipe()
{
    Close();
}

i64 NamedPipe::Seek(i64 position)
{
    return 0;
}

#ifdef _WIN32

static const char* pipePath = "\\\\.\\pipe\\";

bool NamedPipe::Open(const String& name, bool isServer)
{
    DV_PROFILE(OpenNamedPipe);

    Close();

    isServer_ = false;

    if (isServer)
    {
        handle_ = CreateNamedPipeW(WString(pipePath + name).c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
            1,
            PIPE_BUFFER_SIZE,
            PIPE_BUFFER_SIZE,
            0,
            nullptr
        );

        if (handle_ == INVALID_HANDLE_VALUE)
        {
            DV_LOGERROR("Failed to create named pipe " + name);
            return false;
        }
        else
        {
            DV_LOGDEBUG("Created named pipe " + name);
            name_ = name;
            isServer_ = true;
            return true;
        }
    }
    else
    {
        handle_ = CreateFileW(
            WString(pipePath + name).c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (handle_ == INVALID_HANDLE_VALUE)
        {
            DV_LOGERROR("Failed to connect to named pipe " + name);
            return false;
        }
        else
        {
            DV_LOGDEBUG("Connected to named pipe " + name);
            name_ = name;
            return true;
        }
    }
}

i32 NamedPipe::Read(void* dest, i32 size)
{
    assert(size >= 0);

    if (handle_ != INVALID_HANDLE_VALUE)
    {
        DWORD read = 0;
        ReadFile(handle_, dest, size, &read, nullptr);
        return read;
    }

    return 0;
}

i32 NamedPipe::Write(const void* data, i32 size)
{
    assert(size >= 0);

    if (handle_ != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        WriteFile(handle_, data, size, &written, nullptr);
        return written;
    }

    return 0;
}

void NamedPipe::Close()
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        DV_PROFILE(CloseNamedPipe);

        if (isServer_)
        {
            DisconnectNamedPipe(handle_);
            isServer_ = false;
        }

        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        name_.Clear();

        DV_LOGDEBUG("Closed named pipe " + name_);
    }
}

bool NamedPipe::IsOpen() const
{
    return handle_ != INVALID_HANDLE_VALUE;
}

bool NamedPipe::IsEof() const
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        DWORD bytesAvailable = 0;
        PeekNamedPipe(handle_, nullptr, 0, nullptr, &bytesAvailable, nullptr);
        return bytesAvailable == 0;
    }
    else
        return true;
}

#else

static const char* pipePath = "/tmp/";

#define SAFE_CLOSE(handle) if ((handle) != -1) { close(handle); (handle) = -1; }

bool NamedPipe::Open(const String& name, bool isServer)
{
#ifdef __EMSCRIPTEN__
    DV_LOGERROR("Opening a named pipe not supported on Web platform");
    return false;
#else
    DV_PROFILE(OpenNamedPipe);

    Close();

    isServer_ = false;

    String serverReadName = pipePath + name + "SR";
    String clientReadName = pipePath + name + "CR";

    // Make sure SIGPIPE is ignored and will not lead to process termination
    signal(SIGPIPE, SIG_IGN);

    if (isServer)
    {
        mkfifo(serverReadName.c_str(), 0660);
        mkfifo(clientReadName.c_str(), 0660);

        readHandle_ = open(serverReadName.c_str(), O_RDONLY | O_NDELAY);
        writeHandle_ = open(clientReadName.c_str(), O_WRONLY | O_NDELAY);

        if (readHandle_ == -1 && writeHandle_ == -1)
        {
            DV_LOGERROR("Failed to create named pipe " + name);
            SAFE_CLOSE(readHandle_);
            SAFE_CLOSE(writeHandle_);
            unlink(serverReadName.c_str());
            unlink(clientReadName.c_str());
            return false;
        }
        else
        {
            DV_LOGDEBUG("Created named pipe " + name);
            name_ = name;
            isServer_ = true;
            return true;
        }
    }
    else
    {
        readHandle_ = open(clientReadName.c_str(), O_RDONLY | O_NDELAY);
        writeHandle_ = open(serverReadName.c_str(), O_WRONLY | O_NDELAY);
        if (readHandle_ == -1 && writeHandle_ == -1)
        {
            DV_LOGERROR("Failed to connect to named pipe " + name);
            SAFE_CLOSE(readHandle_);
            SAFE_CLOSE(writeHandle_);
            return false;
        }
        else
        {
            DV_LOGDEBUG("Connected to named pipe " + name);
            name_ = name;
            return true;
        }
    }
#endif
}

i32 NamedPipe::Read(void* dest, i32 size)
{
    assert(size >= 0);

    // Attempt to open late if only the write handle is open yet
    if (readHandle_ == -1 && writeHandle_ != -1)
    {
        if (isServer_)
            readHandle_ = open((pipePath + name_ + "SR").c_str(), O_RDONLY | O_NDELAY);
        else
            readHandle_ = open((pipePath + name_ + "CR").c_str(), O_RDONLY | O_NDELAY);
    }

    if (readHandle_ != -1)
    {
        ssize_t readNow = read(readHandle_, dest, size);
        return readNow < 0 ? 0 : (unsigned)readNow;
    }
    else
        return 0;
}

i32 NamedPipe::Write(const void* data, i32 size)
{
    assert(size >= 0);

    // Attempt to open late if only the read handle is open yet
    if (writeHandle_ == -1 && readHandle_ != -1)
    {
        if (isServer_)
            writeHandle_ = open((pipePath + name_ + "CR").c_str(), O_WRONLY | O_NDELAY);
        else
            writeHandle_ = open((pipePath + name_ + "SR").c_str(), O_WRONLY | O_NDELAY);
    }

    // Loop until all bytes written in case of partial write
    if (writeHandle_ != -1)
    {
        ssize_t written = 0;
        while (written < (ssize_t)size)
        {
            ssize_t writtenNow = write(writeHandle_, ((const unsigned char*)data) + written, size - written);
            if (writtenNow < 0)
                return 0; // Error while writing
            written += writtenNow;
        }

        return written;
    }
    else
        return 0;
}

void NamedPipe::Close()
{
    if (readHandle_ != -1 || writeHandle_ != -1)
    {
        DV_PROFILE(CloseNamedPipe);
        SAFE_CLOSE(readHandle_);
        SAFE_CLOSE(writeHandle_);

        if (isServer_)
        {
            String serverReadName = pipePath + name_ + "SR";
            String clientReadName = pipePath + name_ + "CR";
            unlink(serverReadName.c_str());
            unlink(clientReadName.c_str());
            isServer_ = false;
        }

        name_.Clear();
    }
}

bool NamedPipe::IsOpen() const
{
    return readHandle_ != -1 || writeHandle_ != -1;
}

bool NamedPipe::IsEof() const
{
#ifdef __EMSCRIPTEN__
    return true;
#else
    // Attempt to open late if only the write handle is open yet
    if (readHandle_ == -1 && writeHandle_ != -1)
    {
        if (isServer_)
            readHandle_ = open((pipePath + name_ + "SR").c_str(), O_RDONLY | O_NDELAY);
        else
            readHandle_ = open((pipePath + name_ + "CR").c_str(), O_RDONLY | O_NDELAY);
    }

    if (readHandle_ != -1)
    {
        fd_set set;
        FD_ZERO(&set);      // NOLINT(modernize-use-bool-literals)
        FD_SET(readHandle_, &set);

        struct timeval timeout{0, 1000};    // 1ms timeout for select

        return select(readHandle_ + 1, &set, nullptr, nullptr, &timeout) <= 0;
    }
    else
        return true;
#endif
}
#endif

void NamedPipe::SetName(const String &name)
{
    DV_LOGERROR("Cannot change name of the NamedPipe!");
    assert(0);
}

}
