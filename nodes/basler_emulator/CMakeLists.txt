project(basler_emulator)

add_library(basler_emulator SHARED
    basler_emulator.cpp)

target_include_directories(basler_emulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # let others directly include header files

target_link_libraries(basler_emulator PUBLIC 
    pylon::pylon
    libzmq)

install(TARGETS basler_emulator
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)