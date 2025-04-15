#include "basler_camera.h"

using namespace std;

vert::BaslerCamera::BaslerCamera(zmq::context_t *ctx)
    : vert::BaslerBase(ctx)
{
}

vert::BaslerCamera::~BaslerCamera()
{
}

void vert::BaslerCamera::start()
{
    camera_.StartGrabbing(Pylon::GrabStrategy_OneByOne, Pylon::GrabLoop_ProvidedByInstantCamera);
}

bool vert::BaslerCamera::device_specific_init(const YAML::Node &config)
{
    try {

        std::string serial_number;
        if (config["sn"]) {
            serial_number = config["sn"].as<string>();
        } else {
            vert::logger->critical("Failed to init '{}'. Reason: sn is empty", name_);
            return false; 
        }

        Pylon::CDeviceInfo di;
        di.SetSerialNumber(serial_number.c_str());
        camera_.Attach(Pylon::CTlFactory::GetInstance().CreateDevice(di), Pylon::Cleanup_Delete);

        vert::register_default_events(camera_);

        if (!camera_.DeviceUserID.TrySetValue(user_id_.c_str())) {
            vert::logger->error("{} failed to set user_id: {}", name_, user_id_);
            return false; 
        }

    } catch (const YAML::Exception& e) {
        vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
        return false; 
    } catch (const Pylon::GenericException& e) {
        vert::logger->error("{} failed to open camera. Reason: {}", name_, e.what());
        return false;
    } catch (const std::exception& e) {
        vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
        return false; 
    }

    vert::logger->info("{} initialized successfully", name_);

    return true;
}
