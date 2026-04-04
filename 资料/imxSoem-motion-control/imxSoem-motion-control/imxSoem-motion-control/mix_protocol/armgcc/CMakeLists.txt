# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 3.10.0)

# THE VERSION NUMBER
SET (MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION 2)
SET (MCUXPRESSO_CMAKE_FORMAT_MINOR_VERSION 0)

include(ide_overrides.cmake OPTIONAL)

if(CMAKE_SCRIPT_MODE_FILE)
  message("${MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION}")
  return()
endif()


# ENABLE ASM
ENABLE_LANGUAGE(ASM)

SET(CMAKE_STATIC_LIBRARY_PREFIX)
SET(CMAKE_STATIC_LIBRARY_SUFFIX)

SET(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
SET(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

# CURRENT DIRECTORY
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

SET(EXECUTABLE_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})
SET(LIBRARY_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})

project(rpmsg_lite_str_echo_rtos_imxcm7)

set(MCUX_BUILD_TYPES debug release ddr_debug ddr_release flash_debug flash_release)

set(MCUX_SDK_PROJECT_NAME rpmsg_lite_str_echo_rtos_imxcm7.elf)

if (NOT DEFINED SdkRootDirPath)
    SET(SdkRootDirPath ${ProjDirPath}/../../../../..)
endif()

include(${ProjDirPath}/flags.cmake)
include(${ProjDirPath}/config.cmake)

aux_source_directory(${ProjDirPath}/../bsp/can Can)
list(FILTER Can EXCLUDE REGEX ".*/fsl_flexcan\\.c$")
aux_source_directory(${ProjDirPath}/../bsp/osal Osal)
aux_source_directory(${ProjDirPath}/../bsp/oshw Oshw)
aux_source_directory(${ProjDirPath}/../bsp/soem Soem)

aux_source_directory(${ProjDirPath}/../weld_sever Weld)
aux_source_directory(${ProjDirPath}/../bsp Bsp)
aux_source_directory(${ProjDirPath}/../bsp/rpmsg Rpmsg)
aux_source_directory(${ProjDirPath}/../bsp/flash Flash)
aux_source_directory(${ProjDirPath}/../freertos Freertos)
aux_source_directory(${ProjDirPath}/../libtpr20pro Libtpr20pro)
add_executable(${MCUX_SDK_PROJECT_NAME} 
"${ProjDirPath}/../main_remote.c"
${Can}
${Osal}
${Oshw}
${Soem}
${Weld}
${Bsp}
${Rpmsg}
${Freertos}
${Flash}
${Libtpr20pro}
)


target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${ProjDirPath}/..
    ${SdkRootDirPath}/devices/MIMX8ML8/drivers
    ${ProjDirPath}/../bsp/rpmsg  
    ${ProjDirPath}/../bsp/flash  
    ${ProjDirPath}/../bsp  
    ${ProjDirPath}/../freertos 
    ${ProjDirPath}/../bsp/osal  
    ${ProjDirPath}/../bsp/oshw 
    ${ProjDirPath}/../bsp/soem  
    ${ProjDirPath}/../bsp/can 
    ${ProjDirPath}/../libtpr20pro
)

set_source_files_properties("${ProjDirPath}/../FreeRTOSConfig.h" PROPERTIES COMPONENT_CONFIG_FILE "middleware_freertos-kernel_template")

include(${SdkRootDirPath}/devices/MIMX8ML8/all_lib_device.cmake)

include_directories(
    ${ProjDirPath}/../oshw
    ${ProjDirPath}/../soem
    ${ProjDirPath}/../osal
)

IF(NOT DEFINED TARGET_LINK_SYSTEM_LIBRARIES)  
    SET(TARGET_LINK_SYSTEM_LIBRARIES "-lm -lc -lgcc -lnosys")  
ENDIF()  

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE ${TARGET_LINK_SYSTEM_LIBRARIES})

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)

ADD_CUSTOM_COMMAND(TARGET ${MCUX_SDK_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY}
-Obinary ${EXECUTABLE_OUTPUT_PATH}/${MCUX_SDK_PROJECT_NAME} ${EXECUTABLE_OUTPUT_PATH}/rpmsg_lite_str_echo_rtos.bin)

set_target_properties(${MCUX_SDK_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES "output.map;${EXECUTABLE_OUTPUT_PATH}/rpmsg_lite_str_echo_rtos.bin")

