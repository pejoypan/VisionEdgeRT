#ifndef _PYLON_UTILS_H_
#define _PYLON_UTILS_H_

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <filesystem>
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/ConfigurationEventHandler.h>
#include <pylon/CameraEventHandler.h>
#include <pylon/ImageEventHandler.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/PixelType.h>
#include <opencv2/core.hpp>
#include "string_utils.h"

namespace vert
{
    enum CameraEventsEnum {
        ExposureEnd = 1, // used
        Overrun = 2,
        FrameStart = 3, // used
        FrameTriggerMissed = 4,
        FrameBufferOverrun = 5
    };
}

namespace Pylon
{
    class CInstantCamera;

    class ConfigEvents : public CConfigurationEventHandler
    {
    public:
        void OnAttach( CInstantCamera& /*camera*/ )
        {
            std::string message = "Attaching ...";
            std::cout << message << std::endl;
        }

        void OnAttached( CInstantCamera& camera )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Attached";
            std::cout << message << std::endl;
        }

        void OnOpen( CInstantCamera& camera )
        {
            std::string message = "Opening... <" + camera.GetDeviceInfo().GetModelName() + ">";
            std::cout << message << std::endl;
        }

        void OnOpened( CInstantCamera& camera )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Opened";
            std::cout << message << std::endl;
        }

        void OnGrabStart( CInstantCamera& camera )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Grab Starting...";
            std::cout << message << std::endl;
        }

        void OnGrabStarted( CInstantCamera& camera )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Grab Started";
            std::cout << message << std::endl;
        }

        void OnGrabStop( CInstantCamera& camera )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Grab Stopping...";
            std::cout << message << std::endl;
        }

        void OnGrabStopped( CInstantCamera& camera )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Grab Stopped";
            std::cout << message << std::endl;
        }

        void OnClose( CInstantCamera& camera )
        {
            std::string message = "Closing... <" + camera.GetDeviceInfo().GetModelName() + ">";
            std::cout << message << std::endl;
        }

        void OnClosed( CInstantCamera& camera )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Closed";
            std::cout << message << std::endl;
        }

        void OnDestroy( CInstantCamera& camera )
        {
            std::string message = "Destroying... <" + camera.GetDeviceInfo().GetModelName() + ">";
            std::cout << message << std::endl;
        }

        void OnDestroyed( CInstantCamera& /*camera*/ )
        {
            std::string message = "a Camera Destroyed";
            std::cout << message << std::endl;
        }

        void OnDetach( CInstantCamera& camera )
        {
            std::string message = "Detaching... <" + camera.GetDeviceInfo().GetModelName() + ">";
            std::cout << message << std::endl;
        }

        void OnDetached( CInstantCamera& camera )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Detached";
            std::cout << message << std::endl;
        }

        void OnGrabError( CInstantCamera& camera, const char* errorMessage )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Grab Error: ";
            message += errorMessage;
            std::cout << message << std::endl;
        }

        void OnCameraDeviceRemoved( CInstantCamera& camera )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Camera Device Removed";
            std::cout << message << std::endl;
        }
    };


    class ImageEvents : public CImageEventHandler
    {
    public:

        virtual void OnImagesSkipped( CInstantCamera& camera, size_t countOfSkippedImages )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> ";
            message += std::to_string(countOfSkippedImages) + " Images Skipped.";
            std::cout << message << std::endl;
        }


        virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult )
        {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> Image Grabbed:";
            std::cout << message << std::endl;

            // Image grabbed successfully?
            if (ptrGrabResult->GrabSucceeded()) {
                std::stringstream message1;
                message1 << "LifeID: " << ptrGrabResult->GetID() << " FrameID: " << ptrGrabResult->GetImageNumber() << " Timestamp: " << ptrGrabResult->GetTimeStamp() << std::endl;
                message1 << "ROI: [" << ptrGrabResult->GetOffsetX() << ", " << ptrGrabResult->GetOffsetY() << ", " << ptrGrabResult->GetWidth() << ", " << ptrGrabResult->GetHeight() << "] ";
                message1 << "Depth: " << CPixelTypeMapper::GetNameByPixelType(ptrGrabResult->GetPixelType());

                std::cout << message1.str() << std::endl;;
            }
            else
            {
                std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
            }
        }
    };


    class CameraEvents : public CBaslerUniversalCameraEventHandler // CCameraEventHandler
    {
    public:
        virtual void OnCameraEvent( CBaslerUniversalInstantCamera& camera, intptr_t userProvidedId, GenApi::INode* pNode )
        {
            std::cout << "OnCameraEvent event for device " << camera.GetDeviceInfo().GetModelName() << std::endl;
            std::cout << "User provided ID: " << userProvidedId << std::endl;
            std::cout << "Event data node name: " << pNode->GetName() << std::endl;

            switch (userProvidedId)
            {
            case vert::CameraEventsEnum::ExposureEnd:
                if (camera.EventExposureEndFrameID.IsReadable()) {  // Applies to cameras based on SFNC 2.0 or later, e.g, USB cameras
                    std::cout << "Exposure End event. FrameID: " << camera.EventExposureEndFrameID.GetValue() << " Timestamp: " << camera.EventExposureEndTimestamp.GetValue() << std::endl;
                } else {
                    std::cout << "Exposure End event. FrameID: " << camera.ExposureEndEventFrameID.GetValue() << " Timestamp: " << camera.ExposureEndEventTimestamp.GetValue() << std::endl;
                }
                break;
            
            case vert::CameraEventsEnum::FrameStart:
                if (camera.EventFrameStartFrameID.IsReadable()) {  // Applies to cameras based on SFNC 2.0 or later, e.g, USB cameras
                    std::cout << "Frame Start event. FrameID: " << camera.EventFrameStartFrameID.GetValue() << " Timestamp: " << camera.EventFrameStartTimestamp.GetValue() << std::endl;
                }
            default:
                std::cout << "Unknown event." << std::endl;
                break;
            }
        }
    };



} // namespace Pylon

