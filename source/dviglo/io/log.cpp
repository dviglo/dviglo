// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../containers/str.h"
#include "../core/context.h"
#include "../core/core_events.h"
#include "../core/process_utils.h"
#include "../core/thread.h"
#include "../core/time_base.h"
#include "file_base.h"
#include "io_events.h"
#include "log.h"

#include <cstdio>

#include "../common/debug_new.h"

namespace dviglo
{

const char* logLevelPrefixes[] =
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    nullptr
};

static Log* logInstance = nullptr;
static bool threadErrorDisplayed = false;

Log::Log() :
#ifdef _DEBUG
    level_(LOG_DEBUG),
#else
    level_(LOG_INFO),
#endif
    timeStamp_(true),
    inWrite_(false),
    quiet_(false)
{
    logInstance = this;

    SubscribeToEvent(E_ENDFRAME, DV_HANDLER(Log, HandleEndFrame));
}

Log::~Log()
{
    logInstance = nullptr;
}

void Log::Open(const String& filename)
{
    if (filename.Empty())
        return;

    if (log_file_)
    {
        if (log_file_name_ == filename)
            return;
        else
            Close();
    }

    log_file_ = file_open(filename, "wb");

    if (log_file_)
    {
        log_file_name_ = filename;
        Write(LOG_INFO, "Opened log file " + filename);
    }
    else
    {
        Write(LOG_ERROR, "Failed to create log file " + filename);
    }
}

void Log::Close()
{
    if (log_file_)
    {
        file_close(log_file_);
        log_file_ = nullptr;
        log_file_name_.Clear();
    }
}

void Log::SetLevel(int level)
{
    if (level < LOG_TRACE || level > LOG_NONE)
    {
        DV_LOGERRORF("Attempted to set erroneous log level %d", level);
        return;
    }

    level_ = level;
}

void Log::SetTimeStamp(bool enable)
{
    timeStamp_ = enable;
}

void Log::SetQuiet(bool quiet)
{
    quiet_ = quiet;
}

void Log::WriteFormat(int level, const char* format, ...)
{
    if (!logInstance)
        return;

    if (level != LOG_RAW)
    {
        // No-op if illegal level
        if (level < LOG_TRACE || level >= LOG_NONE || logInstance->level_ > level)
            return;
    }

    // Forward to normal Write() after formatting the input
    String message;
    va_list args;
    va_start(args, format);
    message.AppendWithFormatArgs(format, args);
    va_end(args);

    Write(level, message);
}

void Log::Write(int level, const String& message)
{
    // Special case for LOG_RAW level
    if (level == LOG_RAW)
    {
        WriteRaw(message, false);
        return;
    }

    // No-op if illegal level
    if (level < LOG_TRACE || level >= LOG_NONE)
        return;

    // If not in the main thread, store message for later processing
    if (!Thread::IsMainThread())
    {
        if (logInstance)
        {
            std::scoped_lock lock(logInstance->logMutex_);
            logInstance->threadMessages_.Push(StoredLogMessage(message, level, false));
        }

        return;
    }

    // Do not log if message level excluded or if currently sending a log event
    if (!logInstance || logInstance->level_ > level || logInstance->inWrite_)
        return;

    String formattedMessage = logLevelPrefixes[level];
    formattedMessage += ": " + message;
    logInstance->lastMessage_ = message;

    if (logInstance->timeStamp_)
        formattedMessage = "[" + time_to_str() + "] " + formattedMessage;

    if (logInstance->quiet_)
    {
        // If in quiet mode, still print the error message to the standard error stream
        if (level == LOG_ERROR)
            PrintUnicodeLine(formattedMessage, true);
    }
    else
        PrintUnicodeLine(formattedMessage, level == LOG_ERROR);

    if (logInstance->log_file_)
    {
        file_write(formattedMessage.c_str(), sizeof(char), formattedMessage.Length(), logInstance->log_file_);
        file_write("\n", sizeof(char), 1, logInstance->log_file_);
        file_flush(logInstance->log_file_);
    }

    logInstance->inWrite_ = true;

    using namespace LogMessage;

    VariantMap& eventData = logInstance->GetEventDataMap();
    eventData[P_MESSAGE] = formattedMessage;
    eventData[P_LEVEL] = level;
    logInstance->SendEvent(E_LOGMESSAGE, eventData);

    logInstance->inWrite_ = false;
}

void Log::WriteRaw(const String& message, bool error)
{
    // If not in the main thread, store message for later processing
    if (!Thread::IsMainThread())
    {
        if (logInstance)
        {
            std::scoped_lock lock(logInstance->logMutex_);
            logInstance->threadMessages_.Push(StoredLogMessage(message, LOG_RAW, error));
        }

        return;
    }

    // Prevent recursion during log event
    if (!logInstance || logInstance->inWrite_)
        return;

    logInstance->lastMessage_ = message;

    if (logInstance->quiet_)
    {
        // If in quiet mode, still print the error message to the standard error stream
        if (error)
            PrintUnicode(message, true);
    }
    else
        PrintUnicode(message, error);

    if (logInstance->log_file_)
    {
        file_write(message.c_str(), sizeof(char), message.Length(), logInstance->log_file_);
        file_flush(logInstance->log_file_);
    }

    logInstance->inWrite_ = true;

    using namespace LogMessage;

    VariantMap& eventData = logInstance->GetEventDataMap();
    eventData[P_MESSAGE] = message;
    eventData[P_LEVEL] = error ? LOG_ERROR : LOG_INFO;
    logInstance->SendEvent(E_LOGMESSAGE, eventData);

    logInstance->inWrite_ = false;
}

void Log::HandleEndFrame(StringHash eventType, VariantMap& eventData)
{
    // If the MainThreadID is not valid, processing this loop can potentially be endless
    if (!Thread::IsMainThread())
    {
        if (!threadErrorDisplayed)
        {
            fprintf(stderr, "Thread::mainThreadID is not setup correctly! Threaded log handling disabled\n");
            threadErrorDisplayed = true;
        }
        return;
    }

    std::scoped_lock lock(logMutex_);

    // Process messages accumulated from other threads (if any)
    while (!threadMessages_.Empty())
    {
        const StoredLogMessage& stored = threadMessages_.Front();

        if (stored.level_ != LOG_RAW)
            Write(stored.level_, stored.message_);
        else
            WriteRaw(stored.message_, stored.error_);

        threadMessages_.PopFront();
    }
}

}
