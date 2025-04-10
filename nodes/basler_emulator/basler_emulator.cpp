#include <iostream>
#include <thread>
#include "basler_emulator.h"
#include "../third_party/msgpack.hpp"
#include "../utils/pylon_utils.h"
#include "../utils/timer.h"
#include "../utils/types.h"
#include "../utils/logging.h"

using namespace vert;
using namespace std;
using namespace Pylon;
using namespace Basler_UniversalCameraParams;

vert::BaslerEmulator::BaslerEmulator(zmq::context_t *ctx)
{
    publisher_ = zmq::socket_t(*ctx, zmq::socket_type::pub);


    camera_.RegisterImageEventHandler( this, RegistrationMode_ReplaceAll, Cleanup_None );

}

vert::BaslerEmulator::~BaslerEmulator()
{
    close();
}

bool vert::BaslerEmulator::init(const YAML::Node &config)
{
    vert::logger->info("Init BaslerEmulator");
    try {
        if (!config) {
            vert::logger->critical("Failed to init camera. Reason: config is empty");
            return false; 
        }

        if (config["name"]) {
            name_ = config["name"].as<string>(); 
        } else {
            vert::logger->warn("name not provided.");
        }

        if (config["port"]) {
            publisher_.bind(config["port"].as<string>());
        } else {
            vert::logger->critical("Failed to init '{}'. Reason: port is empty", name_);
            return false;
        }

        if (config["user_id"]) {
            cfg_.user_id = config["user_id"].as<string>(); 
        } else {
            vert::logger->critical("Failed to init '{}'. Reason: user_id is empty", name_);
            return false; 
        }

        if (config["file_path"]) {
            // set @ open()
            cfg_.file_path = config["file_path"].as<string>();
        } else {
            vert::logger->critical("Failed to init '{}'. Reason: file_path is empty", name_);
            return false; 
        }

        if (config["max_images"]) {
            // set @ open()
            cfg_.max_images = config["max_images"].as<int>();
        } else {
            vert::logger->warn("max_images not provided, use default {}.", cfg_.max_images);
        }

        if (config["fps"]) {
            // set @ open()
            cfg_.fps = config["fps"].as<double>(); 
        } else {
            vert::logger->warn("fps not provided, use default {}.", cfg_.fps); 
        }

        if (config["pixel_format"]) {
            // set @ open()
            cfg_.pixel_format = config["pixel_format"].as<string>(); 
        } else {
            vert::logger->warn("pixel_format not provided, use default {}.", cfg_.pixel_format); 
        }

    } catch (const YAML::Exception& e) {
        vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
        return false; 
    } catch (const std::exception& e) {
        vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
        return false; 
    }

    vert::logger->info("BaslerEmulator Inited");

    return create_and_open();
}

bool vert::BaslerEmulator::is_open() const
{
    return camera_.IsOpen();
}

bool vert::BaslerEmulator::is_grabbing() const
{
    return camera_.IsGrabbing();
}

bool vert::BaslerEmulator::create_and_open()
{
    // TODO: mutex ?
    try {
        Pylon::CDeviceInfo di;
        di.SetDeviceClass(Pylon::BaslerCamEmuDeviceClass);
        camera_.Attach( Pylon::CTlFactory::GetInstance().CreateFirstDevice(di), Pylon::Cleanup_Delete );

        // DON'T open camera before register
        
        vert::register_default_events(camera_);

        if (!camera_.DeviceUserID.TrySetValue(cfg_.user_id.c_str())) {
            vert::logger->error("Failed to set user_id: {}", cfg_.user_id);
            return false; 
        }
        // Disable standard test images
        camera_.TestImageSelector.SetValue(TestImageSelector_Off);
        // Enable custom test images
        camera_.ImageFileMode.SetValue(ImageFileMode_On);
        
        camera_.AcquisitionMode.SetValue(AcquisitionMode_Continuous);

        if (!set_image_filename(cfg_.file_path))
            return false;

        set_fps(cfg_.fps);

        if (!set_pixel_format(cfg_.pixel_format))
            return false;

    }
    catch (const Pylon::GenericException& e)
    {
        vert::logger->error("Failed to open camera. Reason: {}", e.what());
        return false;
    }
    catch (const std::exception& e) {
        vert::logger->error("Failed to open camera. Reason: {}", e.what());
        return false;
    }

    return true;
}

void vert::BaslerEmulator::close()
{
    // TODO: mutex?
    stop();

    m_ptrGrabResult.Release();

    // TODO: deregister events?
}

void vert::BaslerEmulator::start()
{
    timer_.start();
    if (cfg_.max_images > 0) {
        camera_.StartGrabbing(cfg_.max_images, GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
    } else {
        camera_.StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
    }
    
}


void vert::BaslerEmulator::stop()
{
    camera_.StopGrabbing();
    vert::logger->info("Elapsed: {} ms", timer_.elapsed());
}

bool vert::BaslerEmulator::set_image_filename(std::string_view filename)
{
    return vert::set_image_filename(camera_, filename);
}

bool vert::BaslerEmulator::set_fps(double fps)
{
    if (camera_.AcquisitionFrameRateEnable.TrySetValue(true)) {
        camera_.AcquisitionFrameRate.SetValue(fps);
    }

    return true;
}

bool vert::BaslerEmulator::set_pixel_format(std::string_view format)
{
    return vert::set_pixel_format(camera_, format);
}

int vert::BaslerEmulator::get_pixel_format() const
{
    auto str = camera_.PixelFormat.ToString();
    return Pylon::CPixelTypeMapper::GetPylonPixelTypeByName(str.c_str());
}

void vert::BaslerEmulator::OnImageGrabbed(Pylon::CInstantCamera &, const Pylon::CGrabResultPtr &ptr)
{
    vert::logger->debug("Grab from Device: {} ID: {} Size: {}x{} Type: {} Timestamp: {}",
        cfg_.user_id, ptr->GetID(), ptr->GetWidth(), ptr->GetHeight(), vert::pixel_type_to_string(ptr->GetPixelType()), ptr->GetTimeStamp());
        
    // Send meta
    auto meta_data = msgpack::pack(GrabMeta{cfg_.user_id,
                                            ptr->GetID(),
                                            ptr->GetHeight(),
                                            ptr->GetWidth(),
                                            (int)ptr->GetPixelType(),
                                            ptr->GetTimeStamp(),
                                            ptr->GetPaddingX(),
                                            ptr->GetBufferSize()}); // camera name, etc

    zmq::message_t meta_msg(meta_data.data(), meta_data.size());
    publisher_.send(meta_msg, zmq::send_flags::sndmore);


    // Send image
    zmq::message_t msg;
    msg.rebuild(ptr->GetBuffer(),
                ptr->GetBufferSize(),
                [](void*, void* hint) {/*deconstruction*/}, 
                nullptr);
    publisher_.send(msg, zmq::send_flags::dontwait);

    vert::logger->trace("Send: meta {} bytes, image {} bytes", meta_msg.size(), msg.size());

    // TODO: what if buffer full?
}
