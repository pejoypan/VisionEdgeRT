#include <vector>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "camera_adapter.h"
#include "../third_party/msgpack.hpp"
#include "../third_party/zmq_addon.hpp"
#include "../utils/pylon_utils.h"
#include "../utils/cv_utils.h"
#include "../utils/logging.h"

using namespace std;

#define WINDOW_NAME "recv dst"
#define WINDOW_NAME_SRC "recv src"
// #define VERT_DEBUG_WINDOW

vert::CameraAdapter::CameraAdapter(zmq::context_t *ctx)
    : publisher_(*ctx, zmq::socket_type::pub),
      subscriber_(*ctx, zmq::socket_type::pull)
{
    converter_.OutputOrientation = Pylon::OutputOrientation_Unchanged;
    converter_.MaxNumThreads = 1;
}

vert::CameraAdapter::~CameraAdapter()
{
    if (is_running())
        stop();
}

bool vert::CameraAdapter::init(const YAML::Node &config)
{
    vert::logger->info("Initializing CameraAdapter ...");

    try {
        if (!config) {
            vert::logger->critical("Failed to init CameraAdapter. Reason: config is empty");
            return false; 
        }

        if (config["name"]) {
            name_ = config["name"].as<string>(); 
        } else {
            vert::logger->warn("name not provided.");
        }

        if (config["port"]) {

            if (config["port"]["from"]) {
                string address = config["port"]["from"].as<string>();
                subscriber_.bind(address);
                subscriber_.set(zmq::sockopt::rcvtimeo, 1000);
                vert::logger->info("{} subscriber connected to {}", name_, address);
            } else {
                vert::logger->critical("Failed to init '{}'. Reason: port.from is empty", name_);
                return false;
            }

            if (config["port"]["to_ui"]) {
                string address = config["port"]["to_ui"].as<string>();
                publisher_.connect(address);
                // publisher_.set(zmq::sockopt::sndhwm, 10);      // limited watermark
                vert::logger->info("{} publisher connected to {}", name_, address);
            } else {
                vert::logger->critical("Failed to init '{}'. Reason: port.to_ui is empty", name_); // TODO: temp return false
                return false;
            }

            if (config["port"]["to_node"]) {
                string address = config["port"]["to_node"].as<string>();
                publisher_.bind(address);
                // publisher_.set(zmq::sockopt::sndhwm, 100);      // high warermark
                vert::logger->info("{} publisher bound to {}", name_, address);
            } else {
                vert::logger->critical("Failed to init '{}'. Reason: port.to_node is empty", name_);
                return false;
            }

        } else {
            vert::logger->critical("Failed to init '{}'. Reason: port is empty", name_);
            return false; 
        }

        if (config["converter"]) {

            if (config["converter"]["use"]) {
                auto choice = config["converter"]["use"].as<string>();
                choice = vert::to_lower(choice);
                if (choice == "pylon") {
                    cfg_.converter_choice = ConverterChoice::Pylon; 
                } else if (choice == "opencv") {
                    cfg_.converter_choice = ConverterChoice::OpenCV; 
                } else {
                    vert::logger->warn("unknown converter.use {}, use default {}", choice, (int)cfg_.converter_choice); 
                }
            } else {
                vert::logger->warn("converter.use not provided, use default {}", (int)cfg_.converter_choice); 
            }

            if (cfg_.converter_choice == ConverterChoice::Pylon) {
                if (config["converter"]["num_threads"] && config["converter"]["num_threads"].as<int>() > 0) {
                    cfg_.pylon_thread_num = config["converter"]["num_threads"].as<int>();
                }
                converter_.MaxNumThreads.TrySetValue(cfg_.pylon_thread_num);
                vert::logger->info("converter.MaxNumThreads set to {}", converter_.MaxNumThreads.GetValue());
            } else if (cfg_.converter_choice == ConverterChoice::OpenCV) {
                if (config["converter"]["demosaicing_flag"]) {
                    int flag = config["converter"]["demosaicing_flag"].as<int>();
                    if (flag >= 0 && flag <= 2) {
                        cfg_.cv_demosacing_flag = (DemosaicingFlag)flag; 
                    }
                }
                vert::logger->info("converter.demosacing_flag set to {}", (int)cfg_.cv_demosacing_flag);
            } else {

            }

        } else {
            vert::logger->warn("converter not provided, use default {}", (int)cfg_.converter_choice);
        }

    } catch (const YAML::Exception& e) {
        vert::logger->critical("Failed to init {}. Reason: {}", name_, e.what());
        return false;
    } catch (const std::exception& e) {
        vert::logger->critical("Failed to init {}. Reason: {}", name_, e.what());
        return false;
    }

    vert::logger->info("CameraAdapter '{}' Inited", name_);
    return true;
}

void vert::CameraAdapter::start()
{
    vert::logger->info("{} starting...", name_);
    if (!is_running_) {
        is_running_ = true;
        loop_thread_ = std::thread(&CameraAdapter::loop, this);
        vert::logger->info("{} started", name_);
    }
}

void vert::CameraAdapter::stop()
{
    vert::logger->info("{} stopping...", name_);
    if (is_running_) {
        is_running_ = false;
        if (loop_thread_.joinable()) {
            loop_thread_.join();
        }
        vert::logger->info("{} stopped", name_);
    }
#ifdef VERT_DEBUG_WINDOW
    cv::destroyWindow(WINDOW_NAME);
#endif
}

cv::Mat vert::CameraAdapter::get_curr_image() const
{
    return cv::Mat();
}

void vert::CameraAdapter::loop()
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