namespace vert
{
    inline int pixel_type_to_cv_type(int pixel_type) {
        switch (pixel_type) {
            case Pylon::PixelType_Mono8: return CV_8UC1;
            case Pylon::PixelType_BayerRG8: return CV_8UC1;
            case Pylon::PixelType_BayerBG8: return CV_8UC1;
            case Pylon::PixelType_BayerGR8: return CV_8UC1;
            case Pylon::PixelType_BayerGB8: return CV_8UC1;
            case Pylon::PixelType_RGB8packed: return CV_8UC3;
            case Pylon::PixelType_BGR8packed: return CV_8UC3;
            default: return -1;
        } 
    }

    inline bool set_pixel_format(Pylon::CBaslerUniversalInstantCamera &camera, std::string_view format) {
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

        std::string message = "<" + std::string(camera.GetDeviceInfo().GetModelName()) + "> doesn't support Pixel Format: " + format.data() + ". Supported: [";
        GenApi_3_1_Basler_pylon_v3::StringList_t supported_list;
        camera.PixelFormat.GetSettableValues(supported_list);
        for (const auto& val : supported_list) {
            message += val + ", "; 
        }
        message += "]";
        std::cerr << message << std::endl;

        return false;
    }

    inline std::string get_camera_info(const Pylon::CBaslerUniversalInstantCamera &camera) {
        std::stringstream message;
        message << "Camera Device Information\n"
                << "=========================\n"
                << "Vendor   : " << (camera.DeviceVendorName.IsReadable() ? camera.DeviceVendorName.GetValue() : "") << "\n"
                << "Model    : " << (camera.DeviceModelName.IsReadable() ? camera.DeviceModelName.GetValue() : "") << "\n"
                << "Firmware : " << (camera.DeviceFirmwareVersion.IsReadable() ? camera.DeviceFirmwareVersion.GetValue() : "") << "\n"
                << "S/N      : " << (camera.DeviceSerialNumber.IsReadable() ? camera.DeviceSerialNumber.GetValue() : "") << "\n"
                << "User ID  : " << (camera.DeviceUserID.IsReadable()? camera.DeviceUserID.GetValue() : "") << "\n";

        return message.str(); 
    }

    inline void register_default_events(Pylon::CBaslerUniversalInstantCamera &camera) {

        camera.RegisterConfiguration( new Pylon::ConfigEvents, Pylon::RegistrationMode_Append /* ReplaceAll */, Pylon::Cleanup_Delete);

        camera.RegisterImageEventHandler( new Pylon::ImageEvents, Pylon::RegistrationMode_Append, Pylon::Cleanup_Delete);

        camera.GrabCameraEvents = true;

        if (!camera.IsOpen()) {
            camera.Open();
        }

        std::cout << vert::get_camera_info(camera) << std::endl;

        // Check if the device supports events.
        if (!camera.EventSelector.IsWritable()) {
            std::string message = "<" + camera.GetDeviceInfo().GetModelName() + "> doesn't support events.";
            std::cout << message << std::endl;
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

    inline bool set_image_filename(Pylon::CBaslerUniversalInstantCamera &camera, std::string_view filename) {

        std::filesystem::path file_path(filename);

        if (!std::filesystem::exists(file_path)) {
            std::cerr << "Not exists: " << file_path << std::endl;
            return false;
        }

        if (std::filesystem::is_regular_file(file_path)) {
            
            std::string ext = file_path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            const std::set<std::string> allowed_ext {".bmp", ".jpg", ".jpeg", ".png", ".tif", ".tiff"};
            if (!allowed_ext.count(ext)) {
                std::cerr << "Unsupported: " << ext << std::endl;
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
                std::cerr << "Directory unaccessible: " << e.what() << std::endl;
                return false;
            }
            
            if (has_subdirs) {
                std::cerr << "Should NOT contain subdirectory: " << file_path << std::endl;
                return false;
            }

            camera.ImageFilename.SetValue(filename.data());

        } else {

            std::cerr << "Not file or folder: " << file_path << std::endl;
            return false;
        }

        return true;
    }

} // namespace vert



#endif /* _PYLON_UTILS_H_ */
