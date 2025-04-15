#include "basler_emulator.h"

using namespace vert;
using namespace std;
using namespace Pylon;
using namespace Basler_UniversalCameraParams;

vert::BaslerEmulator::BaslerEmulator(zmq::context_t *ctx)
    : BaslerBase(ctx)
{
}

vert::BaslerEmulator::~BaslerEmulator()
{
}

void vert::BaslerEmulator::start()
{
    if (cfg_.max_images > 0) {
        camera_.StartGrabbing(cfg_.max_images, GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
    } else {
        camera_.StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
    }
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

bool vert::BaslerEmulator::device_specific_init(const YAML::Node &config)
{
    try {
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

        Pylon::CDeviceInfo di;
        di.SetDeviceClass(Pylon::BaslerCamEmuDeviceClass);
        camera_.Attach( Pylon::CTlFactory::GetInstance().CreateFirstDevice(di), Pylon::Cleanup_Delete );

        // DON'T open camera before register
        
        vert::register_default_events(camera_);

        if (!camera_.DeviceUserID.TrySetValue(user_id_.c_str())) {
            vert::logger->error("{} failed to set user_id: {}", name_, user_id_);
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


    } catch (const YAML::Exception& e) {
        vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
        return false; 
    } catch (const Pylon::GenericException& e) {
        vert::logger->error("{} Failed to open camera. Reason: {}", name_, e.what());
        return false;
    } catch (const std::exception& e) {
        vert::logger->critical("Failed to init '{}'. Reason: {}", name_, e.what());
        return false; 
    }

    vert::logger->info("{} initialized successfully", name_);

    return true;
}
