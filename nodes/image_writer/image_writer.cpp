#include "image_writer.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <opencv2/imgcodecs.hpp>
#include "../third_party/msgpack.hpp"
#include "../third_party/zmq_addon.hpp"
#include "../third_party/fmt/format.h"
#include "../utils/logging.h"
#include "../utils/timer.h"

#include <iostream>
using namespace std;



using namespace std::chrono;
namespace fs = std::filesystem;

vert::SimpleTimer timer("");

vert::ImageWriter::ImageWriter(zmq::context_t *ctx)
    : src_subscriber_(*ctx, zmq::socket_type::sub),
      dst_subscriber_(*ctx, zmq::socket_type::sub)
{
}

vert::ImageWriter::~ImageWriter()
{
    stop();
}

bool vert::ImageWriter::init(const YAML::Node &config)
{
    vert::logger->info("Initializing ImageWriter ...");
    try {
        if (!config) {
            vert::logger->critical("Failed to init {}. Reason: node is empty", name_);
            return false;
        }
        
        if (config["name"]) {
            name_ = config["name"].as<string>(); 
        } else {
            vert::logger->warn("name not provided, use default {}", name_); 
        }

        if (config["level"]) {
            set_level(config["level"].as<string>());
        } else {
            vert::logger->warn("level not provided, use default {}", (int)level_);
        }

        if (config["port"]) {
            if (config["port"]["src"]) {
                string src_port = config["port"]["src"].as<string>();
                vert::logger->info("src suscriber connecting to {} ...", src_port);
                src_subscriber_.connect(src_port);
                src_subscriber_.set(zmq::sockopt::subscribe, "");
            } else if (level_ == ONLY_SRC || level_ == BOTH) {
                vert::logger->error("level is {} but src port not provided", (int)level_);
                return false;
            }
            if (config["port"]["dst"]) {
                string dst_port = config["port"]["dst"].as<string>();
                vert::logger->info("dst suscriber connecting to {}...", dst_port);
                dst_subscriber_.connect(dst_port);
                dst_subscriber_.set(zmq::sockopt::subscribe, "");
            } else if (level_ == ONLY_DST || level_ == BOTH) {
                vert::logger->error("level is {} but dst port not provided", (int)level_);
                return false; 
            }
        } else if (level_on()) {
            vert::logger->critical("Failed to init {}. Reason: level is {} but port is empty", name_, (int)level_);
            return false; 
        }

        if (config["root_path"]) {
            config_.root_path = fs::path(config["root_path"].as<string>());
            if (config_.root_path.empty() && level_on()) {
                vert::logger->critical("Failed to init {}. Reason: level is {} but root_path is empty", name_, (int)level_);
                return false;
            }

            if (!fs::exists(config_.root_path)) {
                fs::create_directories(config_.root_path);
                vert::logger->info("root_path {} created", config_.root_path.string());
            } else {
                vert::logger->warn("root_path {} already exists", config_.root_path.string());
            }
            
        } else if (level_on()) {
            vert::logger->critical("Failed to init {}. Reason: level is {} but root_path is empty", name_, (int)level_);
            return false;
        }

        if (config["recycle_bin"]) {
            config_.recycle_bin = fs::path(config["recycle_bin"].as<string>());
            if (config_.recycle_bin.empty()) {
                config_.recycle_bin = config_.root_path / "recycle_bin";
            }
            
            if (config_.recycle_bin.is_relative()) {
                config_.recycle_bin = config_.root_path / config_.recycle_bin;
            }
        } else {
            config_.recycle_bin = config_.root_path / "recycle_bin"; 
        }

        config_.recycle_bin = config_.recycle_bin.lexically_normal();
        if (!fs::exists(config_.recycle_bin)) {
            fs::create_directories(config_.recycle_bin);
            vert::logger->info("recycle_bin {} created", config_.recycle_bin.string()); 
        } else {
            vert::logger->warn("recycle_bin {} already exists", config_.recycle_bin.string()); 
        }

        if (config["max_rotates"]) {
            config_.max_retained_rotates = config["max_rotates"].as<size_t>(); 
            vert::logger->info("max_rotates set to {}", config_.max_retained_rotates);
        } else {
            vert::logger->warn("max_rotates not provided, use default {}", config_.max_retained_rotates); 
        }

        if (config["max_images"]) {
            config_.max_image_count = config["max_images"].as<size_t>(); 
            vert::logger->info("max_images set to {}", config_.max_image_count);
        } else {
            vert::logger->warn("max_images not provided, use default {}", config_.max_image_count);
        }

        if (config["max_disk_usage"]) {
            size_t max_disk_usage = config["max_disk_usage"].as<int>(); // in GB
            config_.max_image_size = max_disk_usage * 1024 * 1024 * 1024; // convert to bytes
            vert::logger->info("max_disk_usage set to {}", config_.max_image_size);
        } else {
            vert::logger->warn("max_disk_usage not provided, use default {} GB", config_.max_image_size / (1024 * 1024 * 1024)); 
        }


    } catch (const YAML::Exception &e) {
        vert::logger->critical("Failed to init {}. Reason: {}", name_, e.what());
        return false; 
    } catch (const std::exception &e) {
        vert::logger->critical("Failed to init {}. Reason: {}", name_, e.what());
        return false; 
    }

    vert::logger->info("ImageWriter Initialized");
    return true;
}

