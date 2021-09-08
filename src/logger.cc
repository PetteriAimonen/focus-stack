#include "logger.hh"
#include <cstdio>
#include <iostream>

using namespace focusstack;

Logger::Logger():
    m_callback(default_callback), m_level(LOG_VERBOSE)
{
}

void Logger::set_callback(std::function<void(log_level_t level, std::string)> callback)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_callback = callback;
}

void Logger::set_level(log_level_t level)
{
    m_level = level;
}

Logger::log_level_t Logger::get_level() const
{
    return m_level.load();
}

void Logger::verbose(const char *format, ...)
{
    va_list va;
    va_start (va, format);
    vlog(LOG_VERBOSE, format, va);
    va_end (va);
}

void Logger::progress(const char *format, ...)
{
    va_list va;
    va_start (va, format);
    vlog(LOG_PROGRESS, format, va);
    va_end (va);
}

void Logger::info(const char *format, ...)
{
    va_list va;
    va_start (va, format);
    vlog(LOG_INFO, format, va);
    va_end (va);
}

void Logger::error(const char *format, ...)
{
    va_list va;
    va_start (va, format);
    vlog(LOG_ERROR, format, va);
    va_end (va);
}

void Logger::vlog(log_level_t level, const char *format, va_list va)
{
    if (level >= get_level())
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_callback)
        {
            char buffer[512];
            std::vsnprintf(buffer, sizeof(buffer), format, va);
            m_callback(level, std::string(buffer));
        }
    }
}

void Logger::default_callback(log_level_t level, std::string message)
{
    if (level == LOG_ERROR)
    {
        std::cerr << message;
        std::cerr.flush();
    }
    else
    {
        std::cout << message;
        std::cout.flush();
    }
}
