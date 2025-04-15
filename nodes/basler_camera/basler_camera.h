#ifndef _BASLER_CAMERA_H_
#define _BASLER_CAMERA_H_

#include "../basler_base/basler_base.h"

namespace vert {

    class BaslerCamera : public BaslerBase
    {
    public:
        BaslerCamera(zmq::context_t *ctx);
        ~BaslerCamera();
        
        void start();
        
    private:
        bool device_specific_init(const YAML::Node& config) override;

    };

} // namespace vert

#endif /* _BASLER_CAMERA_H_ */
