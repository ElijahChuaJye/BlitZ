# Discover application level sources
file(GLOB_RECURSE APP_SOURCES "source/*.cpp")

file(GLOB SHADER_SOURCES 
    "shaders/*.vert"
    "shaders/*.frag"
)

# Establish the master executable application target binary 
add_executable(3DTradingPlatform ${APP_SOURCES} ${SHADER_SOURCES})

# Sets the Visual Studio debugging working directory to the folder where SPV/ and shaders/ live
set_target_properties(3DTradingPlatform PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# ==============================================================================
# LINK ENGINE MODULE
# ==============================================================================
target_link_libraries(3DTradingPlatform PRIVATE Engine)

# ==============================================================================
# AUTOMATED GLSL -> SPIR-V COMPILATION PASS
# ==============================================================================
find_program(GLSLC_EXECUTABLE glslc HINTS "ENV VULKAN_SDK" PATH_SUFFIXES "bin")

if(GLSLC_EXECUTABLE)
    # Ensure the SPV folder exists in the source tree root so the game finds it
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/SPV")
    
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME_WE) # Extracts "vert" or "frag"
        
        # Output directly to SPV/vert.spv and SPV/frag.spv in the working directory
        set(SPV_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/SPV/${SHADER_NAME}.spv")
        
        add_custom_command(
            OUTPUT ${SPV_OUTPUT}
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SPV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Automated Build: Compiling ${SHADER} -> ${SPV_OUTPUT}"
        )
        list(APPEND SPV_SHADERS ${SPV_OUTPUT})
    endforeach()
    
    add_custom_target(CompileShaders ALL DEPENDS ${SPV_SHADERS})
    set_target_properties(CompileShaders PROPERTIES FOLDER "CMakeTargets")
    add_dependencies(3DTradingPlatform CompileShaders)
else()
    message(FATAL_ERROR "Vulkan SDK 'glslc' compiler not found!")
endif()

# Force IDE structure to match the raw shaders directory layout path
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SHADER_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${APP_SOURCES})