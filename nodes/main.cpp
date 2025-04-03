#include <iostream>
#include <pylon/PylonIncludes.h>
#include "third_party/zmq.hpp"
#include "third_party/cxxopts.hpp"
#include "basler_emulator.h"
#include "camera.h"

// test
#include <thread>
#include <future>
#include <opencv2/opencv.hpp>
#include "third_party/msgpack.hpp"
#include "third_party/zmq_addon.hpp"
#include "utils/types.h"
#include "utils/pylon_utils.h"
// 

using namespace std;
using namespace Pylon;


int main(int argc, char* argv[])
{
    cxxopts::Options options("basler_emulator", "pylon camera emulation, set PYLON_CAMEMU to variable");

    options.add_options()
        ("h,help", "Print help")
        ("img", "Image File/Folder to test", cxxopts::value<string>()->default_value(""))
        ("max", "Max number of images to grab", cxxopts::value<int>()->default_value("100"))
        ("fps", "FPS", cxxopts::value<double>()->default_value("30.0"))
        ("pixel-format", "Mono8, BGR8Packed, RGB8Packed, BayerGR8, BayerRG8, BayerGB8, BayerBG8", cxxopts::value<string>()->default_value("bgr"))
        ;

    options.allow_unrecognised_options();

    auto result = options.parse(argc, argv);
    result.handle_unmatched();

    if (result.count("help")) {
        cout << options.help() << endl;
        return 0;
    }

    Pylon::PylonAutoInitTerm autoInitTerm;  // PylonInitialize() will be called now
    
    zmq::context_t context(0);
    vert::BaslerEmulator camera(&context);

    Pylon::CDeviceInfo devInfo;
    camera.open(devInfo);

    if (result.count("img")) {
        auto filename = result["img"].as<string>();
        if (!camera.set_image_filename(filename))
            return 1;
    } else {
        string message = "img not specified";
        cerr << message << endl;
        return 1;
    }


    if (result.count("fps")) {
        double fps = result["fps"].as<double>();
        camera.set_fps(fps);
    }

    if (result.count("pixel-format")) {
        auto pixel_format = result["pixel-format"].as<string>();
        if (!camera.set_pixel_format(pixel_format)) 
            return 1;
    }

    int max_images = -1;
    if (result.count("max")) {
        max_images = result["max"].as<int>();
    } 

    auto pixel_format = camera.get_pixel_format(); // TODO: simplify this
    int image_format;
    if (Pylon::IsColorImage((Pylon::EPixelType)pixel_format)) {
        image_format = Pylon::PixelType_BGR8packed;
    } else if (Pylon::IsMonoImage((Pylon::EPixelType)pixel_format)) {
        image_format = Pylon::PixelType_Mono8; 
    } else {
        cerr << "unsupported pixel format" << endl;
    }

    vert::Camera camera_thread(&context, image_format);
    camera_thread.start();

    max_images > 0 ? camera.start(max_images) : camera.start();


    cout << "Press Enter to stop grabbing..." << endl;
    cin.get();
    camera.stop();
    camera_thread.stop();



}