project(test_image_writer)

add_executable(test_image_writer
    test_image_writer.cpp
)
target_link_libraries(test_image_writer PRIVATE
    image_writer
)

install(TARGETS test_image_writer
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)

project(test_pub_to_ui)

add_executable(test_pub_to_ui
    test_pub_to_ui.cpp
)


target_link_libraries(test_pub_to_ui PRIVATE
    libzmq
    ${OpenCV_LIBS}
    vert_utils  
)

install(TARGETS test_pub_to_ui
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)