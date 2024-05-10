cmake_minimum_required(VERSION 3.28)
project(vulkan_snake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

find_package(Vulkan COMPONENTS glslangValidator REQUIRED)

set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)

add_executable(vulkan_snake
        main.cpp
        renderer.cpp
        renderer.h
        snakegame.cpp
        snakegame.h)

target_link_libraries(vulkan_snake PRIVATE glfw glm)
target_include_directories(vulkan_snake PRIVATE ${Vulkan_INCLUDE_DIRS})
target_precompile_headers(vulkan_snake PRIVATE <vulkan/vulkan.hpp>)
target_compile_definitions(vulkan_snake PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

if (MSVC)
    set_target_properties(vulkan_snake PROPERTIES
            WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
            LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif ()

set(SHADERS
    shaders/basic.vert
    shaders/basic.frag)

foreach (SRC IN LISTS SHADERS)
    cmake_path(ABSOLUTE_PATH SRC)
    cmake_path(GET SRC FILENAME SRC_NAME)
    set(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}.h)
    string(MAKE_C_IDENTIFIER ${SRC_NAME} IDENT)

    add_custom_command(
            OUTPUT ${OUTPUT}
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V --vn ${IDENT} -o ${OUTPUT} ${SRC}
            COMMENT "Compiling SPIR-V Shader ${SRC_NAME}"
            DEPENDS ${SRC}
    )

    target_sources(vulkan_snake PRIVATE ${OUTPUT})
endforeach ()

target_include_directories(vulkan_snake PRIVATE ${CMAKE_CURRENT_BINARY_DIR})