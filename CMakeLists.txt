cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(3dgame)

file(GLOB SOURCES
    *.h
    *.cpp
)

file(GLOB_RECURSE OTHERS
    other/*
    shaders/*.vert
    shaders/*.tesc
    shaders/*.tese
    shaders/*.geom
    shaders/*.frag
    shaders/*.comp
    shaders/*.glsl
    shaders/*.h
)

include_directories(include)

add_executable(${PROJECT_NAME} ${SOURCES} ${OTHERS})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(DEBUG=1)
else()
    add_compile_definitions(NDEBUG=1)
endif()

option(EDITOR_ENABLE "Enable editor" OFF)
if(EDITOR_ENABLE)
    add_compile_definitions(EDITOR_ENABLE=1)
endif()

set(VULKAN_INCLUDE "" CACHE FILEPATH "Vulkan include path")
set(VULKAN_LIB "" CACHE FILEPATH "Vulkan library path")
set(GLSLC_PATH "" CACHE FILEPATH "Glslc spir-v compiler path")

include_directories(${VULKAN_INCLUDE})
target_link_libraries(${PROJECT_NAME} ${VULKAN_LIB})

add_custom_target(
    compile_shaders ALL
    COMMAND python ${CMAKE_SOURCE_DIR}/scripts/compile_shaders.py ${GLSLC_PATH} ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
    COMMENT "Compiling shaders..."
    )

if(UNIX)

include_directories(/usr/include/freetype2 /usr/include/libpng16 /usr/include/harfbuzz /usr/include/glib-2.0 /usr/lib/glib-2.0/include)

target_link_libraries(${PROJECT_NAME} xcb xcb-image)
target_link_libraries(${PROJECT_NAME} png)
target_link_libraries(${PROJECT_NAME} freetype)

elseif(WIN32)

include_directories(C:/lib/libpng C:/lib/freetype/freetype-2.10.2/include)

set(GLM_PATH "" CACHE FILEPATH "GLM library path")
set(ZLIB_LIB "" CACHE FILEPATH "ZLIB library path")

include_directories(${GLM_PATH})

target_link_libraries(${PROJECT_NAME} User32.dll)
target_link_libraries(${PROJECT_NAME} C:/lib/libpng/build/libpng.a)
target_link_libraries(${PROJECT_NAME} ${ZLIB_LIB})
target_link_libraries(${PROJECT_NAME} C:/lib/freetype/freetype-2.10.2/objs/freetype.a)
target_link_libraries(${PROJECT_NAME} xinput)

endif()