void vert::ImageWriter::start()
{
    vert::logger->info("Starting...");
    rotate();
    if (!is_running_) {
        is_running_ = true;
        src_thread_ = std::thread(&ImageWriter::loop_src, this);
        dst_thread_ = std::thread(&ImageWriter::loop_dst, this);
        vert::logger->info("Started");
    }
}

void vert::ImageWriter::stop()
{
    vert::logger->info("Stopping...");
    if (is_running_) {
        is_running_ = false;
        if (src_thread_.joinable()) {
            src_thread_.join();
        }
        if (dst_thread_.joinable()) {
            dst_thread_.join();
        }
        vert::logger->info("Stopped");
    }
}

void vert::ImageWriter::set_level(int level)
{
    switch (level) {
        case 0: level_ = OFF; break;
        case 1: level_ = ONLY_SRC; break;
        case 2: level_ = ONLY_DST; break;
        case 3: level_ = BOTH; break;
        default: level_ = OFF; break;
    }
    vert::logger->info("Set level to: {}", (int)level_);
}

void vert::ImageWriter::set_level(std::string_view level)
{
    if (level == "off") {
        level_ = OFF; 
    } else if (level == "only_src") {
        level_ = ONLY_SRC; 
    } else if (level == "only_dst") {
        level_ = ONLY_DST; 
    } else if (level == "both") {
        level_ = BOTH; 
    } else {
        level_ = OFF;
    }
    vert::logger->info("Set level to: {}", (int)level_);
}

void vert::ImageWriter::set_root_path(std::string_view path)
{
    config_.root_path = path;
}

void vert::ImageWriter::rotate()
{

    auto now = system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* tm_ptr = std::localtime(&now_time);

    std::stringstream str_time;
    str_time << std::put_time(tm_ptr, "%H%M%S");

    auto new_path = config_.root_path / str_time.str();

    if (!fs::exists(new_path)) {
        fs::create_directory(new_path);
        current_.rotate_paths.push(new_path);
        // current_.rotate_image_index = 0;
        std::queue<PathWithSize>().swap(current_.current_images);
        current_.rotate_size = 0;

        vert::logger->info("Rotate create path: {}", new_path.string());

        while (current_.rotate_paths.size() > config_.max_retained_rotates) {
            auto old_path = current_.rotate_paths.front();
            current_.rotate_paths.pop();
            remove_folder(old_path);
        }
    }
}

void vert::ImageWriter::loop_src()
{
    while (is_running_) {
        vector<zmq::message_t> msgs;
        zmq::recv_result_t result = zmq::recv_multipart(src_subscriber_, std::back_inserter(msgs));
        assert(result && "recv failed");
        assert(*result == 2);

        auto meta = msgpack::unpack<vert::MatMeta>(static_cast<const uint8_t *>(msgs[0].data()), msgs[0].size());
        assert(meta.cv_type == CV_8UC1 || meta.cv_type == CV_8UC3);
        cv::Mat temp = cv::Mat(meta.height, meta.width, meta.cv_type, msgs[1].data());
    
        vert::logger->trace("Recv SRC ID: {} ({} x {})", meta.id, meta.width, meta.height);

        if (level_ == ONLY_SRC || level_ == BOTH) {
            write(temp, meta); 
        }

    }
}

void vert::ImageWriter::loop_dst()
{
    while (is_running_) {
        vector<zmq::message_t> msgs;
        zmq::recv_result_t result = zmq::recv_multipart(dst_subscriber_, std::back_inserter(msgs));
        assert(result && "recv failed");
        assert(*result == 2);
    
        auto meta = msgpack::unpack<vert::MatMeta>(static_cast<const uint8_t *>(msgs[0].data()), msgs[0].size());
        assert(meta.cv_type == CV_8UC1 || meta.cv_type == CV_8UC3);
        cv::Mat temp = cv::Mat(meta.height, meta.width, meta.cv_type, msgs[1].data());
    
        vert::logger->trace("Recv DST ID: {} ({} x {})", meta.id, meta.width, meta.height);

        if (level_ == ONLY_DST || level_ == BOTH) {
            write(temp, meta); 
        }
    }
}

