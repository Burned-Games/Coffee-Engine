cmake_minimum_required(VERSION 3.15)
project(Wwise VERSION 0.1.0 LANGUAGES C CXX)

# Ensure WWISESDK environment variable is set
if (NOT DEFINED ENV{WWISESDK})
    message(FATAL_ERROR "WWISESDK environment variable is not set. Please set it to the Wwise SDK path.")
endif()

# Convert the WWISESDK environment variable to a CMake path
file(TO_CMAKE_PATH "$ENV{WWISESDK}" WWISE_SDK_DIR)

# Verify the path exists
if (NOT EXISTS "${WWISE_SDK_DIR}")
    message(FATAL_ERROR "The specified WWISE_SDK_DIR does not exist: ${WWISE_SDK_DIR}")
endif()

set(SAMPLES_DIR "${WWISE_SDK_DIR}/samples")

# Determine platform-specific variables
if (WIN32)
    set(PLATFORM_DIR "Win32")
    set(WWISE_LIB_DIR_DEBUG "${WWISE_SDK_DIR}/x64_vc170/Debug/lib")
    set(WWISE_LIB_DIR_RELEASE "${WWISE_SDK_DIR}/x64_vc170/Release/lib")
    set(PLATFORM_LIBS ws2_32.dll)
elseif(UNIX)
    set(PLATFORM_DIR "POSIX")
    set(WWISE_LIB_DIR_DEBUG "${WWISE_SDK_DIR}/Linux_x64/Debug/lib")
    set(WWISE_LIB_DIR_RELEASE "${WWISE_SDK_DIR}/Linux_x64/Release/lib")
    set(PLATFORM_LIBS "")
endif()

# Gather sources
file(GLOB_RECURSE SOURCES
    "${SAMPLES_DIR}/SoundEngine/Common/*.h"
    "${SAMPLES_DIR}/SoundEngine/Common/*.cpp"
    "${SAMPLES_DIR}/SoundEngine/${PLATFORM_DIR}/*.h"
    "${SAMPLES_DIR}/SoundEngine/${PLATFORM_DIR}/*.cpp"
)

# Add a library target for Wwise
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Include Wwise headers
target_include_directories(${PROJECT_NAME}
    PUBLIC "${WWISE_SDK_DIR}/include"
           "${SAMPLES_DIR}/SoundEngine/Common"
           "${SAMPLES_DIR}/SoundEngine/${PLATFORM_DIR}"
)

# Link necessary libraries
file(GLOB LIB_FILES_DEBUG ${WWISE_LIB_DIR_DEBUG}/*${CMAKE_STATIC_LIBRARY_SUFFIX})
file(GLOB LIB_FILES_RELEASE ${WWISE_LIB_DIR_RELEASE}/*${CMAKE_STATIC_LIBRARY_SUFFIX})

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        $<$<CONFIG:Debug>:${LIB_FILES_DEBUG} ${PLATFORM_LIBS}>
        $<$<CONFIG:Release>:${LIB_FILES_RELEASE} ${PLATFORM_LIBS}>
        $<$<CONFIG:RelWithDebInfo>:${LIB_FILES_RELEASE} ${PLATFORM_LIBS}>
)

# Define AK_OPTIMIZED for release builds
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        $<$<CONFIG:Release>:AK_OPTIMIZED>
        $<$<CONFIG:RelWithDebInfo>:AK_OPTIMIZED>
)