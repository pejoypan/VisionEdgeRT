#ifndef _TYPES_H_
#define _TYPES_H_
#include <tuple>
#include <cstdint>
#include <string>

namespace vert
{
    struct GrabMeta {
        std::string device_id;
        int64_t id;
        uint32_t height;
        uint32_t width;
        int pixel_type;
        uint64_t timestamp;
        uint32_t padding_x;
        size_t buffer_size;
      
        template<class T>
        void pack(T &_pack) {
            _pack(device_id, id, height, width, pixel_type, timestamp, padding_x, buffer_size);
        }
    };

    struct MatMeta {
        std::string device_id;
        int64_t id;
        uint32_t height;
        uint32_t width;
        int cv_type;
        uint64_t timestamp;
      
        template<class T>
        void pack(T &_pack) {
            _pack(device_id, id, height, width, cv_type, timestamp);
        }
    };
} 
// namespace name


#endif /* _TYPES_H_ */
