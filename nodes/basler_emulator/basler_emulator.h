#ifndef _BASLER_EMULATOR_H_
#define _BASLER_EMULATOR_H_
#include <string>
#include <string_view>
#include "../basler_base/basler_base.h"

namespace vert {

    class BaslerEmulator : public BaslerBase
    {
        
        struct BaslerEmulatorConfig {
            std::string file_path;
            std::string pixel_format = "BGR8Packed";
            int max_images = -1;
            double fps = 30.0;
        };

    public:
        BaslerEmulator(zmq::context_t *ctx);
        ~BaslerEmulator();

        void start();

        bool set_image_filename(std::string_view filename);
        bool set_fps(double fps);
        bool set_pixel_format(std::string_view format);

        int get_pixel_format() const;

    private:
        bool device_specific_init(const YAML::Node& config) override;

        BaslerEmulatorConfig cfg_;

    };

} // namespace vert

    
#endif /* _BASLER_EMULATOR_H_ */
