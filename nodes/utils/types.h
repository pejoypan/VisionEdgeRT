#ifndef _TYPES_H_
#define _TYPES_H_
#include <tuple>
#include <cstdint>

namespace HPMVA
{
    struct GrabMeta {
        uint32_t height;
        uint32_t width;
        int pixel_type;
        uint64_t timestamp;
      
        template<class T>
        void pack(T &_pack) {
            _pack(height, width, pixel_type, timestamp);
        }
    };
} 
// namespace name


#endif /* _TYPES_H_ */
