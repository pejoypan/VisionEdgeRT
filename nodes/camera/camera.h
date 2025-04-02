#ifndef _CAMERA_H_
#define _CAMERA_H_
#include <opencv2/core.hpp>
#include <atomic>
#include <thread>
#include <mutex> // TODO: recv, get_curr_image, display, convert?
#include <pylon/PylonIncludes.h>
#include "../utils/types.h"
#include "../third_party/zmq.hpp"

namespace vert {

    class Camera
    {
    public:
        Camera(zmq::context_t *ctx, int _dst_type);
        ~Camera();

        void start();
        void stop();
        cv::Mat get_curr_image() const;
        bool is_running() const {return is_running_.load();}

    private:
        void loop();

        void recv();

        void convert(Pylon::EPixelType from); // TODO

        void display();
        
        void send();

        zmq::socket_t publisher_;
        zmq::socket_t subscriber_;

        std::atomic<bool> is_running_{false};

        std::thread loop_thread_;
        mutable std::mutex image_mutex_;

        cv::Mat img_raw_;
        cv::Mat img_cvt_;

        int type_ = -1;
        int dst_type_ = -1;

        Pylon::CImageFormatConverter converter_;
        Pylon::CPylonImage pylon_image_;
    };

} // namespace vert

#endif /* _CAMERA_H_ */
