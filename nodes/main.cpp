#include <iostream>
#include <pylon/PylonIncludes.h>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "third_party/zmq.hpp"
#include "third_party/cxxopts.hpp"
#include "utils/logging.h"

#include "basler_emulator.h"
#include "camera.h"

using namespace std;


int main(int argc, char* argv[])
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::warn); 
    console_sink->set_pattern("[%H:%M:%S] %^%l%$ %v");

    const size_t MAX_LOG_SIZE = 1024 * 1024 * 100; // 100 MB
    const size_t MAX_LOG_FILES = 5;
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/vert.log", MAX_LOG_SIZE, MAX_LOG_FILES);
    file_sink->set_level(spdlog::level::trace);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");

    auto sink_list = spdlog::sinks_init_list{console_sink, file_sink};
    vert::logger = std::make_shared<spdlog::logger>("multi_sink", sink_list.begin(), sink_list.end());
    vert::logger->set_level(spdlog::level::trace); // Set global log level to trace TODO: config
    vert::logger->flush_on(spdlog::level::info); // TODO: config

    vert::logger->info("**** Welcome to VisionEdgeRT ****");
                        
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
        vert::logger->critical("img not specified");
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
        vert::logger->error("unsupported pixel format");
    }

    vert::Camera camera_thread(&context, image_format);
    camera_thread.start();

    max_images > 0 ? camera.start(max_images) : camera.start();


    cout << "Press Enter to stop grabbing..." << endl;
    cin.get();
    camera.stop();
    camera_thread.stop();



}