#include <iostream>
#include <pylon/PylonIncludes.h>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "third_party/zmq.hpp"
#include "third_party/cxxopts.hpp"
#include "utils/logging.h"

#include "basler_emulator.h"
#include "camera_adapter.h"
#include "image_writer.h"

using namespace std;

bool init_logger(const YAML::Node& config) {

    try {
        if (!config) {
            cerr << "Failed to init logging. Reason: logging node is empty" << endl;
            return false;
        }

        auto str_level = config["level"] ? config["level"].as<string>() : "info";
        auto global_level = spdlog::level::from_str(str_level);
        if (global_level == spdlog::level::off) {
            cerr << "WARN: log level is off." << endl;
        }
        auto flush_level = config["flush_on"] ? spdlog::level::from_str(config["flush_on"].as<string>()) : global_level;

        vector<shared_ptr<spdlog::sinks::sink>> sinks;

        if (config["console"]) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            string level = config["console"]["level"] ? config["console"]["level"].as<string>() : "info";
            console_sink->set_level(spdlog::level::from_str(level)); 
            console_sink->set_pattern("[%H:%M:%S] %^%l%$ %v");
            sinks.push_back(console_sink);
        }

        if (config["file"]) {
            const auto& filenode = config["file"];
            auto file_level = filenode["level"] ? spdlog::level::from_str(filenode["level"].as<string>()) : global_level;
            if (file_level == spdlog::level::off) {
                cerr << "WARN: file log level is off." << endl; 
            }
            int max_size = filenode["max_size"] ? filenode["max_size"].as<int>() : 100; // 100 MB default
            max_size = max_size * 1024 * 1024;
            int max_num = filenode["max_num"] ? filenode["max_num"].as<int>() : 5; // 5 files default
            string path = filenode["path"] ? filenode["path"].as<string>() : "logs/vert.log";

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(path, max_size, max_num);
            file_sink->set_level(file_level);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
            sinks.push_back(file_sink);
        } else {
            cerr << "Failed to init logging. Reason: file node is empty" << endl;
            return false;
        }

        // auto sink_list = spdlog::sinks_init_list{console_sink, file_sink};
        vert::logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
        vert::logger->set_level(global_level); 
        vert::logger->flush_on(flush_level);

    } catch (const std::exception& e) {
        cerr << "Failed to init logging. Reason: " << e.what() << endl;
        return false; 
    }

    vert::logger->info("**** Welcome to VisionEdgeRT ****");

    return true;
}


int main(int argc, char* argv[])
{             
    cxxopts::Options options("VisionEdgeRT", "if use pylon camera emulation, set PYLON_CAMEMU to variable");

    options.add_options()
        ("h,help", "Print help")
        ("c,config", "Config File", cxxopts::value<string>()->default_value("init.yaml"))
        ;

    options.allow_unrecognised_options();

    auto result = options.parse(argc, argv);
    result.handle_unmatched();

    if (result.count("help")) {
        cout << options.help() << endl;
        return 0;
    }

    YAML::Node config;

    if (result.count("config")) {
        auto config_file = result["config"].as<string>(); 
        try {
            config = YAML::LoadFile(config_file);

        } catch (const YAML::Exception& e) {
            cerr << "Failed to load config file. Reason: " << e.what() << endl;
            return 1; 
        }
    } else {
        cerr << "Config file not specified" << endl;
        return 1;
    }

    if (!init_logger(config["logging"])) {
        return 1;
    }


    Pylon::PylonAutoInitTerm autoInitTerm;  // PylonInitialize() will be called now
    
    zmq::context_t context(0);
    vert::BaslerEmulator camera_emu(&context);
    if (!camera_emu.init(config["basler_emulator"])) {
        return 1;
    }

    Pylon::CDeviceInfo devInfo;
    if (!camera_emu.open(devInfo)) {
        return 1;
    }

    vert::ImageWriter writer(&context);
    if (!writer.init(config["image_writer"])) {
        return 1;
    }

    vert::CameraAdapter camera_adapter(&context);
    if (!camera_adapter.init(config["camera_adapter"])) {
        return 1;
    }

    writer.start();
    camera_adapter.start();
    camera_emu.start();


    cout << "Press Enter to stop grabbing..." << endl;
    cin.get();
    camera_emu.stop();
    camera_adapter.stop();
    writer.stop();



    return 0;
}