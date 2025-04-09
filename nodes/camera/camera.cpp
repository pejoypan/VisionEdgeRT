#include <vector>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "camera.h"
#include "../third_party/msgpack.hpp"
#include "../third_party/zmq_addon.hpp"
#include "../utils/pylon_utils.h"
#include "../utils/logging.h"

using namespace std;

#define WINDOW_NAME "recv dst"
#define WINDOW_NAME_SRC "recv src"
// #define VERT_DEBUG_WINDOW

vert::Camera::Camera(zmq::context_t *ctx, int _dst_type)
    : publisher_(*ctx, zmq::socket_type::pub),
      subscriber_(*ctx, zmq::socket_type::sub),
      dst_type_(static_cast<Pylon::EPixelType>(_dst_type))
{
    vert::logger->info("Init Abstract Camera");
    subscriber_.connect("inproc://#1");
    subscriber_.set(zmq::sockopt::subscribe, "");

    publisher_.bind("inproc://#2");
    
    if (converter_.IsSupportedOutputFormat(dst_type_))
        converter_.OutputPixelFormat = dst_type_;
    else {
        vert::logger->error("pylon converter doesn't support {}", vert::pixel_type_to_string(dst_type_));
    }

    converter_.OutputOrientation = Pylon::OutputOrientation_TopDown;
    // converter_.OutputBitAlignment = Pylon::OutputBitAlignment_MsbAligned;
    converter_.MaxNumThreads = 1;
    // converter.MaxNumThreads.TrySetToMaximum();
    vert::logger->info("Abstract Camera Inited");
}

vert::Camera::~Camera()
{
    stop();
}

void vert::Camera::start()
{
    vert::logger->info("Starting...");
    if (!is_running_) {
        is_running_ = true;
        loop_thread_ = std::thread(&Camera::loop, this);
        vert::logger->info("Started");
    }
}

