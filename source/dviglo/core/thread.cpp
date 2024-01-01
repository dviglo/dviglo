// Copyright (c) 2022-2024 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include "thread.h"

#ifdef _WIN32
#include "../common/win_wrapped.h"
#else
#include <pthread.h>
#endif

#include "../common/debug_new.h"


namespace dviglo
{

#ifdef DV_THREADING
#ifdef _WIN32

static DWORD WINAPI ThreadFunctionStatic(void* data)
{
    Thread* thread = static_cast<Thread*>(data);
    thread->ThreadFunction();
    return 0;
}

#else

static void* ThreadFunctionStatic(void* data)
{
    auto* thread = static_cast<Thread*>(data);
    thread->ThreadFunction();
    pthread_exit((void*)nullptr);
    return nullptr;
}

#endif
#endif // DV_THREADING

ThreadId Thread::mainThreadID;

Thread::Thread() :
    handle_(nullptr),
    shouldRun_(false)
{
}

Thread::~Thread()
{
    Stop();
}

bool Thread::Run()
{
#ifdef DV_THREADING
    // Check if already running
    if (handle_)
        return false;

    shouldRun_ = true;
#ifdef _WIN32
    handle_ = CreateThread(nullptr, 0, ThreadFunctionStatic, this, 0, nullptr);
#else
    handle_ = new pthread_t;
    pthread_attr_t type;
    pthread_attr_init(&type);
    pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);
    pthread_create((pthread_t*)handle_, &type, ThreadFunctionStatic, this);
#endif
    return handle_ != nullptr;
#else
    return false;
#endif // DV_THREADING
}

void Thread::Stop()
{
#ifdef DV_THREADING
    // Check if already stopped
    if (!handle_)
        return;

    shouldRun_ = false;
#ifdef _WIN32
    WaitForSingleObject((HANDLE)handle_, INFINITE);
    CloseHandle((HANDLE)handle_);
#else
    auto* thread = (pthread_t*)handle_;
    if (thread)
        pthread_join(*thread, nullptr);
    delete thread;
#endif
    handle_ = nullptr;
#endif // DV_THREADING
}

void Thread::SetPriority(int priority)
{
#ifdef DV_THREADING
#ifdef _WIN32
    if (handle_)
        SetThreadPriority((HANDLE)handle_, priority);
#elif defined(__linux__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    auto* thread = (pthread_t*)handle_;
    if (thread)
        pthread_setschedprio(*thread, priority);
#endif
#endif // DV_THREADING
}

void Thread::SetMainThread()
{
    mainThreadID = GetCurrentThreadID();
}

ThreadId Thread::GetCurrentThreadID()
{
#ifdef DV_THREADING
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
#else
    return ThreadId();
#endif // DV_THREADING
}

bool Thread::IsMainThread()
{
#ifdef DV_THREADING
    return GetCurrentThreadID() == mainThreadID;
#else
    return true;
#endif // DV_THREADING
}

} // namespace dviglo
