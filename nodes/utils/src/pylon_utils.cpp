#include "pylon_utils.h"
#include "logging.h"

void Pylon::ConfigEvents::OnAttach(CInstantCamera &)
{
    vert::logger->info("Attaching...");
}

void Pylon::ConfigEvents::OnAttached(CInstantCamera &camera)
{
    vert::logger->info("{} Attached", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnOpen(CInstantCamera &camera)
{
    vert::logger->info("Opening... {}", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnOpened(CInstantCamera &camera)
{
    vert::logger->info("{} Opened", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnGrabStart(CInstantCamera &camera)
{
    vert::logger->info("{} Grab Starting...", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnGrabStarted(CInstantCamera &camera)
{
    vert::logger->info("{} Grab Started", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnGrabStop(CInstantCamera &camera)
{
    vert::logger->info("{} Grab Stopping...", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnGrabStopped(CInstantCamera &camera)
{
    vert::logger->info("{} Grab Stopped", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnClose(CInstantCamera &camera)
{
    vert::logger->info("Closing... {}", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnClosed(CInstantCamera &camera)
{
    vert::logger->info("{} Closed", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnDestroy(CInstantCamera &camera)
{
    vert::logger->info("Destroying... {}", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnDestroyed(CInstantCamera &)
{
    vert::logger->info("a Camera Destroyed");
}

void Pylon::ConfigEvents::OnDetach(CInstantCamera &camera)
{
    vert::logger->info("Detaching... {}", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnDetached(CInstantCamera &camera)
{
    vert::logger->info("{} Detached", vert::brief_info(camera));
}

void Pylon::ConfigEvents::OnGrabError(CInstantCamera &camera, const char *errorMessage)
{
    vert::logger->error("{} Grab Error: {}", vert::brief_info(camera), errorMessage);
}

void Pylon::ConfigEvents::OnCameraDeviceRemoved(CInstantCamera &camera)
{
    vert::logger->warn("{} Camera Device Removed", vert::brief_info(camera));
}

void Pylon::ImageEvents::OnImagesSkipped(CInstantCamera &camera, size_t countOfSkippedImages)
{
    vert::logger->error("{} {} Images Skipped", vert::brief_info(camera), countOfSkippedImages);
}

void Pylon::ImageEvents::OnImageGrabbed(CInstantCamera &camera, const CGrabResultPtr &ptrGrabResult)
{
    vert::logger->trace("{} Image Grabbed: ", vert::brief_info(camera));

    // Image grabbed successfully?
    if (ptrGrabResult->GrabSucceeded()) {
        vert::logger->trace("LifeID: {} FrameID: {} Timestamp: {} ROI: [{}, {}, {}, {}] Depth: {}", 
            ptrGrabResult->GetID(), ptrGrabResult->GetImageNumber(), ptrGrabResult->GetTimeStamp(), 
            ptrGrabResult->GetOffsetX(), ptrGrabResult->GetOffsetY(), ptrGrabResult->GetWidth(), ptrGrabResult->GetHeight(), 
            vert::pixel_type_to_string(ptrGrabResult->GetPixelType()).c_str());
    }
    else
    {
        vert::logger->error("Error: {:x} {}", ptrGrabResult->GetErrorCode(), ptrGrabResult->GetErrorDescription().c_str());
    }
}

void Pylon::CameraEvents::OnCameraEvent(CBaslerUniversalInstantCamera &camera, intptr_t userProvidedId, GenApi::INode *pNode)
{
    vert::logger->trace("{} OnCameraEvent, ID: {} Name: {}", vert::brief_info(camera), userProvidedId, pNode->GetName().c_str());

    switch (userProvidedId)
    {
    case vert::CameraEventsEnum::ExposureEnd:
        vert::logger->trace("Exposure End. FrameID: {} Timestamp: {}", 
            camera.EventExposureEndFrameID.IsReadable() ? camera.EventExposureEndFrameID.GetValue() : camera.ExposureEndEventFrameID.GetValue(), 
            camera.EventExposureEndTimestamp.IsReadable() ? camera.EventExposureEndTimestamp.GetValue() : camera.ExposureEndEventTimestamp.GetValue());
        break;
    
    case vert::CameraEventsEnum::FrameStart:
        vert::logger->trace("Frame Start. FrameID: {} Timestamp: {}", 
            camera.EventFrameStartFrameID.IsReadable() ? camera.EventFrameStartFrameID.GetValue() : -1, 
            camera.EventFrameStartTimestamp.IsReadable() ? camera.EventFrameStartTimestamp.GetValue() : camera.FrameStartEventTimestamp.GetValue());
        break;

    default:
        vert::logger->error("Unknown event.");
        break;
    }
}

bool vert::set_pixel_format(Pylon::CBaslerUniversalInstantCamera &camera, std::string_view format)
{
    std::string format_lower = to_lower(format); 

    using BPFE = Basler_UniversalCameraParams::PixelFormatEnums;
    const static std::map<std::string, std::vector<BPFE>> format_map = {
        {"bgr",  {BPFE::PixelFormat_BGR8, BPFE::PixelFormat_BGR8Packed}},
        {"rgb",  {BPFE::PixelFormat_RGB8, BPFE::PixelFormat_RGB8Packed}},
        {"mono", {BPFE::PixelFormat_Mono8}},
        {"gray", {BPFE::PixelFormat_Mono8}},
        {"mono8",{BPFE::PixelFormat_Mono8}},
        {"bayer",{
            BPFE::PixelFormat_BayerRG8, BPFE::PixelFormat_BayerBG8, 
            BPFE::PixelFormat_BayerGR8, BPFE::PixelFormat_BayerGB8}}
    };

    if (auto it = format_map.find(format_lower); it != format_map.end()) {
        for (const auto& fmt : it->second) {
            if (camera.PixelFormat.CanSetValue(fmt)) {
                camera.PixelFormat.SetValue(fmt);
                return true;
            }
        }
    }

    std::set<std::string> vert_supported_set = {"Mono8", "BGR8Packed", "RGB8Packed", "BayerGR8", "BayerRG8", "BayerGB8", "BayerBG8"};
    std::set<std::string> pylon_supported_set;
    GenApi_3_1_Basler_pylon_v3::StringList_t supported_list;
    camera.PixelFormat.GetSettableValues(supported_list);
    for (const auto& val : supported_list) {
        pylon_supported_set.insert(static_cast<std::string>(val));
    }

    std::set<std::string> supported;
    std::set_intersection(vert_supported_set.begin(), vert_supported_set.end(),
                          pylon_supported_set.begin(), pylon_supported_set.end(), 
                          std::inserter(supported, supported.begin()));
    
    if (supported.find(format.data()) != supported.end()) {
        if (camera.PixelFormat.TrySetValue(format.data())) {
            return true;
        }
    }

    vert::logger->error("{} doesn't support Pixel Format: {}", vert::brief_info(camera), format.data());

    auto gen_supported_str = [&supported](){
        std::string message;
        for (const auto& val : supported)
            message += val + ", ";
        return message;
    };

    vert::logger->info("Supported: [{}]", gen_supported_str().c_str());

    return false;
}

void vert::register_default_events(Pylon::CBaslerUniversalInstantCamera &camera)
{
    camera.RegisterConfiguration( new Pylon::ConfigEvents, Pylon::RegistrationMode_Append /* ReplaceAll */, Pylon::Cleanup_Delete);

    camera.RegisterImageEventHandler( new Pylon::ImageEvents, Pylon::RegistrationMode_Append, Pylon::Cleanup_Delete);

    camera.GrabCameraEvents = true;

    if (!camera.IsOpen()) {
        camera.Open();
    }

    vert::logger->info("\n{}", vert::get_camera_info(camera).c_str());


    // Check if the device supports events.
    if (!camera.EventSelector.IsWritable()) {
        vert::logger->warn("{} doesn't support camera events.", vert::brief_info(camera));
    } else {
        // Cameras based on SFNC 2.0 or later, e.g., USB cameras
        std::string ExposureEndName, FrameStartName;
        if (camera.GetSfncVersion() >= Pylon::Sfnc_2_0_0) {
            ExposureEndName = "EventExposureEnd";
            FrameStartName = "EventFrameStart";
        } else {
            ExposureEndName = "ExposureEndEvent";
            FrameStartName = "FrameStartEvent";
        }
        
        camera.RegisterCameraEventHandler( new Pylon::CameraEvents, ExposureEndName.c_str(), vert::CameraEventsEnum::ExposureEnd, Pylon::RegistrationMode_Append, Pylon::Cleanup_Delete );
        camera.RegisterCameraEventHandler( new Pylon::CameraEvents, FrameStartName.c_str(), vert::CameraEventsEnum::FrameStart, Pylon::RegistrationMode_Append, Pylon::Cleanup_Delete );

        if (camera.EventSelector.TrySetValue(Basler_UniversalCameraParams::EventSelector_ExposureEnd)) {
            camera.EventNotification.TrySetValue(Basler_UniversalCameraParams::EventNotification_On);
        }

        if (camera.EventSelector.TrySetValue(Basler_UniversalCameraParams::EventSelector_FrameStart)) {
            camera.EventNotification.TrySetValue(Basler_UniversalCameraParams::EventNotification_On); 
        }
    } 
}

bool vert::set_image_filename(Pylon::CBaslerUniversalInstantCamera &camera, std::string_view filename)
{
    std::filesystem::path file_path(filename);

    if (!std::filesystem::exists(file_path)) {
        vert::logger->error("file path: {} not exists", file_path.string().c_str());
        return false;
    }

    if (std::filesystem::is_regular_file(file_path)) {
        
        std::string ext = file_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        const std::set<std::string> allowed_ext {".bmp", ".jpg", ".jpeg", ".png", ".tif", ".tiff"};
        if (!allowed_ext.count(ext)) {
            vert::logger->error("Not supported: {}", ext.c_str());
            return false;
        }

        camera.ImageFilename.SetValue(filename.data()); 

    } else if (std::filesystem::is_directory(file_path)) {

        bool has_subdirs = false;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(file_path)) {
                if (entry.is_directory()) {
                    has_subdirs = true;
                    break;
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            vert::logger->error("Directory unaccessible: {}", e.what());
            return false;
        }
        
        if (has_subdirs) {
            vert::logger->error("Should NOT contain subdirectory: {}", file_path.string().c_str());
            return false;
        }

        camera.ImageFilename.SetValue(filename.data());

    } else {
        vert::logger->error("Not file or folder: {}", file_path.string().c_str());
        return false;
    }

    return true;
}
