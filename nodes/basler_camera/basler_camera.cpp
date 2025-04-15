#include "basler_camera.h"
#include "../third_party/msgpack.hpp"
#include "../utils/pylon_utils.h"
#include "../utils/types.h"
#include "../utils/logging.h"

using namespace std;
using namespace Pylon;
using namespace Basler_UniversalCameraParams;

vert::BaslerCamera::BaslerCamera(zmq::context_t *ctx)
{
    publisher_ = zmq::socket_t(*ctx, zmq::socket_type::pub);

    camera_.RegisterImageEventHandler( this, RegistrationMode_ReplaceAll, Cleanup_None );
}

vert::BaslerCamera::~BaslerCamera()
{
    close();
}

bool vert::BaslerCamera::init(const YAML::Node &config)
{
    vert::logger->info("Initializing BaslerCamera ...");
    try {
        if (!config) {
            vert::logger->critical("Failed to init basler_camera. Reason: config is empty");
            return false;
        }

        if (config["name"]) {
            name_ = config["name"].as<string>(); 
        } else {
            vert::logger->warn("name not provided.");
        }

        if (config["port"]) {
            auto addr = config["port"].as<string>();
            publisher_.bind(addr);
            vert::logger->info("{} bound to {}", name_, addr);
        } else {
            vert::logger->critical("Failed to init '{}'. Reason: port is empty", name_);
            return false; 
        }

        if (config["user_id"]) {
            user_id_ = config["user_id"].as<string>(); 
        } else {
            vert::logger->critical("Failed to init '{}'. Reason: user_id is empty", name_);
            return false;
        }

        if (config["sn"]) {
            sn_ = config["sn"].as<string>();
        } else {
            vert::logger->critical("Failed to init '{}'. Reason: sn is empty", name_);
            return false; 
        }
        
    } catch (const YAML::Exception& e) {
        vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
        return false; 
    } catch (const std::exception& e) {
        vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
        return false; 
    }

    vert::logger->info("{} initialized successfully", name_);

    return create_and_open();
}

bool vert::BaslerCamera::is_open() const
{
    return camera_.IsOpen();
}

bool vert::BaslerCamera::is_grabbing() const
{
    return camera_.IsGrabbing();
}

bool vert::BaslerCamera::create_and_open()
{
    try {
        Pylon::CDeviceInfo di;
        di.SetSerialNumber(sn_.c_str());
        camera_.Attach(Pylon::CTlFactory::GetInstance().CreateDevice(di), Pylon::Cleanup_Delete);

        vert::register_default_events(camera_);

        if (!camera_.DeviceUserID.TrySetValue(user_id_.c_str())) {
            vert::logger->error("{} failed to set user_id: {}", name_, user_id_);
            return false; 
        }
        
    } catch (const Pylon::GenericException& e) {
        vert::logger->error("{} failed to open camera. Reason: {}", name_, e.what());
        return false;
    } catch (const std::exception& e) {
        vert::logger->error("{} failed to open camera. Reason: {}", name_, e.what());
        return false;
    }


    return true;
}

void vert::BaslerCamera::close()
{
    stop();
    m_ptrGrabResult.Release();
}

void vert::BaslerCamera::start()
{
    camera_.StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
}

void vert::BaslerCamera::stop()
{
    camera_.StopGrabbing();
}

void vert::BaslerCamera::OnImageGrabbed(Pylon::CInstantCamera &_camera, const Pylon::CGrabResultPtr &ptr)
{
    std::string user_id = camera_.DeviceUserID.GetValue();
    int64_t frame_id = ptr->GetID();
    uint32_t width = ptr->GetWidth();
    uint32_t height = ptr->GetHeight();
    Pylon::EPixelType pixel_type = ptr->GetPixelType();
    uint64_t timestamp = ptr->GetTimeStamp();
    size_t bufsize = ptr->GetBufferSize();
    
    vert::logger->debug("Grab from Device: '{}' ID: {} Size: {}x{} Type: {} Timestamp: {}",
        user_id, frame_id, width, height, vert::pixel_type_to_string(pixel_type), timestamp);
        
    // Send meta
    auto meta_data = msgpack::pack(GrabMeta{user_id,
                                            frame_id,
                                            height,
                                            width,
                                            (int)pixel_type,
                                            timestamp,
                                            ptr->GetPaddingX(),
                                            bufsize}); // camera name, etc

    zmq::message_t meta_msg(meta_data.data(), meta_data.size());
    publisher_.send(meta_msg, zmq::send_flags::sndmore);


    // Send image
    zmq::message_t msg;
    msg.rebuild(ptr->GetBuffer(),
                bufsize,
                [](void*, void* hint) {/*deconstruction*/}, 
                nullptr);
    publisher_.send(msg, zmq::send_flags::dontwait);

    vert::logger->trace("Send: meta {} bytes, image {} bytes", meta_msg.size(), msg.size());
}
