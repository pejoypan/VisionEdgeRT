#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include "../nodes/third_party/zmq.hpp"
#include "../nodes/third_party/zmq_addon.hpp"
#include "../nodes/third_party/msgpack.hpp"
#include "../nodes/utils/types.h"

int main(int argc, char **argv) {
    
    zmq::context_t context(1);
    zmq::socket_t publisher;

    try
    {
        publisher = zmq::socket_t(context, zmq::socket_type::pub);
        publisher.bind("tcp://localhost:5555");
        
        std::cout << "start send image" << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    cv::Mat image;
    for (size_t i = 0; i < 9999; i++) {

        image.create(1024, 1024, CV_8UC3);
        cv::randn(image, cv::Scalar(128, 128, 128), cv::Scalar(128, 128, 128));
        vert::MatMeta meta{"cam", (int64_t)i, 1024, 1024, CV_8UC3, 3, 0};
        
        auto meta_data = msgpack::pack(meta);
        zmq::message_t meta_msg(meta_data.data(), meta_data.size());
        publisher.send(meta_msg, zmq::send_flags::sndmore);
        
        zmq::message_t img_msg;
        img_msg.rebuild(image.data ,image.total() * image.elemSize());
        publisher.send(img_msg, zmq::send_flags::dontwait);
        
        std::cout << "send image " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    
}