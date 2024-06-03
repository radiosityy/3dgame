cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(3dgame)

find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)
find_package(PNG REQUIRED)

set(GLSLC_PATH "" CACHE FILEPATH "Glslc spir-v compiler path")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(DEBUG=1)
else()
    add_compile_definitions(NDEBUG=1)
endif()

option(EDITOR_ENABLE "Enable editor" OFF)
if(EDITOR_ENABLE)
    add_compile_definitions(EDITOR_ENABLE=1)
endif()

option(VULKAN_VALIDATION_ENABLE "Enable Vulkan validation" OFF)
if(VULKAN_VALIDATION_ENABLE)
    add_compile_definitions(VULKAN_VALIDATION_ENABLE=1)
endif()

file(GLOB SOURCES *.h *.cpp)

if(EDITOR_ENABLE)
file(GLOB EDITOR_SOURCES
    editor/*.h
    editor/*.cpp
)
set(SOURCES ${SOURCES} ${EDITOR_SOURCES})
endif()

file(GLOB SHADER_SOURCES shaders/*.h)

add_executable(${PROJECT_NAME} ${SOURCES} ${SHADER_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan ZLIB::ZLIB PNG::PNG Freetype::Freetype)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)

add_custom_target(
    compile_shaders ALL
    COMMAND python ${CMAKE_SOURCE_DIR}/scripts/compile_shaders.py ${GLSLC_PATH} ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
    COMMENT "Compiling shaders..."
    )

if(UNIX)

target_link_libraries(${PROJECT_NAME} PRIVATE xcb xcb-image)

elseif(WIN32)

target_link_libraries(${PROJECT_NAME} PRIVATE stdc++exp)
target_link_libraries(${PROJECT_NAME} PRIVATE User32.dll)
#target_link_libraries(${PROJECT_NAME} xinput)

set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ ${CMAKE_CXX_STANDARD_LIBRARIES}")
set(CMAKE_EXE_LINKER_FLAGS "-mwindows ${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")

endif()
