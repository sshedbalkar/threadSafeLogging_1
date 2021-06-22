#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "Logging.h"

void work() {
    std::ostringstream s;
    s << "hi my name is1: " << std::this_thread::get_id();
    std::string id = s.str();
    for (size_t i = 0; i < 10; ++i) {
        //merci::logging::ERROR(id);
        merci::logging::ERROR()
            << "hi my name is2: " << std::this_thread::get_id() << std::endl;
        //merci::logging::WARN(id);
        merci::logging::WARN()
            << "hi my name is2: " << std::this_thread::get_id() << std::endl;
        //merci::logging::INFO(id);
        merci::logging::INFO()
            << "hi my name is2: " << std::this_thread::get_id() << std::endl;
        //merci::logging::DEBUG(id);
        merci::logging::DEBUG()
            << "hi my name is2: " << std::this_thread::get_id() << std::endl;
        //merci::logging::TRACE(id);
        merci::logging::TRACE()
            << "hi my name is2: " << std::this_thread::get_id() << std::endl;
    }
}

int main(void) {
    // configure logging, if you dont it defaults to standard out logging with
    // colors logging::configure({ {"type", "file"}, {"file_name", "test.log"},
    // {"reopen_interval", "1"} });

    // start up some threads
    std::vector<std::shared_ptr<std::thread>> threads(
        std::thread::hardware_concurrency());
    for (auto& thread : threads) {
        thread.reset(new std::thread(work));
    }

    // wait for finish
    for (auto& thread : threads) {
        thread->join();
    }

    return 0;
}