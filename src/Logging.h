#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace merci::logging {
enum class log_level : uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4
};

struct enum_hasher {
    template <typename T>
    std::size_t operator()(T t) const {
        return static_cast<std::size_t>(t);
    }
};

const std::unordered_map<log_level, std::string, enum_hasher> uncolored{
    {log_level::ERROR, " [ERROR] "},
    {log_level::WARN, " [WARN] "},
    {log_level::INFO, " [INFO] "},
    {log_level::DEBUG, " [DEBUG] "},
    {log_level::TRACE, " [TRACE] "}};
const std::unordered_map<log_level, std::string, enum_hasher> colored{
    {log_level::ERROR, " \x1b[31;1m[ERROR]\x1b[0m "},
    {log_level::WARN, " \x1b[33;1m[WARN]\x1b[0m "},
    {log_level::INFO, " \x1b[32;1m[INFO]\x1b[0m "},
    {log_level::DEBUG, " \x1b[34;1m[DEBUG]\x1b[0m "},
    {log_level::TRACE, " \x1b[37;1m[TRACE]\x1b[0m "}};

// all, something in between, none or default to info
#if defined(LOGGING_LEVEL_ALL) || defined(LOGGING_LEVEL_TRACE)
constexpr log_level LOG_LEVEL_CUTOFF = log_level::TRACE;
#elif defined(LOGGING_LEVEL_DEBUG)
constexpr log_level LOG_LEVEL_CUTOFF = log_level::DEBUG;
#elif defined(LOGGING_LEVEL_WARN)
constexpr log_level LOG_LEVEL_CUTOFF = log_level::WARN;
#elif defined(LOGGING_LEVEL_ERROR)
constexpr log_level LOG_LEVEL_CUTOFF = log_level::ERROR;
#elif defined(LOGGING_LEVEL_NONE)
constexpr log_level LOG_LEVEL_CUTOFF = log_level::ERROR + 1;
#else
constexpr log_level LOG_LEVEL_CUTOFF = log_level::TRACE;
#endif
// returns formated to: 'year/mo/dy hr:mn:sc.xxxxxx'
std::string timestamp();

// logger base class, not pure virtual so you can use as a null logger if you
// want
using logging_config_t = std::unordered_map<std::string, std::string>;
class logger {
    using ManipFn = std::ostream& (*)(std::ostream&);
    using FlagsFn = std::ios_base& (*)(std::ios_base&);

   public:
    logger() = delete;
    logger(const logging_config_t&);
    virtual ~logger();
    virtual void log(const std::string&, const log_level);
    virtual void log(const std::string&);

    template <class T>    // int, double, strings, etc
    logger& operator<<(const T& output) {
        m_stream << output;
        return *this;
    }
    logger& operator<<(ManipFn manip);    /// endl, flush, setw, setfill, etc.
    logger& operator<<(FlagsFn manip);    /// setiosflags, resetiosflags
    logger& set_level(log_level e);
    logger& operator()(log_level e);
    logger& startsync(bool safe);
    void flush();

   protected:
    std::mutex lock;

   private:
    std::stringstream m_stream;
    log_level m_logLevel;
};

// logger that writes to standard out
class std_out_logger : public logger {
   public:
    std_out_logger() = delete;
    std_out_logger(const logging_config_t& config);
    virtual void log(const std::string& message, const log_level level);
    virtual void log(const std::string& message);

   protected:
    const std::unordered_map<log_level, std::string, enum_hasher> levels;
};

// TODO: add log rolling
// logger that writes to file
class file_logger : public logger {
   public:
    file_logger() = delete;
    file_logger(const logging_config_t& config);
    virtual void log(const std::string& message, const log_level level);
    virtual void log(const std::string& message);

   protected:
    void reopen();
    std::string file_name;
    std::ofstream file;
    std::chrono::seconds reopen_interval;
    std::chrono::system_clock::time_point last_reopen;
};

// a factory that can create loggers (that derive from 'logger') via function
// pointers this way you could make your own logger that sends log messages to
// who knows where
using logger_creator = logger* (*)(const logging_config_t&);
class logger_factory {
   public:
    logger_factory();
    logger* produce(const logging_config_t& config) const;

   protected:
    std::unordered_map<std::string, logger_creator> creators;
};

// statically get a factory
inline logger_factory& get_factory() {
    static logger_factory factory_singleton{};
    return factory_singleton;
}

// get at the singleton
inline logger& get_logger(const logging_config_t& config = {{"type", "std_out"},
                                                            {"color", ""}}) {
    static std::unique_ptr<logger> singleton(get_factory().produce(config));
    return *singleton;
}

// configure the singleton (once only)
inline void configure(const logging_config_t& config) { get_logger(config); }

// statically log manually without the macros below
inline void log(const std::string& message, const log_level level) {
    get_logger().log(message, level);
}

// statically log manually without a level or maybe with a custom one
inline void log(const std::string& message) { get_logger().log(message); }

// these standout when reading code
inline logger& TRACE() {
    return get_logger().set_level(log_level::TRACE).startsync(true);
}
inline void TRACE(const std::string& message) {
    get_logger().log(message, log_level::TRACE);
}
inline logger& DEBUG() {
    return get_logger().set_level(log_level::DEBUG).startsync(true);
}
inline void DEBUG(const std::string& message) {
    get_logger().log(message, log_level::DEBUG);
}
inline logger& INFO() {
    return get_logger().set_level(log_level::INFO).startsync(true);
}
inline void INFO(const std::string& message) {
    get_logger().log(message, log_level::INFO);
}
inline logger& WARN() {
    return get_logger().set_level(log_level::WARN).startsync(true);
}
inline void WARN(const std::string& message) {
    get_logger().log(message, log_level::WARN);
}
inline logger& ERROR() {
    return get_logger().set_level(log_level::ERROR).startsync(true);
}
inline void ERROR(const std::string& message) {
    get_logger().log(message, log_level::ERROR);
}
}    // namespace merci::logging
#endif