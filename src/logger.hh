// Simple log interface

#pragma once
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <cstdarg>
#include "focusstack.hh"

namespace focusstack {

class Logger {
public:
    Logger();
    
    // The log_level_t enum is part of the public API and thus in focusstack.hh,
    // but we alias it here for convenience.
    using log_level_t = FocusStack::log_level_t;
    static const log_level_t LOG_VERBOSE  = FocusStack::LOG_VERBOSE;
    static const log_level_t LOG_PROGRESS = FocusStack::LOG_PROGRESS;
    static const log_level_t LOG_INFO     = FocusStack::LOG_INFO;
    static const log_level_t LOG_ERROR    = FocusStack::LOG_ERROR;
    
    void set_callback(std::function<void(log_level_t level, std::string)> callback);
    void set_level(log_level_t level);
    log_level_t get_level() const;

    void verbose(const char *format, ...);
    void progress(const char *format, ...);
    void info(const char *format, ...);
    void error(const char *format, ...);
    void vlog(log_level_t level, const char *format, va_list va);

    static void default_callback(log_level_t level, std::string message);

private:
    std::mutex m_mutex;
    std::function<void(log_level_t level, std::string)> m_callback;
    std::atomic<log_level_t> m_level;
};

}