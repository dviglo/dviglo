// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../containers/list.h"
#include "../core/object.h"
#include "../core/string_utils.h"

#include <mutex>

namespace dviglo
{

/// Fictional message level to indicate a stored raw message.
static const int LOG_RAW = -1;
/// Trace message level.
static const int LOG_TRACE = 0;
/// Debug message level. By default only shown in debug mode.
static const int LOG_DEBUG = 1;
/// Informative message level.
static const int LOG_INFO = 2;
/// Warning message level.
static const int LOG_WARNING = 3;
/// Error message level.
static const int LOG_ERROR = 4;
/// Disable all log messages.
static const int LOG_NONE = 5;

class File;

/// Stored log message from another thread.
struct StoredLogMessage
{
    /// Construct undefined.
    StoredLogMessage() = default;

    /// Construct with parameters.
    StoredLogMessage(const String& message, int level, bool error) :
        message_(message),
        level_(level),
        error_(error)
    {
    }

    /// Message text.
    String message_;
    /// Message level. -1 for raw messages.
    int level_{};
    /// Error flag for raw messages.
    bool error_{};
};

/// Logging subsystem.
class DV_API Log : public Object
{
    DV_OBJECT(Log, Object);

public:
    /// Construct.
    explicit Log();
    /// Destruct. Close the log file if open.
    ~Log() override;

    /// Open the log file.
    void Open(const String& fileName);
    /// Close the log file.
    void Close();
    /// Set logging level.
    void SetLevel(int level);
    /// Set whether to timestamp log messages.
    void SetTimeStamp(bool enable);
    /// Set quiet mode ie. only print error entries to standard error stream (which is normally redirected to console also). Output to log file is not affected by this mode.
    void SetQuiet(bool quiet);

    /// Return logging level.
    int GetLevel() const { return level_; }

    /// Return whether log messages are timestamped.
    bool GetTimeStamp() const { return timeStamp_; }

    /// Return last log message.
    String GetLastMessage() const { return lastMessage_; }

    /// Return whether log is in quiet mode (only errors printed to standard error stream).
    bool IsQuiet() const { return quiet_; }

    /// Write to the log. If logging level is higher than the level of the message, the message is ignored.
    static void Write(int level, const String& message);
    /// Write formatted message to the log. If logging level is higher than the level of the message, the message is ignored.
    static void WriteFormat(int level, const char* format, ...);
    /// Write raw output to the log.
    static void WriteRaw(const String& message, bool error = false);

private:
    /// Handle end of frame. Process the threaded log messages.
    void HandleEndFrame(StringHash eventType, VariantMap& eventData);

    /// Mutex for threaded operation.
    std::mutex logMutex_;
    /// Log messages from other threads.
    List<StoredLogMessage> threadMessages_;
    /// Log file.
    SharedPtr<File> logFile_;
    /// Last log message.
    String lastMessage_;
    /// Logging level.
    int level_;
    /// Timestamp log messages flag.
    bool timeStamp_;
    /// In write flag to prevent recursion.
    bool inWrite_;
    /// Quiet mode flag.
    bool quiet_;
};

#ifdef DV_LOGGING
#define DV_LOG(level, message) dviglo::Log::Write(level, message)
#define DV_LOGTRACE(message) dviglo::Log::Write(dviglo::LOG_TRACE, message)
#define DV_LOGDEBUG(message) dviglo::Log::Write(dviglo::LOG_DEBUG, message)
#define DV_LOGINFO(message) dviglo::Log::Write(dviglo::LOG_INFO, message)
#define DV_LOGWARNING(message) dviglo::Log::Write(dviglo::LOG_WARNING, message)
#define DV_LOGERROR(message) dviglo::Log::Write(dviglo::LOG_ERROR, message)
#define DV_LOGRAW(message) dviglo::Log::Write(dviglo::LOG_RAW, message)

#define DV_LOGF(level, format, ...) dviglo::Log::WriteFormat(level, format, ##__VA_ARGS__)
#define DV_LOGTRACEF(format, ...) dviglo::Log::WriteFormat(dviglo::LOG_TRACE, format, ##__VA_ARGS__)
#define DV_LOGDEBUGF(format, ...) dviglo::Log::WriteFormat(dviglo::LOG_DEBUG, format, ##__VA_ARGS__)
#define DV_LOGINFOF(format, ...) dviglo::Log::WriteFormat(dviglo::LOG_INFO, format, ##__VA_ARGS__)
#define DV_LOGWARNINGF(format, ...) dviglo::Log::WriteFormat(dviglo::LOG_WARNING, format, ##__VA_ARGS__)
#define DV_LOGERRORF(format, ...) dviglo::Log::WriteFormat(dviglo::LOG_ERROR, format, ##__VA_ARGS__)
#define DV_LOGRAWF(format, ...) dviglo::Log::WriteFormat(dviglo::LOG_RAW, format, ##__VA_ARGS__)

#else
#define DV_LOG(message) ((void)0)
#define DV_LOGTRACE(message) ((void)0)
#define DV_LOGDEBUG(message) ((void)0)
#define DV_LOGINFO(message) ((void)0)
#define DV_LOGWARNING(message) ((void)0)
#define DV_LOGERROR(message) ((void)0)
#define DV_LOGRAW(message) ((void)0)

#define DV_LOGF(...) ((void)0)
#define DV_LOGTRACEF(...) ((void)0)
#define DV_LOGDEBUGF(...) ((void)0)
#define DV_LOGINFOF(...) ((void)0)
#define DV_LOGWARNINGF(...) ((void)0)
#define DV_LOGERRORF(...) ((void)0)
#define DV_LOGRAWF(...) ((void)0)
#endif

}