void vert::CameraAdapter::recv()
{
    vector<zmq::message_t> msgs;
    zmq::recv_result_t result = zmq::recv_multipart(subscriber_, std::back_inserter(msgs));
    if (!result)
        return;    
    // assert(result && "recv failed");
    assert(*result == 2);

    auto meta = msgpack::unpack<vert::GrabMeta>(static_cast<const uint8_t *>(msgs[0].data()), msgs[0].size());
    auto src_type = static_cast<Pylon::EPixelType>(meta.pixel_type);

    img_meta_ = MatMeta{meta.device_id, meta.id, meta.height, meta.width, get_output_cv_type(src_type), get_output_cn(src_type), meta.timestamp, meta.error_cnt};

    vert::logger->debug("Recv from Device: {} Image ID: {} Timestamp: {} ({} x {} {}) Error: {}", meta.device_id, meta.id, meta.timestamp, meta.width, meta.height, vert::pixel_type_to_string(src_type), meta.error_cnt);

#ifdef VERT_DEBUG_WINDOW
    cv::Mat temp = cv::Mat(meta.height, meta.width, vert::pixel_type_to_cv_type(src_type), msgs[1].data()).clone();
    cv::imshow(WINDOW_NAME_SRC, temp);
#endif

    vert::logger->debug("{} ---convert--> {}", vert::pixel_type_to_string(src_type), vert::cv_type_to_str(img_meta_.cv_type));
    convert(msgs[1].data(), meta, src_type);

}

void vert::CameraAdapter::cv_convert(void *buffer, const vert::GrabMeta &meta, Pylon::EPixelType src_type)
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

void vert::CameraAdapter::pylon_convert(void *buffer, const vert::GrabMeta &meta, Pylon::EPixelType src_type)
{
    Pylon::EPixelType dst_type = get_output_pylon_type(src_type);
    assert(converter_.IsSupportedOutputFormat(dst_type));
    converter_.OutputPixelFormat = dst_type;

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

void vert::CameraAdapter::convert(void *buffer, const vert::GrabMeta &meta, Pylon::EPixelType src_type)
{
    if (cfg_.converter_choice == ConverterChoice::Pylon) {
        pylon_convert(buffer, meta, src_type);
    } else if (cfg_.converter_choice == ConverterChoice::OpenCV) {
        cv_convert(buffer, meta, src_type);
    } else {
        assert(false);
    }
}

void vert::CameraAdapter::display()
{
    cv::imshow(WINDOW_NAME, img_cvt_);
    cv::waitKey(1);
}

void vert::CameraAdapter::send()
{
    auto meta_data = msgpack::pack(img_meta_);
    zmq::message_t meta_msg(meta_data.data(), meta_data.size());
    publisher_.send(meta_msg, zmq::send_flags::sndmore);

    zmq::message_t img_msg;
    img_msg.rebuild(img_cvt_.data ,img_cvt_.total() * img_cvt_.elemSize());
    publisher_.send(img_msg, zmq::send_flags::dontwait);

    vert::logger->debug("Send Image Device_ID: {} ID: {} Size: {}x{} Type: {} Timestamp: {} Error: {}", img_meta_.device_id, img_meta_.id, img_meta_.width, img_meta_.height, vert::cv_type_to_str(img_meta_.cv_type), img_meta_.timestamp, img_meta_.error_cnt);
}

int vert::CameraAdapter::get_bayer_code(Pylon::EPixelType from) const
{
    static const std::unordered_map<Pylon::EPixelType, std::array<int, 3>> bayer_table = {
        {Pylon::PixelType_BayerRG8, {cv::COLOR_BayerRG2BGR, cv::COLOR_BayerRG2BGR_VNG, cv::COLOR_BayerRG2BGR_EA}},
        {Pylon::PixelType_BayerGR8, {cv::COLOR_BayerGR2BGR, cv::COLOR_BayerGR2BGR_VNG, cv::COLOR_BayerGR2BGR_EA}},
        {Pylon::PixelType_BayerBG8, {cv::COLOR_BayerBG2BGR, cv::COLOR_BayerBG2BGR_VNG, cv::COLOR_BayerBG2BGR_EA}},
        {Pylon::PixelType_BayerGB8, {cv::COLOR_BayerGB2BGR, cv::COLOR_BayerGB2BGR_VNG, cv::COLOR_BayerGB2BGR_EA}}
    };

    auto it = bayer_table.find(from);
    if (it != bayer_table.end()) {
        int index = (int)cfg_.cv_demosacing_flag;
        return it->second[index];
    }

    return -1;
}

bool vert::CameraAdapter::use_pylon_converter(Pylon::EPixelType from) const
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

int vert::CameraAdapter::get_output_cv_type(Pylon::EPixelType from) const
{
    if (Pylon::IsMonoImage(from)) {
        return CV_8UC1; 
    } else if (Pylon::IsColorImage(from)) {
        return CV_8UC3;
    } else {
        return -1; 
    }
}

Pylon::EPixelType vert::CameraAdapter::get_output_pylon_type(Pylon::EPixelType from) const
{
    if (Pylon::IsMonoImage(from)) {
        return Pylon::PixelType_Mono8; 
    } else if (Pylon::IsColorImage(from)) {
        return Pylon::PixelType_BGR8packed; 
    } else {
        return Pylon::PixelType_Undefined;
    }
}

uint8_t vert::CameraAdapter::get_output_cn(Pylon::EPixelType from) const
{
    if (Pylon::IsMonoImage(from)) {
        return 1; 
    } else if (Pylon::IsColorImage(from)) {
        return 3; 
    } else {
        assert(false);
        return -1;
    }
}
