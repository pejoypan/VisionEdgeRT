add_subdirectory(abstract_camera)
add_subdirectory(algo)
add_subdirectory(basler_camera)
add_subdirectory(basler_emulator)
add_subdirectory(image_writer)
add_subdirectory(io)

add_executable(hpmva main.cpp)

target_link_libraries(hpmva PRIVATE
    basler_emulator
    pylon::pylon
    libzmq
    ${OpenCV_LIBS})

install(TARGETS hpmva
        RUNTIME DESTINATION bin)