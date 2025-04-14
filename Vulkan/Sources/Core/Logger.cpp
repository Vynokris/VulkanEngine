#include "Core/Logger.h"
#include "Core/Engine.h"
#include <filesystem>
#include <fstream>
#include <iostream>

#include "Core/Application.h"

using namespace Core;
using namespace Core;

std::string Core::LogSeverityToStr(const LogSeverity& severity)
{
    switch (severity)
    {
    case LogSeverity::Info:
        return "Info";
    case LogSeverity::Warning:
        return "Warning";
    case LogSeverity::Error:
        return "Error";
    case LogSeverity::None:
    default:
        return "";
    }
}
std::string Core::LogTypeToStr(const LogType& type)
{
    switch (type)
    {
    case LogType::Assertion:
        return "Assertion";
    case LogType::FileIO:
        return "File IO";
    case LogType::GLFW:
        return "GLFW";
    case LogType::Vulkan:
        return "Vulkan";
    case LogType::Resources:
        return "Resources";
    case LogType::Default:
    default:
        return "";
    }
}

void Core::Assertion(const bool& condition, const std::string& message, const char* sourceFile, const char* sourceFunction, const long& sourceLine, const bool& throwError)
{
    if (condition) return;

    Logger::PushLog({ LogType::Assertion, LogSeverity::Error, message, sourceFile, sourceFunction, sourceLine });
    if (throwError)
    {
        Logger::SaveLogs();
        throw std::runtime_error(message);
    }
}

bool Log::operator==(const Log& other) const
{
    return type           == other.type
        && severity       == other.severity
        && sourceLine     == other.sourceLine
        && sourceFunction == other.sourceFunction
        && sourceFile     == other.sourceFile
        && message        == other.message;
}

Log::operator std::string() const
{
    std::string output;
    if (type != LogType::Default) {
        output += LogTypeToStr(type);
        output += severity != LogSeverity::None ? " " : ": ";
    }
    if (severity != LogSeverity::None) {
        output += LogSeverityToStr(severity) + ": ";
    }
    if (sourceFile[0] != '\0') {
        output += std::filesystem::path(sourceFile).filename().string();
    }
    if (sourceLine >= 0) {
        output += "(";
        output += std::to_string(sourceLine);
        output += ")";
    }
    if (sourceFunction[0] != '\0') {
        output += " in ";
        output += sourceFunction;
    }
    if (!output.empty()) {
        output += "\n";
    }
    output += message;
    return output;
}

Logger::Logger()
{
    if (!instance)
        instance = this;
}

Logger::Logger(const std::string& _filename)
{
    if (!instance)
        instance = this;
    SetLogFilename(_filename);
}

Logger::~Logger()
{
    SaveLogs();
}

void Logger::SetLogFilename(const std::string& _filename, const bool& clearFile)
{
    CheckInstance();
    instance->filename = _filename;
    if (clearFile)
    {
        std::ofstream file(instance->filename);
        file.close();
    }
}

void Logger::SaveLogs()
{
    CheckInstance();
    if (instance->filename.empty())
    {
        LogWarning(LogType::FileIO, "Unable to save logs because the log file name isn't set.");
        return;
    }
    if (std::fstream f(instance->filename, std::fstream::out); f.is_open())
    {
        for (const Log& log : instance->logs)
            f << log.ToString() << std::endl << std::endl;
        f.close();
        return;
    }
    LogWarning(LogType::FileIO, std::string("Unable to open log file: ") + instance->filename);
}

void Logger::PushLog(const Log& log, const size_t checkPreviousLogs)
{
    CheckInstance();
    if (!instance->logs.empty())
    {
        const size_t logCount = instance->logs.size();
        for (size_t i = 0; i < checkPreviousLogs && logCount-i > 0; i++)
        {
            if (*(instance->logs.end()-1-i) == log)
                return;
        }
    }
    instance->logs.push_back(log);
    std::cout << std::endl << log.ToString() << std::endl;
}

void Logger::CheckInstance()
{
    if (!instance)
        if (const Application* application = Application::Get())
            instance = application->GetLogger();
}
