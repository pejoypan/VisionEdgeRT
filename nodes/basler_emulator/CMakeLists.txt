project(basler_emulator)

add_executable(basler_emulator 
    main.cpp
    basler_emulator.cpp)

target_link_libraries(basler_emulator PRIVATE 
    pylon::pylon
    libzmq
    ${OpenCV_LIBS})