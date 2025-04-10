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
        void OnAttach( CInstantCamera& /*camera*/ );

        void OnAttached( CInstantCamera& camera );

        void OnOpen( CInstantCamera& camera );

        void OnOpened( CInstantCamera& camera );

        void OnGrabStart( CInstantCamera& camera );

        void OnGrabStarted( CInstantCamera& camera );

        void OnGrabStop( CInstantCamera& camera );

        void OnGrabStopped( CInstantCamera& camera );

        void OnClose( CInstantCamera& camera );

        void OnClosed( CInstantCamera& camera );

        void OnDestroy( CInstantCamera& camera );

        void OnDestroyed( CInstantCamera& /*camera*/ );

        void OnDetach( CInstantCamera& camera );

        void OnDetached( CInstantCamera& camera );
 
        void OnGrabError( CInstantCamera& camera, const char* errorMessage );

        void OnCameraDeviceRemoved( CInstantCamera& camera );

    };


    class ImageEvents : public CImageEventHandler
    {
    public:

        virtual void OnImagesSkipped( CInstantCamera& camera, size_t countOfSkippedImages );

        virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult );

    };


    class CameraEvents : public CBaslerUniversalCameraEventHandler // CCameraEventHandler
    {
    public:
        virtual void OnCameraEvent( CBaslerUniversalInstantCamera& camera, intptr_t userProvidedId, GenApi::INode* pNode );
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

    inline std::string pixel_type_to_string(Pylon::EPixelType pixel_type) {
        return Pylon::CPixelTypeMapper::GetNameByPixelType(pixel_type);
    }

    inline Pylon::EPixelType string_to_pixel_type(std::string_view pixel_type) {
        return Pylon::CPixelTypeMapper::GetPylonPixelTypeByName(pixel_type.data());
    }

    bool set_pixel_format(Pylon::CBaslerUniversalInstantCamera &camera, std::string_view format);

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

    inline std::string brief_info(const Pylon::CInstantCamera &camera) {
        std::stringstream message;
        auto& info = camera.GetDeviceInfo();
        message << (info.IsModelNameAvailable() ? info.GetModelName() : "unknown")
                << "(" << (info.IsSerialNumberAvailable()? info.GetSerialNumber() : "nan") << ")"
                << "'" << (info.IsUserDefinedNameAvailable()? info.GetUserDefinedName() : "") << "'";
        
        return message.str();
    }

    inline std::string get_device_info(const Pylon::CDeviceInfo &info) {
        std::stringstream message;
        message << "====================\n"
                << "Vendor   : " << (info.IsVendorNameAvailable() ? info.GetVendorName() : "") << "\n"
                << "Model    : " << (info.IsModelNameAvailable() ? info.GetModelName() : "") << "\n"
                << "S/N      : " << (info.IsSerialNumberAvailable()? info.GetSerialNumber() : "") << "\n"
                << "User ID  : " << (info.IsUserDefinedNameAvailable() ? info.GetUserDefinedName() : "") << "\n"
                << "====================\n";

        return message.str();
    }

    void register_default_events(Pylon::CBaslerUniversalInstantCamera &camera); 

    bool set_image_filename(Pylon::CBaslerUniversalInstantCamera &camera, std::string_view filename); 

    template<typename Fn>
    void enumerate_devices(Fn&& handler) {
        Pylon::CTlFactory& tl_factory = Pylon::CTlFactory::GetInstance();
        Pylon::DeviceInfoList_t devices;
        tl_factory.EnumerateDevices(devices);
        if (!devices.empty()) {
            vert::logger->info("Found {} devices:", devices.size());
            for (const auto& device : devices) {
                handler(device);
            }
        } else {
            vert::logger->critical("No devices found.");
        }
    }

} // namespace vert



#endif /* _PYLON_UTILS_H_ */
