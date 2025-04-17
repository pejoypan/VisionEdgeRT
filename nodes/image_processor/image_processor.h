#ifndef _IMAGE_PROCESSOR_H_
#define _IMAGE_PROCESSOR_H_
#include <atomic>
#include <thread>
#include <string>
#include <yaml-cpp/yaml.h>
#include "../third_party/zmq.hpp"

namespace vert {

    class ImageProcessor {
    public:
        ImageProcessor(zmq::context_t *ctx);
        ~ImageProcessor();

        bool init(const YAML::Node &config);

        void start();
        void stop();
        bool is_running() const {return is_running_.load();}

    private:
        void receiver_thread_func();
        void worker_thread_func(int id);

        zmq::socket_t sub_socket_;
        zmq::socket_t inner_push_socket_;
        zmq::socket_t inner_pull_socket_;
        // TODO: socket to outer

        std::atomic<bool> is_running_{false};

        std::thread receiver_thread_;
        std::vector<std::thread> worker_threads_;

        std::string name_ = "ImageProcessor";
        std::string addr_from_;
        std::string addr_to_;

        int num_workers_ = 5;

    };

}


#endif /* _IMAGE_PROCESSOR_H_ */
