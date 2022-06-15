
add_executable(testing 
    include/app.hpp
    include/fbx.hpp
    include/mesh.hpp 
    include/model.hpp
    include/scene.hpp
    include/shader.hpp
    include/texture.hpp
    source/main.cpp
    source/glad.c)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Remove warnings from these files
    set_source_files_properties(source/glad.c PROPERTIES COMPILE_FLAGS "-w")
endif ()

target_compile_features(testing PRIVATE cxx_std_14)

target_include_directories(testing PRIVATE include/)
target_include_directories(testing SYSTEM PRIVATE include/stb)

target_link_libraries(testing PRIVATE animation_retargeting)

#---------------------------------------------------
# FBX SDK.

find_package(FbxSdk REQUIRED)
target_link_libraries(testing PRIVATE FbxSdk::fbx_sdk)

#---------------------------------------------------

find_package(assimp CONFIG REQUIRED)
target_link_libraries(testing PRIVATE assimp::assimp)

find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(testing PRIVATE Microsoft.GSL::GSL)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(testing PRIVATE glfw)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(testing PRIVATE fmt::fmt)

