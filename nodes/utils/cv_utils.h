#ifndef _CV_UTILS_H_
#define _CV_UTILS_H_

#include <opencv2/core.hpp>
#include <string>

namespace vert {

    inline std::string cv_type_to_str(int type) {
        std::string r;
        
        uchar depth = type & CV_MAT_DEPTH_MASK;
        uchar chans = 1 + (type >> CV_CN_SHIFT);
        
        switch (depth) {
            case CV_8U:  r = "CV_8U"; break;
            case CV_8S:  r = "CV_8S"; break;
            case CV_16U: r = "CV_16U"; break;
            case CV_16S: r = "CV_16S"; break;
            case CV_32S: r = "CV_32S"; break;
            case CV_32F: r = "CV_32F"; break;
            case CV_64F: r = "CV_64F"; break;
            default:     r = "CV_UNKNOWN"; break;
        }
        
        r += "C" + std::to_string(chans);
        return r;
    }
    
} // namespace vert
    

#endif /* _CV_UTILS_H_ */
