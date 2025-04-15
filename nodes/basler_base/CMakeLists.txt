project(basler_base)

add_library(basler_base INTERFACE)

target_include_directories(basler_base INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # ${pylon_INCLUDE_DIRS}
    # ${ZeroMQ_INCLUDE_DIRS}
)

target_link_libraries(basler_base INTERFACE
    pylon::pylon
    libzmq
    yaml-cpp::yaml-cpp
)
