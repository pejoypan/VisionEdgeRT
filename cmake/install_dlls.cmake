function(install_runtime_dlls)

    set(OpenCV_DLLS
        "${OpenCV_DIR}/x64/vc15/bin/opencv_world3416.dll"
    )

    set(PYLON_DLL_DIR "${pylon_DIR}/../../../Runtime/x64/")
    file(GLOB pylon_DLLS "${PYLON_DLL_DIR}/*Basler_pylon*.dll")
    list(APPEND pylon_DLLS 
        "${PYLON_DLL_DIR}/PylonBase_v10.dll"
        "${PYLON_DLL_DIR}/PylonUtility_v10.dll"
        "${PYLON_DLL_DIR}/PylonCamEmu_v10_TL.dll"
    )

    file(GLOB ZeroMQ_DLLS "${ZeroMQ_DIR}/../../bin/*.dll")

    file(GLOB spdlog_DLLS "${spdlog_DIR}/../../../bin/*.dll")

    file(GLOB yaml-cpp_DLLS "${yaml-cpp_DIR}/../../bin/*.dll")
    
    install(FILES
        ${OpenCV_DLLS}
        ${pylon_DLLS}
        ${ZeroMQ_DLLS}
        ${spdlog_DLLS}
        ${yaml-cpp_DLLS}
        ${CMAKE_CURRENT_SOURCE_DIR}/init.yaml
        DESTINATION bin
    )

endfunction()