#ifndef _CAMERA_ADAPTER_H_
#define _CAMERA_ADAPTER_H_
#include <opencv2/core.hpp>
#include <atomic>
#include <thread>
#include <mutex>
#include <pylon/PylonIncludes.h>
#include <yaml-cpp/yaml.h>
#include "../utils/types.h"
#include "../third_party/zmq.hpp"

/*
    TODO:
    - compare converter performance: pylon thread num, opencv VNG, opencv EdgeAware
    - mutex necessary? recv, get_curr_image, display, convert
    - BUG using opencv demosaicing (change to RGB can fix)
    - LATER: need recv and send split to 2 threads?
*/

namespace vert {

    class CameraAdapter
    {
        enum DemosaicingFlag {
            Bilinear = 0,  // Using bilinear interpolation, fast but not good
            VNG = 1,       // Variable Number of Gradients, slower but better
            EdgeAware = 2  // Edge-Aware Demosaicing, slowest but best
        };

        enum ConverterChoice {
            Pylon = 0,
            OpenCV = 1 
        };

        struct CameraAdapterConfig {
            int pylon_thread_num = 1;
            CameraAdapter::DemosaicingFlag cv_demosacing_flag = CameraAdapter::DemosaicingFlag::Bilinear;
            CameraAdapter::ConverterChoice converter_choice = CameraAdapter::ConverterChoice::Pylon;
        };

    public:
        CameraAdapter(zmq::context_t *ctx);
        ~CameraAdapter();

        bool init(const YAML::Node &config);

        void start();
        void stop();
        cv::Mat get_curr_image() const;
        bool is_running() const {return is_running_.load();}


    private:
        void loop();

        void recv();

        void cv_convert(void *buffer, const vert::GrabMeta &meta, Pylon::EPixelType src_type);

        void pylon_convert(void *buffer, const vert::GrabMeta &meta, Pylon::EPixelType src_type);

        void convert(void *buffer, const vert::GrabMeta &meta, Pylon::EPixelType src_type);

        void display();
        
        void send();

        int get_bayer_code(Pylon::EPixelType from) const;

        bool use_pylon_converter(Pylon::EPixelType from) const;

        int get_output_cv_type(Pylon::EPixelType from) const;

        Pylon::EPixelType get_output_pylon_type(Pylon::EPixelType from) const;

        zmq::socket_t publisher_;
        zmq::socket_t subscriber_;

        std::atomic<bool> is_running_{false};

        std::thread loop_thread_;
        mutable std::mutex image_mutex_;

        cv::Mat img_raw_;
        cv::Mat img_cvt_;

        MatMeta img_meta_;

        Pylon::EImageOrientation src_orientation_ = Pylon::ImageOrientation_TopDown; // we assume it's top down

        Pylon::CImageFormatConverter converter_;
        Pylon::CPylonImage pylon_image_;

        CameraAdapterConfig cfg_;

        std::string name_ = "CameraAdapter";
    };

} // namespace vert

#endif /* _CAMERA_ADAPTER_H_ */
