project(image_writer)

add_library(image_writer SHARED
image_writer.cpp)

target_include_directories(image_writer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # let others directly include header files

target_link_libraries(image_writer PRIVATE 
    ${OpenCV_LIBS}
    libzmq
    vert_utils
)

install(TARGETS image_writer
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)