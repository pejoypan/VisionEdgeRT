#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

#include <string>
#include <string_view>
#include <algorithm>

namespace vert
{
    inline std::string to_lower(std::string_view s) {
        std::string result(s);
        std::transform(s.begin(), s.end(), result.begin(),
            [](unsigned char c){ return std::tolower(c); });
        return result;
    }
} // namespace vert


#endif /* _STRING_UTILS_H_ */
