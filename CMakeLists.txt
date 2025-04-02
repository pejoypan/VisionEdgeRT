cmake_minimum_required(VERSION 3.12)
project(HPMVA VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_INSTALL_PREFIX "C:/hpmva" CACHE PATH "Default install prefix" FORCE)

option(HPMVA_DISABLE_TIMING "Disable Timing" OFF)
if (HPMVA_DISABLE_TIMING)
    message(STATUS "HPMVA: Timing is disabled")
    add_compile_definitions(HPMVA_DISABLE_TIMING)
endif()

if (WIN32)
    add_compile_definitions(NOMINMAX)
endif()

find_package(OpenCV 3.4.16 REQUIRED COMPONENTS core imgproc imgcodecs highgui)

find_package(pylon 8.0.1 REQUIRED)
message(STATUS "Found pylon ${pylon_VERSION} in ${pylon_DIR}")

find_package(ZeroMQ CONFIG REQUIRED)
message(STATUS "Found ZeroMQ ${ZeroMQ_VERSION} in ${ZeroMQ_DIR}")

# include directories for libs without INTERFACE_INCLUDE_DIRECTORIES
include_directories(${OpenCV_INCLUDE_DIRS})


add_subdirectory(nodes)
add_subdirectory(test)
