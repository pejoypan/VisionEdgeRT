#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <memory>
#include <spdlog/logger.h>

#ifdef _WIN32
    #ifdef VERT_UTILS_BUILD
        #define VERT_UTILS_API __declspec(dllexport)
    #else
        #define VERT_UTILS_API __declspec(dllimport)
    #endif
#else
    #define VERT_UTILS_API
#endif

namespace vert {
    extern VERT_UTILS_API std::shared_ptr<spdlog::logger> logger; // trace, debug, info, warn, error, critical
}

#endif
