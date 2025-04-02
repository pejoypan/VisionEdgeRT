#include <vector>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "camera.h"
#include "../third_party/msgpack.hpp"
#include "../third_party/zmq_addon.hpp"
#include "../utils/pylon_utils.h"

using namespace std;

#define WINDOW_NAME "recv"
// #define VERT_DEBUG_WINDOW

vert::Camera::Camera(zmq::context_t *ctx, int _dst_type)
    : publisher_(*ctx, zmq::socket_type::pub),
      subscriber_(*ctx, zmq::socket_type::sub),
      dst_type_(_dst_type)
{
    publisher_.bind("inproc://#2");
    subscriber_.connect("inproc://#1");
    subscriber_.set(zmq::sockopt::subscribe, "");

    converter_.OutputPixelFormat = (Pylon::EPixelType)dst_type_; // Pylon::PixelType_BGR8packed;
    // converter_.OutputBitAlignment = Pylon::OutputBitAlignment_MsbAligned;
    converter_.MaxNumThreads = 1;
    // converter.MaxNumThreads.TrySetToMaximum();

}

vert::Camera::~Camera()
{
    stop();
}

void vert::Camera::start()
{
    if (!is_running_) {
        is_running_ = true;
        loop_thread_ = std::thread(&Camera::loop, this);
    }
}

void vert::Camera::stop()
{
    if (is_running_) {
        is_running_ = false;
        if (loop_thread_.joinable()) {
            loop_thread_.join();
        }
    }
#ifdef VERT_DEBUG_WINDOW
    cv::destroyWindow(WINDOW_NAME);
#endif
}

cv::Mat vert::Camera::get_curr_image() const
{
    return cv::Mat();
}

void vert::Camera::loop()
{
#ifdef VERT_DEBUG_WINDOW
    cv::namedWindow(WINDOW_NAME, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
#endif

    while (is_running_) {
        recv();

        convert(static_cast<Pylon::EPixelType>(type_));
#ifdef VERT_DEBUG_WINDOW
        display();
#endif
        send();
    }
}

void vert::Camera::recv()
{
    vector<zmq::message_t> msgs;
    zmq::recv_result_t result = zmq::recv_multipart(subscriber_, std::back_inserter(msgs));
    assert(result && "recv failed");
    assert(*result == 2);

    auto [h, w, type, timestamp] = msgpack::unpack<vert::GrabMeta>(static_cast<const uint8_t *>(msgs[0].data()), msgs[0].size());

    type_ = type;

    img_raw_ = cv::Mat(h, w, vert::pixel_type_to_cv_type(type_), msgs[1].data());

    cout << "Recv Image " << img_raw_.size() << "Type: " << Pylon::CPixelTypeMapper::GetNameByPixelType((Pylon::EPixelType)type_) << endl;
}

void vert::Camera::convert(Pylon::EPixelType from)
{

    // TODO: need lock ?
    if (Pylon::IsMono(from)) {
        cout << "Mono" << endl;
        img_cvt_ = img_raw_;
        // img_raw_.copyTo(img_cvt_);
    } else if (Pylon::IsBayer(from)) {
        cout << "Bayer" << endl;
#ifdef USE_CUDA_DEBAYERING

#else
        int code = -1;
        switch (from) {
        case Pylon::PixelType_BayerRG8:
            code = cv::COLOR_BayerRG2BGR;
            break;
        case Pylon::PixelType_BayerGR8:
            code = cv::COLOR_BayerGR2BGR;
            break;
        case Pylon::PixelType_BayerBG8:
            code = cv::COLOR_BayerBG2BGR;
            break;
        case Pylon::PixelType_BayerGB8:
            code = cv::COLOR_BayerGB2BGR;
            break;
        default:
            break; 
        }
        assert(code != -1);
        cv::cvtColor(img_raw_, img_cvt_, code);
#endif
    } else if (Pylon::IsBGR(from) || Pylon::IsBGRPacked(from)) {
        cout << "BGR" << endl;
        img_cvt_ = img_raw_;
        // img_raw_.copyTo(img_cvt_);
    } else if (Pylon::IsRGB(from) || Pylon::IsRGBPacked(from)) {
        cout << "RGB" << endl;
        cv::cvtColor(img_raw_, img_cvt_, cv::COLOR_RGB2BGR);
    } else {
        // converter_.ImageHasDestinationFormat() // TODO: more info in msgpack
    }

}

void vert::Camera::display()
{
    // cv::namedWindow(WINDOW_NAME, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    cv::imshow(WINDOW_NAME, img_cvt_);
    cv::waitKey(1);
}

void vert::Camera::send()
{
}
