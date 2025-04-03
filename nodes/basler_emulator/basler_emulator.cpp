#include <iostream>
#include <thread>
#include "basler_emulator.h"
#include "../third_party/msgpack.hpp"
#include "../utils/pylon_utils.h"
#include "../utils/timer.h"
#include "../utils/types.h"

using namespace vert;
using namespace std;
using namespace Pylon;
using namespace Basler_UniversalCameraParams;

vert::BaslerEmulator::BaslerEmulator(zmq::context_t *ctx)
{
    publisher_ = zmq::socket_t(*ctx, zmq::socket_type::pub);
    publisher_.bind("inproc://#1");

    this_thread::sleep_for(chrono::milliseconds(20)); // FIXME wait for others socket to bind

    camera_.RegisterImageEventHandler( this, RegistrationMode_ReplaceAll, Cleanup_None );

}

vert::BaslerEmulator::~BaslerEmulator()
{
    close();
}

bool vert::BaslerEmulator::is_open() const
{
    return camera_.IsOpen();
}

bool vert::BaslerEmulator::is_grabbing() const
{
    return camera_.IsGrabbing();
}

void vert::BaslerEmulator::open(const Pylon::CDeviceInfo &deviceInfo)
{
    // TODO: mutex ?
    try {

        Pylon::IPylonDevice* pDevice = Pylon::CTlFactory::GetInstance().CreateDevice( deviceInfo );
        camera_.Attach( pDevice, Pylon::Cleanup_Delete );

        // DON'T open camera before register
        
        // TODO: what if real camera exists?
        vert::register_default_events(camera_);
        
        // Disable standard test images
        camera_.TestImageSelector.SetValue(TestImageSelector_Off);
        // Enable custom test images
        camera_.ImageFileMode.SetValue(ImageFileMode_On);
        
        camera_.AcquisitionMode.SetValue(AcquisitionMode_Continuous);
    }
    catch (const Pylon::GenericException& e)
    {
        cerr << "Failed to interact with camera. Reason: " << e.what() << endl;
    }
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
    camera_.StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
    
}

void vert::BaslerEmulator::start(int max_images)
{
    timer_.start();
    camera_.StartGrabbing(max_images, GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
}

void vert::BaslerEmulator::stop()
{
    camera_.StopGrabbing();
    cout << "Elapsed: " << timer_.elapsed() << " ms"; // For filemode test
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

void vert::BaslerEmulator::OnImageGrabbed(Pylon::CInstantCamera &, const Pylon::CGrabResultPtr &ptrGrabResult)
{

    // Send meta
    auto meta_data = msgpack::pack(GrabMeta{ptrGrabResult->GetHeight(),
                                            ptrGrabResult->GetWidth(),
                                            (int)ptrGrabResult->GetPixelType(),
                                            ptrGrabResult->GetTimeStamp(),
                                            ptrGrabResult->GetPaddingX(),
                                            ptrGrabResult->GetBufferSize()}); // camera name, etc

    zmq::message_t meta_msg(meta_data.data(), meta_data.size()); // TODO: size?
    publisher_.send(meta_msg, zmq::send_flags::sndmore);


    // Send image
    zmq::message_t msg;
    msg.rebuild(ptrGrabResult->GetBuffer(),
                ptrGrabResult->GetBufferSize(),
                [](void*, void* hint) {/*deconstruction*/}, 
                nullptr);
    publisher_.send(msg, zmq::send_flags::dontwait);

    cout << "Publish Image" << endl;

    // TODO: what if buffer full?
}
