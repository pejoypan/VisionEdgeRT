add_subdirectory(abstract_camera)
add_subdirectory(algo)
add_subdirectory(basler_camera)
add_subdirectory(basler_emulator)
add_subdirectory(image_writer)
add_subdirectory(io)

add_executable(VERT main.cpp)

target_link_libraries(VERT PRIVATE
    basler_emulator
    pylon::pylon
    libzmq
    ${OpenCV_LIBS})

install(TARGETS VERT
        RUNTIME DESTINATION bin)