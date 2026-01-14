# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

set(HUBBLE_SDK_PATH "../hubble-sdk")

# Project definition
project(
	hubble-sample
	VERSION 1.0
	LANGUAGES C CXX ASM
)

# Include the definition of the slc_hubble-sample target,
# which contains the content of the SLC project
include(hubble-sample.cmake)

add_executable(hubble-sample
    "${HUBBLE_SDK_PATH}/src/hubble_ble.c"
    "${HUBBLE_SDK_PATH}/src/crypto/psa.c"
    "${HUBBLE_SDK_PATH}/src/hubble.c"
    # Add additional sources here
)

include_directories(hubble-sample
    "${HUBBLE_SDK_PATH}/include"
    "${HUBBLE_SDK_PATH}/src"
    # Add additional include paths here
)

target_include_directories(hubble-sample PUBLIC
    # Add additional include paths here
)

target_compile_definitions(hubble-sample PUBLIC
    CONFIG_HUBBLE_KEY_SIZE=32
    CONFIG_HUBBLE_BLE_NETWORK_TIMER_COUNTER_DAILY
)

target_compile_options(hubble-sample PUBLIC
    # Set additional compiler flags here
)

target_link_options(hubble-sample PUBLIC
    # Set additional linker flags here
)

# Link with the content defined in the SLC project
target_link_libraries(hubble-sample PRIVATE
    slc
)

# Include managed project content if available
include(hubble-sample_project.cmake OPTIONAL RESULT_VARIABLE managed_project)
if(managed_project)
    message(STATUS "Using managed project content from ${managed_project}")
endif()

# Force the gcc linker command
set_target_properties(hubble-sample PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET hubble-sample
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:hubble-sample>" "$<TARGET_FILE_DIR:hubble-sample>/$<TARGET_FILE_BASE_NAME:hubble-sample>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:hubble-sample>" "$<TARGET_FILE_DIR:hubble-sample>/$<TARGET_FILE_BASE_NAME:hubble-sample>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:hubble-sample>" "$<TARGET_FILE_DIR:hubble-sample>/$<TARGET_FILE_BASE_NAME:hubble-sample>.bin" 
)

# Run post-build pipeline to perform additional post-processing
if(post_build_command)
add_custom_command(TARGET hubble-sample
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()