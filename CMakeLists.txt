cmake_minimum_required(VERSION 3.12)
project(VisionEdgeRT VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Set Install Prefix
set(CMAKE_INSTALL_PREFIX "C:/VisionEdgeRT" CACHE PATH "Default install prefix" FORCE)

# Set Timing On/Off
option(VERT_DISABLE_TIMING "Disable Timing" OFF)
if (VERT_DISABLE_TIMING)
    message(STATUS "VisionEdgeRT: Timing is disabled")
    add_compile_definitions(VERT_DISABLE_TIMING)
endif()

# Set Debug Window On/Off
option(VERT_DEBUG_WINDOW "Enable Debug Window" OFF)
if (VERT_DEBUG_WINDOW)
    message(STATUS "VisionEdgeRT: Debug Window is Enabled")
    add_compile_definitions(VERT_DEBUG_WINDOW)
endif()

if (WIN32)
    add_compile_definitions(NOMINMAX)
endif()

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

find_package(OpenCV 3.4.16 REQUIRED COMPONENTS core imgproc imgcodecs highgui)

find_package(pylon 8.0.1 REQUIRED)
message(STATUS "Found pylon ${pylon_VERSION} in ${pylon_DIR}")

find_package(ZeroMQ CONFIG REQUIRED)
message(STATUS "Found ZeroMQ ${ZeroMQ_VERSION} in ${ZeroMQ_DIR}")

find_package(spdlog REQUIRED)
message(STATUS "Found spdlog ${spdlog_VERSION} in ${spdlog_DIR}")

find_package(yaml-cpp CONFIG REQUIRED)
message(STATUS "Found yaml-cpp ${yaml-cpp_VERSION} in ${yaml-cpp_DIR}")

# include directories for libs without INTERFACE_INCLUDE_DIRECTORIES
include_directories(${OpenCV_INCLUDE_DIRS})


add_subdirectory(nodes)
add_subdirectory(test)

# ========== Install Targets ==========
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_dlls.cmake)
install_runtime_dlls()

include(InstallRequiredSystemLibraries)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE) 

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")

set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_GENERATOR "ZIP")
include(CPack)