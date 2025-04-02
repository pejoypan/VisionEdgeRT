#ifndef _BASLER_EMULATOR_H_
#define _BASLER_EMULATOR_H_
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <string_view>
#include "../third_party/zmq.hpp"
#include "../utils/timer.h"

namespace HPMVA {

    class BaslerEmulator : public Pylon::CImageEventHandler
    {
    public:
        BaslerEmulator(zmq::context_t *ctx);
        ~BaslerEmulator();

        bool is_open() const;
        bool is_grabbing() const;

        void open( const Pylon::CDeviceInfo& deviceInfo );
        void close();

        void start();
        void start(int max_images);
        void stop();

        bool set_image_filename(std::string_view filename);
        bool set_fps(double fps);
        bool set_pixel_format(std::string_view format);

        virtual void OnImageGrabbed(Pylon::CInstantCamera & /*camera*/, const Pylon::CGrabResultPtr &ptrGrabResult);

    private:
        Pylon::CBaslerUniversalInstantCamera camera_;
        Pylon::CGrabResultPtr m_ptrGrabResult;

        zmq::socket_t publisher_;

        SimpleTimer timer_ = SimpleTimer("BaslerEmulator");

    };

} // namespace HPMVA

    
#endif /* _BASLER_EMULATOR_H_ */
