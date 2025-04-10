project(camera_adapter)

add_library(camera_adapter SHARED
    camera_adapter.cpp)

target_include_directories(camera_adapter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # let others directly include header files

target_link_libraries(camera_adapter PUBLIC 
    pylon::pylon
    ${OpenCV_LIBS}
    libzmq
    vert_utils
)

install(TARGETS camera_adapter
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)