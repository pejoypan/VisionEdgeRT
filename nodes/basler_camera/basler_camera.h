#ifndef _BASLER_CAMERA_H_
#define _BASLER_CAMERA_H_
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <string>
#include <yaml-cpp/yaml.h>
#include "../third_party/zmq.hpp"

namespace vert {

    class BaslerCamera : public Pylon::CImageEventHandler
    {
        
    public:
        BaslerCamera(zmq::context_t *ctx);
        ~BaslerCamera();

        bool init(const YAML::Node& config);

        bool is_open() const;
        bool is_grabbing() const;

        bool create_and_open();
        void close();

        void start();

        void stop();

        virtual void OnImageGrabbed(Pylon::CInstantCamera & /*camera*/, const Pylon::CGrabResultPtr &ptrGrabResult) override;

        std::string name_;

    private:
        Pylon::CBaslerUniversalInstantCamera camera_;
        Pylon::CGrabResultPtr m_ptrGrabResult;

        zmq::socket_t publisher_;

        std::string sn_;
        std::string user_id_;

    };

} // namespace vert

#endif /* _BASLER_CAMERA_H_ */
