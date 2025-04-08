#ifndef _BASLER_EMULATOR_H_
#define _BASLER_EMULATOR_H_
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <string>
#include <string_view>
#include <yaml-cpp/yaml.h>
#include "../third_party/zmq.hpp"
#include "../utils/timer.h"

namespace vert {

    class BaslerEmulator : public Pylon::CImageEventHandler
    {
        
        struct BaslerEmulatorConfig {
            std::string user_id;
            std::string file_path;
            std::string pixel_format = "BGR8Packed";
            int max_images = -1;
            double fps = 30.0;
        };

    public:
        BaslerEmulator(zmq::context_t *ctx);
        ~BaslerEmulator();

        bool init(const YAML::Node& config);

        bool is_open() const;
        bool is_grabbing() const;

        bool open( const Pylon::CDeviceInfo& deviceInfo );
        void close();

        void start();

        void stop();

        bool set_image_filename(std::string_view filename);
        bool set_fps(double fps);
        bool set_pixel_format(std::string_view format);

        int get_pixel_format() const;

        virtual void OnImageGrabbed(Pylon::CInstantCamera & /*camera*/, const Pylon::CGrabResultPtr &ptrGrabResult);

        std::string name_;

    private:
        Pylon::CBaslerUniversalInstantCamera camera_;
        Pylon::CGrabResultPtr m_ptrGrabResult;

        zmq::socket_t publisher_;

        SimpleTimer timer_ = SimpleTimer("BaslerEmulator");

        BaslerEmulatorConfig cfg_;

    };

} // namespace vert

    
#endif /* _BASLER_EMULATOR_H_ */
