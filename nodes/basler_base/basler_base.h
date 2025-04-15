#ifndef _BASLER_BASE_H_
#define _BASLER_BASE_H_

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <string>
#include <yaml-cpp/yaml.h>
#include "../third_party/zmq.hpp"
#include "../third_party/msgpack.hpp"
#include "../utils/logging.h"
#include "../utils/pylon_utils.h"
#include "../utils/types.h"

namespace vert {
    
class BaslerBase : public Pylon::CImageEventHandler {
public:
    BaslerBase(zmq::context_t *ctx)
        : publisher_(*ctx, zmq::socket_type::pub)
    {
        camera_.RegisterImageEventHandler(this, Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_None);
    }

    virtual ~BaslerBase() {
        close(); 
    }

    bool init(const YAML::Node& config) {
        vert::logger->info("Initializing BaslerBase ...");
        try {
            if (!config) {
                vert::logger->critical("Failed to init basler_base. Reason: config is empty");
                return false;
            }
    
            if (config["name"]) {
                name_ = config["name"].as<std::string>(); 
            } else {
                vert::logger->warn("name not provided.");
            }
    
            if (config["port"]) {
                auto addr = config["port"].as<std::string>();
                publisher_.bind(addr);
                vert::logger->info("{} bound to {}", name_, addr);
            } else {
                vert::logger->critical("Failed to init '{}'. Reason: port is empty", name_);
                return false; 
            }
    
            if (config["user_id"]) {
                user_id_ = config["user_id"].as<std::string>(); 
            } else {
                vert::logger->critical("Failed to init '{}'. Reason: user_id is empty", name_);
                return false;
            }
            
        } catch (const YAML::Exception& e) {
            vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
            return false; 
        } catch (const std::exception& e) {
            vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
            return false; 
        }
    
        return device_specific_init(config);
    }

    bool is_open() const { return camera_.IsOpen(); }
    bool is_grabbing() const { return camera_.IsGrabbing(); }

    void close() {
        if (is_grabbing())
            stop();
        m_ptrGrabResult.Release();
        if (camera_.IsOpen())
            camera_.Close();
    }
    virtual void start() = 0;

    void stop() {
        camera_.StopGrabbing();
    }

    virtual void OnImageGrabbed(Pylon::CInstantCamera & _camera, const Pylon::CGrabResultPtr &ptr) override {
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
        auto meta_data = msgpack::pack(vert::GrabMeta{user_id,
                                                      frame_id,
                                                      height,
                                                      width,
                                                      (int)pixel_type,
                                                      timestamp,
                                                      ptr->GetPaddingX(),
                                                      bufsize});
    
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

    std::string name_;

protected:
    Pylon::CBaslerUniversalInstantCamera camera_;
    Pylon::CGrabResultPtr m_ptrGrabResult;
    zmq::socket_t publisher_;
    std::string user_id_;

    virtual bool device_specific_init(const YAML::Node& config) = 0;

};

} // namespace vert


#endif /* _BASLER_BASE_H_ */
