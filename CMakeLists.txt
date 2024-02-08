cmake_minimum_required(VERSION 3.2)

# set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(3dgame)

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

include_directories(${CMAKE_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SOURCES} ${OTHERS})

target_compile_options(${PROJECT_NAME} PRIVATE -std=c++2b)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(DEBUG=1)
else()
    add_compile_definitions(NDEBUG=1)
endif()

#vulkan
set(VULKAN_INCLUDE "" CACHE PATH "Vulkan include path")
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

set(GLM_PATH "" CACHE FILEPATH "glm library path")
set(ZLIB_LIB "" CACHE FILEPATH "zlib library path")
set(LIBPNG_INCLUDE "" CACHE PATH "libpng include dir")
set(LIBPNG_LIB "" CACHE FILEPATH "libpng library path")
set(FREETYPE_INCLUDE "" CACHE PATH "freetype include dir")
set(FREETYPE_LIB "" CACHE FILEPATH "freetype library path")

include_directories(${GLM_PATH} ${LIBPNG_INCLUDE} ${FREETYPE_INCLUDE})

target_link_libraries(${PROJECT_NAME} User32.dll)
target_link_libraries(${PROJECT_NAME} ${LIBPNG_LIB})
target_link_libraries(${PROJECT_NAME} ${ZLIB_LIB})
target_link_libraries(${PROJECT_NAME} ${FREETYPE_LIB})
#target_link_libraries(${PROJECT_NAME} xinput)

set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ ${CMAKE_CXX_STANDARD_LIBRARIES}")
set(CMAKE_EXE_LINKER_FLAGS "-mwindows ${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")

endif()
