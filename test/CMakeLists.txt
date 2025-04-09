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
