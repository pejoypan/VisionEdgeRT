#include "image_processor.h"
#include <opencv2/imgproc.hpp>
#include "../utils/logging.h"
#include "../third_party/zmq_addon.hpp"
#include "../third_party/msgpack.hpp"
#include "../utils/types.h"
#include "../utils/timer.h"


using namespace std;
using vert::logger;

vert::ImageProcessor::ImageProcessor(zmq::context_t *_ctx)
    : 
    ctx_(_ctx),
    sub_socket_(*_ctx, zmq::socket_type::sub)
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

    logger->info("Stopping {}...", name_);
    
    is_running_.store(false);

    sub_socket_.close();
    // TODO: close outer push_socket

    for (auto &t : worker_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }

    if (receiver_thread_.joinable())
        receiver_thread_.join();

    logger->info("{} stopped", name_);
}

void vert::ImageProcessor::receiver_thread_func()
{
    // use push/pull pattern to achieve load balancing
    zmq::socket_t push_socket(*ctx_, zmq::socket_type::push);
    push_socket.bind("inproc://worker");

    while (is_running()) {
        vector<zmq::message_t> msgs;
        zmq::recv_result_t result = zmq::recv_multipart(sub_socket_, std::back_inserter(msgs));
        if (!result)
            continue;
        // assert(result && "recv failed");
        assert(*result == 2);

        push_socket.send(std::move(msgs[0]), zmq::send_flags::sndmore); // meta
        push_socket.send(std::move(msgs[1]), zmq::send_flags::dontwait); // image data

    }

    push_socket.close();
}

void vert::ImageProcessor::worker_thread_func(int id)
{
    zmq::socket_t pull_socket(*ctx_, zmq::socket_type::pull);
    pull_socket.connect("inproc://worker");
    pull_socket.set(zmq::sockopt::rcvtimeo, 1000);

    // zmq::socket_t test_socket(*ctx_, zmq::socket_type::pub);
    // test_socket.connect("tcp://127.0.0.1:5555");

    while (is_running()) {
        vector<zmq::message_t> msgs;
        zmq::recv_result_t result = zmq::recv_multipart(pull_socket, std::back_inserter(msgs));
        if (!result)
            continue;
        // assert(result && "recv failed");
        assert(*result == 2);

        auto meta = msgpack::unpack<vert::MatMeta>(static_cast<const uint8_t *>(msgs[0].data()), msgs[0].size());
        assert(meta.cv_type == CV_8UC1 || meta.cv_type == CV_8UC3);

        vert::log_mat(meta, "Worker recv");

        cv::Mat temp = cv::Mat(meta.height, meta.width, meta.cv_type, msgs[1].data());
        cv::Mat dst;

        test_process(temp, dst);

        // auto test_meta = meta;
        // test_meta.device_id = "cam#3";
        // auto test_meta_data = msgpack::pack(test_meta);
        // zmq::message_t test_meta_msg(test_meta_data.data(), test_meta_data.size());
        // test_socket.send(test_meta_msg, zmq::send_flags::sndmore); // meta

        // zmq::message_t test_msg;
        // test_msg.rebuild(dst.data, dst.total() * dst.elemSize());
        // test_socket.send(test_msg, zmq::send_flags::dontwait);

    }

    pull_socket.close();
    // test_socket.close();
}

void vert::ImageProcessor::test_process(const cv::Mat &src, cv::Mat &dst)
{
    FUNC_TIMER()
    src.copyTo(dst);

    for (int i = 0; i < 3; ++i) {
        cv::GaussianBlur(dst, dst, cv::Size(7, 7), 0);

        cv::Mat hsv, lab;
        cv::cvtColor(dst, hsv, cv::COLOR_BGR2HSV_FULL);

        cv::Mat mask;
        cv::inRange(hsv, cv::Scalar(0, 100, 0), cv::Scalar(255, 255, 255), mask);

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15));
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
        cv::dilate(mask, mask, kernel);

        dst.setTo(cv::Scalar(0, 0, 0), ~mask);

        cv::bitwise_not(dst, dst);
    }

}
