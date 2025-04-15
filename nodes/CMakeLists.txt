add_subdirectory(utils)
add_subdirectory(camera_adapter)
add_subdirectory(algo)
add_subdirectory(basler_camera)
add_subdirectory(basler_emulator)
add_subdirectory(image_writer)
add_subdirectory(io)

add_executable(VERT main.cpp)

target_link_libraries(VERT PRIVATE
    pylon::pylon
    libzmq
    spdlog::spdlog
    $<$<BOOL:${MINGW}>:ws2_32>
    yaml-cpp::yaml-cpp
    vert_utils  
    basler_camera
    basler_emulator
    camera_adapter
    image_writer
)

install(TARGETS VERT
        RUNTIME DESTINATION bin)