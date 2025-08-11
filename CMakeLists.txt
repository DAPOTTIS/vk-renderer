cmake_minimum_required(VERSION 3.29)
project(vulkan)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# Find Vulkan package
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_program(GLSLC glslc REQUIRED)

# Gather all vertex and fragment shader source files.
file(GLOB_RECURSE VERT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert)
file(GLOB_RECURSE FRAG_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag)
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})


set(SHADER_SPV_FILES "")

# Process vertex shaders
foreach(shader ${VERT_SOURCES})
    # Create output file name by appending .spv to the shader file.
    # For example, shaders/basic.vert becomes shaders/basic.vert.spv.
    set(SPIRV_FILE "${shader}.spv")
    add_custom_command(
            OUTPUT ${SPIRV_FILE}
            COMMAND ${GLSLC} ${shader} -o ${SPIRV_FILE}
            DEPENDS ${shader}
            COMMENT "Compiling vertex shader: ${shader}"
            VERBATIM
    )
    list(APPEND SHADER_SPV_FILES ${SPIRV_FILE})
endforeach()

# Process fragment shaders
foreach(shader ${FRAG_SOURCES})
    set(SPIRV_FILE "${shader}.spv")
    add_custom_command(
            OUTPUT ${SPIRV_FILE}
            COMMAND ${GLSLC} ${shader} -o ${SPIRV_FILE}
            DEPENDS ${shader}
            COMMENT "Compiling fragment shader: ${shader}"
            VERBATIM
    )
    list(APPEND SHADER_SPV_FILES ${SPIRV_FILE})
endforeach()

# Create a custom target that depends on all compiled shader files.
add_custom_target(shaders ALL DEPENDS ${SHADER_SPV_FILES})


# Add the executable
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

# Link Vulkan libraries
target_link_libraries(vulkan PRIVATE Vulkan::Vulkan)
target_link_libraries(vulkan PRIVATE glfw)
# Include Vulkan headers
# Include directories for your project
target_include_directories(vulkan PRIVATE 
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW3_INCLUDE_DIRS}
    "libs/tinyobjloader"
    "${PROJECT_SOURCE_DIR}/src" 
)


#add_dependencies(vulkan shaders)
