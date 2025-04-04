project(vert_utils)

add_library(vert_utils SHARED
    src/logging.cpp
    src/pylon_utils.cpp
    src/timer.cpp
    src/string_utils.cpp
)

target_include_directories(vert_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vert_utils PUBLIC 
    pylon::pylon
    spdlog::spdlog
    $<$<BOOL:${MINGW}>:ws2_32>
    ${OpenCV_LIBS}
)

install(TARGETS vert_utils
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)