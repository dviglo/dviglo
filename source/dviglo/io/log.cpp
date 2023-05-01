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

using namespace std;


namespace dviglo
{

const char* logLevelPrefixes[] =
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARNING",
    "!!ERROR", // С восклицательными знаками, чтобы легче было заметить в консоли Linux среди кучи DEBUG
    nullptr
};

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
    // Проверяем, что лог только один
    assert(!instance_);

    subscribe_to_event(E_ENDFRAME, DV_HANDLER(Log, HandleEndFrame));
    instance_ = this;
}

Log::~Log()
{
    if (log_file_)
    {
        Write(LOG_INFO, "Log closed in destructor");
        Close();
    }

    instance_ = nullptr;
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
    if (level != LOG_RAW)
    {
        // No-op if illegal level
        if (level < LOG_TRACE || level >= LOG_NONE || instance_->level_ > level)
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
        scoped_lock lock(instance_->log_mutex_);
        instance_->threadMessages_.Push(StoredLogMessage(message, level, false));

        return;
    }

    // Do not log if message level excluded or if currently sending a log event
    if (instance_->level_ > level || instance_->inWrite_)
        return;

    String formattedMessage = logLevelPrefixes[level];
    formattedMessage += ": " + message;
    instance_->lastMessage_ = message;

    if (instance_->timeStamp_)
        formattedMessage = "[" + time_to_str() + "] " + formattedMessage;

    if (instance_->quiet_)
    {
        // If in quiet mode, still print the error message to the standard error stream
        if (level == LOG_ERROR)
            PrintUnicodeLine(formattedMessage, true);
    }
    else
        PrintUnicodeLine(formattedMessage, level == LOG_ERROR);

    if (instance_->log_file_)
    {
        // Пишем в файл сообщения, которые были добавлены в лог до открытия файла
        if (!instance_->early_messages_.Empty())
        {
            file_write(instance_->early_messages_.c_str(), sizeof(char), instance_->early_messages_.Length(), instance_->log_file_);
            instance_->early_messages_.Clear();
        }

        file_write(formattedMessage.c_str(), sizeof(char), formattedMessage.Length(), instance_->log_file_);
        file_write("\n", sizeof(char), 1, instance_->log_file_);
        file_flush(instance_->log_file_);
    }
    else
    {
        // Запоминаем сообщение, чтобы вывести его в файл, когда файл будет открыт
        instance_->early_messages_ += formattedMessage + "\n";
    }

    instance_->inWrite_ = true;

    using namespace LogMessage;

    VariantMap& eventData = instance_->GetEventDataMap();
    eventData[P_MESSAGE] = formattedMessage;
    eventData[P_LEVEL] = level;
    instance_->SendEvent(E_LOGMESSAGE, eventData);

    instance_->log_message.emit(formattedMessage, level);

    instance_->inWrite_ = false;
}

void Log::WriteRaw(const String& message, bool error)
{
    // If not in the main thread, store message for later processing
    if (!Thread::IsMainThread())
    {
        scoped_lock lock(instance_->log_mutex_);
        instance_->threadMessages_.Push(StoredLogMessage(message, LOG_RAW, error));

        return;
    }

    // Prevent recursion during log event
    if (instance_->inWrite_)
        return;

    instance_->lastMessage_ = message;

    if (instance_->quiet_)
    {
        // If in quiet mode, still print the error message to the standard error stream
        if (error)
            PrintUnicode(message, true);
    }
    else
        PrintUnicode(message, error);

    if (instance_->log_file_)
    {
        // Пишем в файл сообщения, которые были добавлены в лог до открытия файла
        if (!instance_->early_messages_.Empty())
        {
            file_write(instance_->early_messages_.c_str(), sizeof(char), instance_->early_messages_.Length(), instance_->log_file_);
            instance_->early_messages_.Clear();
        }

        file_write(message.c_str(), sizeof(char), message.Length(), instance_->log_file_);
        file_flush(instance_->log_file_);
    }
    else
    {
        // Запоминаем сообщение, чтобы вывести его в файл, когда файл будет открыт
        instance_->early_messages_ += message + "\n";
    }

    instance_->inWrite_ = true;

    using namespace LogMessage;

    VariantMap& eventData = instance_->GetEventDataMap();
    eventData[P_MESSAGE] = message;
    eventData[P_LEVEL] = error ? LOG_ERROR : LOG_INFO;
    instance_->SendEvent(E_LOGMESSAGE, eventData);

    instance_->log_message.emit(message, error ? LOG_ERROR : LOG_INFO);

    instance_->inWrite_ = false;
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

    scoped_lock lock(log_mutex_);

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

} // namespace dviglo