void vert::Camera::stop()
{
    vert::logger->info("Stopping...");
    if (is_running_) {
        is_running_ = false;
        if (loop_thread_.joinable()) {
            loop_thread_.join();
        }
        vert::logger->info("Stopped");
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
    cv::namedWindow(WINDOW_NAME_SRC, cv::WINDOW_AUTOSIZE | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_EXPANDED);
    cv::namedWindow(WINDOW_NAME, cv::WINDOW_AUTOSIZE | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_EXPANDED);
#endif

    while (is_running_) {
        recv();

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

    auto meta = msgpack::unpack<vert::GrabMeta>(static_cast<const uint8_t *>(msgs[0].data()), msgs[0].size());
    auto src_type = static_cast<Pylon::EPixelType>(meta.pixel_type);

    img_meta_ = MatMeta{meta.device_id, meta.id, meta.height, meta.width, vert::pixel_type_to_cv_type(meta.pixel_type), meta.timestamp};

    vert::logger->debug("Recv from Device: {} Image ID: {} Timestamp: {} ({} x {} {})", meta.device_id, meta.id, meta.timestamp, meta.width, meta.height, vert::pixel_type_to_string(src_type));

#ifdef VERT_DEBUG_WINDOW
    cv::Mat temp = cv::Mat(meta.height, meta.width, vert::pixel_type_to_cv_type(src_type), msgs[1].data()).clone();
    cv::imshow(WINDOW_NAME_SRC, temp);
#endif

    convert(msgs[1].data(), meta, src_type);

}

void vert::Camera::cv_convert(void *buffer, const vert::GrabMeta &meta, Pylon::EPixelType src_type)
{
    int src_cv_type = vert::pixel_type_to_cv_type(src_type);
    assert(src_cv_type != -1);

    img_raw_ = cv::Mat(meta.height, meta.width, src_cv_type, buffer);

    if (Pylon::IsMonoImage(src_type)) {
        img_cvt_ = img_raw_;
        // img_raw_.copyTo(img_cvt_);
    } else if (Pylon::IsBayer(src_type)) {
#ifdef USE_CUDA_DEBAYERING

#else
        int code = get_bayer_code(src_type);
        assert(code != -1);
        cv::demosaicing(img_raw_, img_cvt_, code);
#endif
    } else if (Pylon::IsBGR(src_type) || Pylon::IsBGRPacked(src_type)) {
        img_cvt_ = img_raw_;
        // img_raw_.copyTo(img_cvt_);
    } else if (Pylon::IsRGB(src_type) || Pylon::IsRGBPacked(src_type)) {
        cv::cvtColor(img_raw_, img_cvt_, cv::COLOR_RGB2BGR);
    } else {
        assert(false);
    }

}

void vert::Camera::pylon_convert(void *buffer, const vert::GrabMeta &meta, Pylon::EPixelType src_type)
{
    if (converter_.ImageHasDestinationFormat(src_type, meta.padding_x, src_orientation_)) {
        int src_cv_type = vert::pixel_type_to_cv_type(src_type);
        assert(src_cv_type != -1);
        img_raw_ = cv::Mat(meta.height, meta.width, src_cv_type, buffer);
        img_cvt_ = img_raw_;

    } 
    else {
        converter_.Convert(pylon_image_, buffer, meta.buffer_size, src_type, meta.width, meta.height, meta.padding_x, src_orientation_);
        int dst_cv_type = -1;
        if (Pylon::IsMonoImage(src_type))
            dst_cv_type = CV_8UC1;
        else if (Pylon::IsColorImage(src_type))
            dst_cv_type = CV_8UC3;
        assert(dst_cv_type != -1);
        img_cvt_ = cv::Mat(meta.height, meta.width, dst_cv_type, pylon_image_.GetBuffer());
    }
}

void vert::Camera::convert(void *buffer, const vert::GrabMeta &meta, Pylon::EPixelType src_type)
{
    vert::logger->trace("{} ---convert--> {}", vert::pixel_type_to_string(src_type), vert::pixel_type_to_string(dst_type_));

    if (use_pylon_converter(src_type)) {
        pylon_convert(buffer, meta, src_type);
    } else {
        cv_convert(buffer, meta, src_type);
    }
}

void vert::Camera::display()
{
    cv::imshow(WINDOW_NAME, img_cvt_);
    cv::waitKey(1);
}

void vert::Camera::send()
{
    auto meta_data = msgpack::pack(img_meta_);
    zmq::message_t meta_msg(meta_data.data(), meta_data.size());
    publisher_.send(meta_msg, zmq::send_flags::sndmore);

    zmq::message_t img_msg;
    img_msg.rebuild(img_cvt_.data ,img_cvt_.total() * img_cvt_.elemSize());
    publisher_.send(img_msg, zmq::send_flags::dontwait);

    vert::logger->trace("Publish: [meta {} bytes, image {} bytes]", meta_msg.size(), img_msg.size());
}

int vert::Camera::get_bayer_code(Pylon::EPixelType from) const
{
    switch (from)
    {
    case Pylon::PixelType_BayerRG8:
        if (CV_DEMOSAICING_FLAG_ == Bilinear) {
            return cv::COLOR_BayerRG2BGR; 
        } else if (CV_DEMOSAICING_FLAG_ == VNG) {
            return cv::COLOR_BayerRG2BGR_VNG; 
        } else if (CV_DEMOSAICING_FLAG_ == EdgeAware) {
            return cv::COLOR_BayerRG2BGR_EA; 
        } else {
            return cv::COLOR_BayerRG2BGR;
        }
    case Pylon::PixelType_BayerGR8:
        if (CV_DEMOSAICING_FLAG_ == Bilinear) {
            return cv::COLOR_BayerGR2BGR; 
        } else if (CV_DEMOSAICING_FLAG_ == VNG) {
            return cv::COLOR_BayerGR2BGR_VNG; 
        } else if (CV_DEMOSAICING_FLAG_ == EdgeAware) {
            return cv::COLOR_BayerGR2BGR_EA;
        } else {
            return cv::COLOR_BayerGR2BGR;
        }
    case Pylon::PixelType_BayerBG8:
        if (CV_DEMOSAICING_FLAG_ == Bilinear) {
            return cv::COLOR_BayerBG2BGR; 
        } else if (CV_DEMOSAICING_FLAG_ == VNG) {
            return cv::COLOR_BayerBG2BGR_VNG; 
        } else if (CV_DEMOSAICING_FLAG_ == EdgeAware) {
            return cv::COLOR_BayerBG2BGR_EA; 
        } else {
            return cv::COLOR_BayerBG2BGR; 
        }
    case Pylon::PixelType_BayerGB8:
        if (CV_DEMOSAICING_FLAG_ == Bilinear) {
            return cv::COLOR_BayerGB2BGR; 
        } else if (CV_DEMOSAICING_FLAG_ == VNG) {
            return cv::COLOR_BayerGB2BGR_VNG; 
        } else if (CV_DEMOSAICING_FLAG_ == EdgeAware) {
            return cv::COLOR_BayerGB2BGR_EA;
        } else {
            return cv::COLOR_BayerGB2BGR;
        }
    default:
        return -1;
    }
}

bool vert::Camera::use_pylon_converter(Pylon::EPixelType from) const
{
    return !(
        Pylon::IsMonoImage(from) ||
        // Pylon::IsBayer(from) ||  // temporarily use pylon converter
        Pylon::IsBGR(from) ||
        Pylon::IsBGRPacked(from) ||
        Pylon::IsRGB(from) ||
        Pylon::IsRGBPacked(from)
    );
}
