project(image_processor)

add_library(image_processor SHARED
    image_processor.cpp)

target_include_directories(image_processor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # let others directly include header files

target_link_libraries(image_processor PUBLIC
    ${OpenCV_LIBS}
    libzmq
    yaml-cpp::yaml-cpp
    vert_utils
)

install(TARGETS image_processor
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)