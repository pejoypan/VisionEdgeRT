#ifndef _IMAGE_WRITER_H_
#define _IMAGE_WRITER_H_


#include <string_view>
#include <atomic>
#include <thread>
#include <filesystem>
#include <queue>
#include <tuple>
#include <opencv2/core.hpp>
#include <yaml-cpp/yaml.h>
#include "../utils/types.h"
#include "../third_party/zmq.hpp"




namespace vert {
    using PathWithSize = std::tuple<std::filesystem::path, size_t>;
    
    class ImageWriter
    {
        struct ImageWriterConfig {
            std::filesystem::path root_path;
            std::filesystem::path recycle_bin;
            size_t max_image_count = 30000;      // per rotation
            size_t max_image_size = 32212254720; // 30GB
            size_t max_retained_rotates = 10;
        };
        struct ImageWriterState {
            std::queue<std::filesystem::path> rotate_paths;
            std::queue<PathWithSize> current_images;
            size_t rotate_size = 0;
        };

        enum Level {
            OFF = 0,
            ONLY_SRC,
            ONLY_DST,
            BOTH
        };

    public:
        ImageWriter(zmq::context_t *ctx);
        ~ImageWriter();

        bool init(const YAML::Node& config);

        void start();
        void stop();

        void set_level(int level);
        void set_level(std::string_view level);

        void set_root_path(std::string_view path);

        bool is_running() const {return is_running_.load();}

#ifdef VERT_ENABLE_TEST
        void test();
        void test1();
        void test2();
        void test3();
        void test4();
        void test5();
#endif
    private:
        void rotate();

        void loop_src();

        void loop_dst();

        void write(const cv::Mat &img, const MatMeta &meta, std::string_view pattern);

        void remove_folder(const std::filesystem::path &folder_path);

        void remove_file(const std::filesystem::path &file_path);

        bool level_on() const {return level_ != Level::OFF;}
        
        zmq::socket_t src_subscriber_;
        zmq::socket_t dst_subscriber_;

        std::atomic<bool> is_running_{false};

        std::thread src_thread_;
        std::thread dst_thread_;

        Level level_ = Level::OFF;

        ImageWriterConfig config_;
        ImageWriterState current_;

        std::string name_ = "ImageWriter";
        std::string src_pattern_ = "{}_{:05d}_src.";
        std::string dst_pattern_ = "{}_{:05d}_dst.";

    };

} // namespace vert


#endif /* _IMAGE_WRITER_H_ */
