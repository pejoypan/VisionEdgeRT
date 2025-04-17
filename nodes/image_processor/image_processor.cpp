#include "image_processor.h"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "../utils/logging.h"
#include "../third_party/zmq_addon.hpp"
#include "../third_party/msgpack.hpp"
#include "../utils/types.h"


using namespace std;
using vert::logger;

vert::ImageProcessor::ImageProcessor(zmq::context_t *ctx)
    : 
    sub_socket_(*ctx, zmq::socket_type::sub),
    inner_push_socket_(*ctx, zmq::socket_type::push),
    inner_pull_socket_(*ctx, zmq::socket_type::pull)
{
}

vert::ImageProcessor::~ImageProcessor()
{
    stop();
}

bool vert::ImageProcessor::init(const YAML::Node &config)
{
    logger->info("Initializing ImageProcessor...");

    try {
        if (!config) {
            logger->critical("Failed to init ImageProcessor. Reason: config is empty");
            return false;
        }

        if (config["name"]) {
            name_ = config["name"].as<std::string>();  
        } else {
            logger->warn("name not provided."); 
        }

        if (config["port"]) {
            if (config["port"]["from"]) {
                addr_from_ = config["port"]["from"].as<std::string>();
            } else {
                logger->critical("Failed to init {}. Reason: port.from is empty", name_);
                return false;
            }

            // TODO: port.to

        } else {
            logger->critical("Failed to init {}. Reason: port is empty", name_);
            return false; 
        }

        if (config["num_workers"]) {
            int count = config["num_workers"].as<int>();
            num_workers_ = max(1, count);
            logger->info("num_workers set to: {}", num_workers_);
        } else {
            logger->warn("num_workers not provided. Using default value: {}", num_workers_);
            
        }


    } catch(const std::exception& e) {
        vert::logger->critical("Failed to init {}. Reason: {}", name_, e.what());
        return false;
    }
    
    logger->info("{} initialized successfully", name_);
    return true;
}

void vert::ImageProcessor::start()
{
    sub_socket_.connect(addr_from_);
    logger->info("{} sub_socket connect to {}", name_, addr_from_);
    sub_socket_.set(zmq::sockopt::subscribe, ""); // subscribe to all topics
    sub_socket_.set(zmq::sockopt::rcvtimeo, 1000);

    // TODO: outer socket

    is_running_.store(true);

    receiver_thread_ = thread(&ImageProcessor::receiver_thread_func, this);

    for (int i = 0; i < num_workers_; ++i) { // 4 worker threads
        worker_threads_.emplace_back(&ImageProcessor::worker_thread_func, this, i);
    }
}

void vert::ImageProcessor::stop()
{
    if (!is_running())
        return;
    
    is_running_.store(false);

    sub_socket_.close();
    // TODO: close outer push_socket

    if (receiver_thread_.joinable())
        receiver_thread_.join();

    for (auto &t : worker_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void vert::ImageProcessor::receiver_thread_func()
{
    // use push/pull pattern to achieve load balancing
    inner_push_socket_.bind("inproc://worker");

    while (is_running()) {
        vector<zmq::message_t> msgs;
        zmq::recv_result_t result = zmq::recv_multipart(sub_socket_, std::back_inserter(msgs));
        if (!result)
            continue;
        // assert(result && "recv failed");
        assert(*result == 2);

        inner_push_socket_.send(std::move(msgs[0]), zmq::send_flags::sndmore); // meta
        inner_push_socket_.send(std::move(msgs[1]), zmq::send_flags::dontwait); // image data

    }

    inner_push_socket_.close();
}

void vert::ImageProcessor::worker_thread_func(int id)
{
    inner_pull_socket_.connect("inproc://worker");

    while (is_running()) {
        vector<zmq::message_t> msgs;
        zmq::recv_result_t result = zmq::recv_multipart(sub_socket_, std::back_inserter(msgs));
        if (!result)
            continue;
        // assert(result && "recv failed");
        assert(*result == 2);

        auto meta = msgpack::unpack<vert::MatMeta>(static_cast<const uint8_t *>(msgs[0].data()), msgs[0].size());
        assert(meta.cv_type == CV_8UC1 || meta.cv_type == CV_8UC3);

        vert::log_mat(meta, "Worker recv");

        cv::Mat temp = cv::Mat(meta.height, meta.width, meta.cv_type, msgs[1].data());

        cv::Mat dst;
        cv::bitwise_not(temp, dst);

        logger->warn("worker finish");
    }

    inner_pull_socket_.close();
}
