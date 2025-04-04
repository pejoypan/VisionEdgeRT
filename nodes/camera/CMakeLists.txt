project(camera)

add_library(camera SHARED
    camera.cpp)

target_include_directories(camera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # let others directly include header files

target_link_libraries(camera PRIVATE 
    vert_utils
    pylon::pylon
    ${OpenCV_LIBS}
    libzmq)

install(TARGETS camera
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)