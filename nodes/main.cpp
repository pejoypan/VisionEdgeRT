#include <iostream>
#include <vector>
#include <variant>
#include <string>
#include <memory>
#include <pylon/PylonIncludes.h>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "third_party/zmq.hpp"
#include "third_party/cxxopts.hpp"
#include "utils/logging.h"
#include "utils/pylon_utils.h"

#include "basler_camera.h"
#include "basler_emulator.h"
#include "camera_adapter.h"
#include "image_writer.h"

using namespace std;

namespace vert
{
    using node = std::variant<
        vert::BaslerEmulator,
        vert::BaslerCamera,
        vert::CameraAdapter,
        vert::ImageWriter>;
    
    bool create_nodes(zmq::context_t *context, const YAML::Node& config, std::vector<std::unique_ptr<vert::node>>& nodes) {

        nodes.clear();

        auto is_use = [&config](const string& key) {
            if (config[key]) {
                if (config[key]["is_use"])
                    return config[key]["is_use"].as<bool>();
                return true;
            } else return false; 
        };

        if (is_use("basler_emulator") && is_use("basler_camera")) {
            vert::logger->critical("basler_emulator & basler_camera both exists");
            return false; 
        }

        // 1.A
        if (is_use("basler_emulator")) {
            auto emu = std::make_unique<vert::node>(std::in_place_type<vert::BaslerEmulator>, context);
            if (!std::get<vert::BaslerEmulator>(*emu).init(config["basler_emulator"])) 
                return false;
            nodes.push_back(std::move(emu));
        }
        
        // 1.B
        if (is_use("basler_camera")) {
            auto cam = std::make_unique<vert::node>(std::in_place_type<vert::BaslerCamera>, context);
            if (!std::get<vert::BaslerCamera>(*cam).init(config["basler_camera"])) 
                return false;
            nodes.push_back(std::move(cam));
        }
        
        // 2
        if (is_use("camera_adapter")) {
            auto adapter = std::make_unique<vert::node>(std::in_place_type<vert::CameraAdapter>, context);
            if (!std::get<vert::CameraAdapter>(*adapter).init(config["camera_adapter"])) 
                return false;
            nodes.push_back(std::move(adapter));
        }
        
        // 3.A
        if (is_use("image_writer")) {
            auto writer = std::make_unique<vert::node>(std::in_place_type<vert::ImageWriter>, context);
            if (!std::get<vert::ImageWriter>(*writer).init(config["image_writer"])) 
                return false;
            nodes.push_back(std::move(writer));
        }

        // 3.B Algo

        // 4 Result Handler
    
        if (nodes.empty()) {
            vert::logger->critical("Failed to init any node");
            return false;
        }

        vert::logger->info("{} nodes created", nodes.size());

        return true;
    }

}
static zmq::context_t context(2); // 1. send image to ui 2. send log to ui

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

        {
            auto zmq_sink = std::make_shared<vert::zmq_sink_mt>(&context, "tcp://127.0.0.1:5556"); // TODO: configurable
            zmq_sink->set_level(spdlog::level::warn); // now is hard coded to warn
            zmq_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
            sinks.push_back(zmq_sink);
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
   
    vert::enumerate_devices([](const Pylon::CDeviceInfo& device) {
        vert::logger->info("\n{}", vert::get_device_info(device));
    });


    std::vector<std::unique_ptr<vert::node>> nodes;
    if (!vert::create_nodes(&context, config, nodes)) {
        return 1;
    }

    // start
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
        std::visit([](auto& n) { n.start(); }, **it);
    }

    cout << "Press Enter to stop grabbing..." << endl;
    cin.get();

    // stop
    for (auto& node : nodes) {
        std::visit([](auto& n) { n.stop(); }, *node);
    }


    return 0;
}