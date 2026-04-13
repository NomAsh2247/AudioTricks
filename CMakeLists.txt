# CMakeList.txt : CMake project for AudioTricks, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("AudioTricks")

# Add source to this project's executable.
add_executable (AudioTricks "AudioTricks.cpp"    "BasePortAudio.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AudioTricks PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(AudioTricks PRIVATE 
    ${CMAKE_SOURCE_DIR}/deps/fftw-3.3.5-dll64
    ${CMAKE_SOURCE_DIR}/deps/onnxruntime-win-x64-1.24.4/include
    ${CMAKE_SOURCE_DIR}/deps/portaudio-19.7.0/include
)

target_link_directories(AudioTricks PRIVATE 
    ${CMAKE_SOURCE_DIR}/deps/onnxruntime-win-x64-1.24.4/lib
    ${CMAKE_SOURCE_DIR}/deps/portaudio-19.7.0/build/msvc/x64/Release
)

target_link_libraries(AudioTricks PRIVATE
    ${CMAKE_SOURCE_DIR}/deps/fftw-3.3.5-dll64/libfftw3f-3.lib
    onnxruntime
    ${CMAKE_SOURCE_DIR}/deps/portaudio-19.7.0/build/msvc/x64/Release/portaudio_x64.lib
)

function(copy_at_runtime target)
    foreach(file IN LISTS ARGN)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${file}"
                $<TARGET_FILE_DIR:${target}>
        )
    endforeach()
endfunction()

copy_at_runtime(AudioTricks
    ${CMAKE_SOURCE_DIR}/deps/fftw-3.3.5-dll64/libfftw3f-3.dll
    ${CMAKE_SOURCE_DIR}/deps/onnxruntime-win-x64-1.24.4/lib/onnxruntime.dll
    ${CMAKE_SOURCE_DIR}/deps/portaudio-19.7.0/build/msvc/x64/Release/portaudio_x64.dll
)