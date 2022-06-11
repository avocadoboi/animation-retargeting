
add_executable(testing 
    include/shader.hpp
    include/mesh.hpp 
    include/model.hpp
    source/main.cpp
    source/glad.c)

target_compile_features(testing PRIVATE cxx_std_14)

target_include_directories(testing PRIVATE include/)

target_link_libraries(testing PRIVATE animation_retargeting)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(testing PRIVATE assimp::assimp)

find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(testing PRIVATE Microsoft.GSL::GSL)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(testing PRIVATE glfw)
