#pragma once
#include <string>
#include <sstream>
#include <vector>

#pragma region Macros

// Returns the name of the file in which it is called.
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

// Crash if the condition is false.
#define Assert(condition, message) Core::Assertion(condition, message, __FILENAME__, __FUNCTION__, __LINE__)

// Log info to console and file cache at the same time.
#define LogInfo(type, message) Core::Logger::PushLog({ type, Core::LogSeverity::Info, message, __FILENAME__, __FUNCTION__, __LINE__ })
// Log warning to console and file cache at the same time.
#define LogWarning(type, message) Core::Logger::PushLog({ type, Core::LogSeverity::Warning, message, __FILENAME__, __FUNCTION__, __LINE__ })
// Log error to console and file cache at the same time.
#define LogError(type, message) Core::Logger::PushLog({ type, Core::LogSeverity::Error, message, __FILENAME__, __FUNCTION__, __LINE__ })
// Log of specified log type in console and file cache at the same time.
#define LogWithSeverity(type, severity, message) Core::Logger::PushLog({ type, severity, message, __FILENAME__, __FUNCTION__, __LINE__ })

#pragma endregion 

namespace Core
{
    enum class LogSeverity
    {
        None,
        Info,
        Warning,
        Error,
    };
    enum class LogType
    {
        Default,
        Assertion,
        FileIO,
        GLFW,
        Vulkan,
        Resources,
    };
    std::string LogSeverityToStr(const LogSeverity& severity);
    std::string LogTypeToStr    (const LogType&     type    );
    
    void Assertion(const bool& condition, const std::string& message, const char* sourceFile, const char* sourceFunction, const long& sourceLine, const bool& throwError = true);

    struct Log
    {
        LogType     type;
        LogSeverity severity;
        std::string message;
        std::string sourceFile;
        std::string sourceFunction;
        long        sourceLine;
        
        Log(const LogType& _type, const LogSeverity& _severity, std::string _message, const char* _sourceFile, const char* _sourceFunction, const long& _sourceLine)
            : type(_type), severity(_severity), message(std::move(_message)), sourceFile(_sourceFile), sourceFunction(_sourceFunction), sourceLine(_sourceLine) {}

        explicit operator std::string() const;
        std::string ToString() const { return std::string(*this); }
    };
    
    class Logger
    {
    private:
        inline static Logger* instance = nullptr;
        std::string      filename;
        std::vector<Log> logs;
        
    public:
        Logger();
        Logger(const std::string& _filename);
        ~Logger();
        Logger(const Logger&)            = delete;
        Logger(Logger&&)                 = delete;
        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&)      = delete;
        
        static void SetLogFilename(const std::string& _filename, const bool& clearFile = false);
        static void SaveLogs();
        static void PushLog(const Log& log);

        static std::vector<Log>& GetLogs     () { return instance->logs;        }
        static Log&              GetLatestLog() { return instance->logs.back(); }

    private:
        static void CheckInstance();
    };
}
