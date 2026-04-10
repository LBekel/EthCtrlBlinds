cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME ETHCTRLBLINDS)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/dio.c
    Core/Src/web.c
    Core/Src/eeprom.c
    Core/Src/jumpcode.c
    Core/Src/mqtt_client.c
    LWIP/Target/md5.c
    Core/Src/PHY_KSZ8081RNA.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Prefer toolchain-provided objcopy and fall back to common names.
set(OBJCOPY_TOOL ${CMAKE_OBJCOPY})
if(NOT OBJCOPY_TOOL)
    find_program(OBJCOPY_TOOL NAMES arm-none-eabi-objcopy objcopy)
endif()

if(NOT OBJCOPY_TOOL)
    message(FATAL_ERROR "objcopy tool not found. Set CMAKE_OBJCOPY or add arm-none-eabi-objcopy to PATH.")
endif()

set(HEX_FILE ${CMAKE_PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_PROJECT_NAME}.bin)
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${OBJCOPY_TOOL} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${HEX_FILE}
        COMMAND ${OBJCOPY_TOOL} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${BIN_FILE}
        COMMENT "Generating ${HEX_FILE} and ${BIN_FILE}"
        VERBATIM
)

set_property(TARGET ${CMAKE_PROJECT_NAME} APPEND PROPERTY
        ADDITIONAL_CLEAN_FILES ${HEX_FILE} ${BIN_FILE}
)
