cmake_minimum_required(VERSION 3.20)
project(Dempt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Collect all sources
file(GLOB_RECURSE SOURCES
    "Source/**/*.cpp"
    "Source/**/*.hpp"      # ← Add this for .hpp files
)

# AMD sources + headers
if(EXISTS "${CMAKE_SOURCE_DIR}/AMD")
    file(GLOB_RECURSE AMD_SOURCES "AMD/*.cpp" "AMD/*.hpp")
    list(APPEND SOURCES ${AMD_SOURCES})
elseif(EXISTS "${CMAKE_SOURCE_DIR}/Includes/AMD")
    file(GLOB_RECURSE AMD_SOURCES "Includes/AMD/*.cpp" "Includes/AMD/*.hpp")
    list(APPEND SOURCES ${AMD_SOURCES})
endif()

add_executable(Dempt ${SOURCES})

# Include directories - this is the key part
target_include_directories(Dempt PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Includes
    ${CMAKE_SOURCE_DIR}/Includes/AMD   # helps with both "Utility.hpp" and "AMD/Utility.hpp"
)

if(WIN32)
    target_link_libraries(Dempt PRIVATE
        d3d11 dxgi d3dcompiler dwmapi gdi32 user32 ole32 shell32 uuid wbemuuid netapi32 shlwapi
    )
    
    target_compile_definitions(Dempt PRIVATE
        UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
    )

    # Copy DLLs
    set(dlls
        "C:/msys64/ucrt64/bin/libstdc++-6.dll"
        "C:/msys64/ucrt64/bin/libgcc_s_seh-1.dll"
        "C:/msys64/ucrt64/bin/libwinpthread-1.dll"
    )
    foreach(d ${dlls})
        add_custom_command(TARGET Dempt POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${d} $<TARGET_FILE_DIR:Dempt>
        )
    endforeach()
endif()