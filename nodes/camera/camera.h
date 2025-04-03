#ifndef _CAMERA_H_
#define _CAMERA_H_
#include <opencv2/core.hpp>
#include <atomic>
#include <thread>
#include <mutex>
#include <pylon/PylonIncludes.h>
#include "../utils/types.h"
#include "../third_party/zmq.hpp"

/*
    TODO:
    - compare converter performance: pylon thread num, opencv VNG, opencv EdgeAware
    - mutex necessary? recv, get_curr_image, display, convert
    - how to get EImageOrientation from camera?
    - use opencv bayer demosaicing color incorrect @ file mode
    - need recv and send split to 2 threads?
*/

namespace vert {

    class Camera
    {
        enum DemosaicingFlag {
            Bilinear = 0,  // Using bilinear interpolation, fast but not good
            VNG = 1,       // Variable Number of Gradients, slower but better
            EdgeAware = 2  // Edge-Aware Demosaicing, slowest but best
        };

    public:
        Camera(zmq::context_t *ctx, int _dst_type);
        ~Camera();

        void start();
        void stop();
        cv::Mat get_curr_image() const;
        bool is_running() const {return is_running_.load();}

        void set_demosacing_flag(int flag) {CV_DEMOSAICING_FLAG_ = flag;}

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

        zmq::socket_t publisher_;
        zmq::socket_t subscriber_;

        std::atomic<bool> is_running_{false};

        std::thread loop_thread_;
        mutable std::mutex image_mutex_;

        cv::Mat img_raw_;
        cv::Mat img_cvt_;

        int CV_DEMOSAICING_FLAG_ = Bilinear;

        Pylon::EPixelType dst_type_ = Pylon::EPixelType::PixelType_Undefined;
        Pylon::EImageOrientation src_orientation_ = Pylon::ImageOrientation_TopDown;

        Pylon::CImageFormatConverter converter_;
        Pylon::CPylonImage pylon_image_;
    };

} // namespace vert

#endif /* _CAMERA_H_ */
