project(basler_camera)

add_library(basler_camera SHARED
    basler_camera.cpp)
    
target_include_directories(basler_camera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # let others directly include header files

target_link_libraries(basler_camera PUBLIC
    pylon::pylon
    yaml-cpp::yaml-cpp
    libzmq
    vert_utils
)

install(TARGETS basler_camera
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)