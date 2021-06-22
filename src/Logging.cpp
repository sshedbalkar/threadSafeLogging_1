#include "Logging.h"
#include <cstddef>
//
namespace merci::logging {

std::string timestamp() {
    // get the time
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm gmt{};
    gmtime_r(&tt, &gmt);
    std::chrono::duration<double> fractional_seconds =
        (tp - std::chrono::system_clock::from_time_t(tt)) +
        std::chrono::seconds(gmt.tm_sec);
    // format the string
    std::string buffer("year/mo/dy hr:mn:sc.xxxxxx");
    sprintf(&buffer.front(), "%04d/%02d/%02d %02d:%02d:%09.6f",
            gmt.tm_year + 1900, gmt.tm_mon + 1, gmt.tm_mday, gmt.tm_hour,
            gmt.tm_min, fractional_seconds.count());
    return buffer;
}

void std_out_logger::log(const std::string& message, const log_level level) {
    if (level < LOG_LEVEL_CUTOFF) return;
    std::string output;
    output.reserve(message.length() + 64);
    output.append(timestamp());
    output.append(levels.find(level)->second);
    output.append(message);
    output.push_back('\n');
    log(output);
}

void std_out_logger::log(const std::string& message) {
    // cout is thread safe, to avoid multiple threads interleaving on one
    // line though, we make sure to only call the << operator once on
    // std::cout otherwise the << operators from different threads could
    // interleave obviously we dont care if flushes interleave
    // std::lock_guard<std::mutex> lk{lock};
    std::cout << message;
    std::cout.flush();
}

file_logger::file_logger(const logging_config_t& config) : logger(config) {
    // grab the file name
    auto name = config.find("file_name");
    if (name == config.end())
        throw std::runtime_error("No output file provided to file logger");
    file_name = name->second;

    // if we specify an interval
    reopen_interval = std::chrono::seconds(300);
    auto interval = config.find("reopen_interval");
    if (interval != config.end()) {
        try {
            reopen_interval =
                std::chrono::seconds(std::stoul(interval->second));
        } catch (...) {
            throw std::runtime_error(interval->second +
                                     " is not a valid reopen interval");
        }
    }

    // crack the file open
    reopen();
}

void file_logger::log(const std::string& message, const log_level level) {
    if (level < LOG_LEVEL_CUTOFF) return;
    std::string output;
    output.reserve(message.length() + 64);
    output.append(timestamp());
    output.append(uncolored.find(level)->second);
    output.append(message);
    output.push_back('\n');
    log(output);
}

void file_logger::log(const std::string& message) {
    lock.lock();
    file << message;
    file.flush();
    lock.unlock();
    reopen();
}

void file_logger::reopen() {
    // TODO: use CLOCK_MONOTONIC_COARSE
    // check if it should be closed and reopened
    auto now = std::chrono::system_clock::now();
    lock.lock();
    if (now - last_reopen > reopen_interval) {
        last_reopen = now;
        try {
            file.close();
        } catch (...) {
        }
        try {
            file.open(file_name, std::ofstream::out | std::ofstream::app);
            last_reopen = std::chrono::system_clock::now();
        } catch (std::exception& e) {
            try {
                file.close();
            } catch (...) {
            }
            throw e;
        }
    }
    lock.unlock();
}

logger_factory::logger_factory() {
    creators.emplace("", [](const logging_config_t& config) -> logger* {
        return new logger(config);
    });
    creators.emplace("std_out", [](const logging_config_t& config) -> logger* {
        return new std_out_logger(config);
    });
    creators.emplace("file", [](const logging_config_t& config) -> logger* {
        return new file_logger(config);
    });
}

logger* logger_factory::produce(const logging_config_t& config) const {
    // grab the type
    auto type = config.find("type");
    if (type == config.end())
        throw std::runtime_error(
            "Logging factory configuration requires a type of logger");
    // grab the logger
    auto found = creators.find(type->second);
    if (found != creators.end()) return found->second(config);
    // couldn't get a logger
    throw std::runtime_error("Couldn't produce logger for type: " +
                             type->second);
}
}