void vert::ImageWriter::write(const cv::Mat &img, const MatMeta &meta)
{
    std::string filename = fmt::format("{}_{:05d}_src.bmp", meta.device_id, meta.id);
    auto full_path = current_.rotate_paths.back() / filename;
    vert::logger->trace("Writing image: {}", full_path.string());

    cv::imwrite(full_path.string(), img);

    size_t byte_size = img.total() * img.elemSize();
    current_.current_images.push(std::make_tuple(full_path, byte_size));
    current_.rotate_size += byte_size;
    vert::logger->info("Writed image: {}", full_path.string());

    while (current_.current_images.size() > config_.max_image_count) {
        auto [old_path, old_byte_size] = current_.current_images.front();
        current_.current_images.pop();
        remove_file(old_path);
        current_.rotate_size -= old_byte_size;
    }

    while (current_.rotate_size > config_.max_image_size) {
        auto [old_path, old_byte_size] = current_.current_images.front();
        current_.current_images.pop();
        remove_file(old_path);
        current_.rotate_size -= old_byte_size;
    }

}

void vert::ImageWriter::remove_folder(const std::filesystem::path &folder_path)
{
    if (fs::exists(folder_path)) {
        fs::rename(folder_path, config_.recycle_bin / folder_path.lexically_normal().filename()); // O(1)
    }
    vert::logger->warn("Remove folder: {}", folder_path.string());
}

void vert::ImageWriter::remove_file(const std::filesystem::path &file_path)
{
    if (fs::exists(file_path)) {
        fs::rename(file_path, config_.recycle_bin / file_path.filename()); // O(1)
    }
    vert::logger->warn("Remove file: {}", file_path.string());
}



#ifdef VERT_ENABLE_TEST
void vert::ImageWriter::test()
{
    rotate();
}

void vert::ImageWriter::test1()
{
    rotate();
    std::this_thread::sleep_for(milliseconds(100));
    rotate();
}

void vert::ImageWriter::test2()
{
    config_.max_retained_rotates = 3;
    rotate();
    std::this_thread::sleep_for(seconds(1));
    rotate();
    std::this_thread::sleep_for(seconds(1));
    rotate();
    std::this_thread::sleep_for(seconds(1));
    rotate();
    std::this_thread::sleep_for(seconds(1));
    rotate();
}

void vert::ImageWriter::test3()
{
    config_.max_retained_rotates = 3;
    config_.max_image_count = 10;
    rotate();
    for (size_t i = 0; i < 20; i++) {
        MatMeta meta;
        meta.id = i;
        cv::Mat temp(100, 100, CV_8UC3, cv::Scalar(0, 0, 255));
        write(temp, meta);
        std::this_thread::sleep_for(seconds(1));
    }
    
}

void vert::ImageWriter::test4()
{
    config_.max_retained_rotates = 3;
    config_.max_image_size = 20000000;
    rotate();
    for (size_t i = 0; i < 20; i++) {
        MatMeta meta;
        meta.id = i;
        cv::Mat temp(1000, 1000, CV_8UC3, cv::Scalar(0, 0, 255));
        write(temp, meta);
        std::this_thread::sleep_for(seconds(1));
    }
}

void vert::ImageWriter::test5()
{
    config_.max_retained_rotates = 3;
    config_.max_image_count = 95;
    rotate();
    for (size_t i = 0; i < 100; i++) {
        MatMeta meta;
        meta.id = i;
        cv::Mat temp(1000, 1000, CV_8UC3, cv::Scalar(0, 0, 255));
        write(temp, meta);
        std::this_thread::sleep_for(milliseconds(100));
    }
    rotate();
    for (size_t i = 0; i < 100; i++) {
        MatMeta meta;
        meta.id = i;
        cv::Mat temp(1000, 1000, CV_8UC3, cv::Scalar(0, 0, 255));
        write(temp, meta);
        std::this_thread::sleep_for(milliseconds(100));
    }
    rotate();
    for (size_t i = 0; i < 100; i++) {
        MatMeta meta;
        meta.id = i;
        cv::Mat temp(1000, 1000, CV_8UC3, cv::Scalar(0, 0, 255));
        write(temp, meta);
        std::this_thread::sleep_for(milliseconds(100));
    }
    rotate();
    for (size_t i = 0; i < 100; i++) {
        MatMeta meta;
        meta.id = i;
        cv::Mat temp(1000, 1000, CV_8UC3, cv::Scalar(0, 0, 255));
        write(temp, meta);
        std::this_thread::sleep_for(milliseconds(100));
    }
}
#endif