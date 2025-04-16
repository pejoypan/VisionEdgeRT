#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <string>
#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>
#include "../third_party/zmq.hpp"
#include "../third_party/fmt/format.h"



#ifdef _WIN32
    #ifdef VERT_UTILS_BUILD
        #define VERT_UTILS_API __declspec(dllexport)
    #else
        #define VERT_UTILS_API __declspec(dllimport)
    #endif
#else
    #define VERT_UTILS_API
#endif

namespace vert {
    extern VERT_UTILS_API std::shared_ptr<spdlog::logger> logger; // trace, debug, info, warn, error, critical

    
// A thread-safe zmq sink for spdlog
// Only messages with level >= warn are sent via ZeroMQ
template<typename Mutex>
class zmq_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
    zmq_sink(zmq::context_t* ctx, const std::string& endpoint)
        : socket_(*ctx, zmq::socket_type::push), stop_flag_(false)
    {
        socket_.connect(endpoint);
        sender_thread_ = std::thread(&zmq_sink::sender_loop, this);
    }

    ~zmq_sink() override {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_flag_ = true;
        }
        queue_cv_.notify_one();
        if (sender_thread_.joinable())
            sender_thread_.join();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (msg.level < spdlog::level::warn)
            return;

        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            message_queue_.emplace(fmt::to_string(formatted));
        }
        queue_cv_.notify_one();
    }

    void flush_() override {}

private:
    void sender_loop() {
        while (true) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [&] { return !message_queue_.empty() || stop_flag_; });

            if (stop_flag_ && message_queue_.empty())
                break;

            std::string message = std::move(message_queue_.front());
            message_queue_.pop();
            lock.unlock();

            zmq::message_t zmq_msg(message.begin(), message.end());
            socket_.send(zmq_msg, zmq::send_flags::none);
        }
    }

    zmq::socket_t socket_;
    std::thread sender_thread_;

    std::mutex queue_mutex_;
    std::condition_variable queue_cv_; // use condition_variable, wait message high efficiency
    std::queue<std::string> message_queue_;

    bool stop_flag_;
};

using zmq_sink_mt = zmq_sink<std::mutex>;
using zmq_sink_st = zmq_sink<spdlog::details::null_mutex>;


}

#endif
