cmake_minimum_required(VERSION 3.22)

# set the project name
project(importer VERSION 1.0)

# TODO: should the module dependency be done here, should I download the module
# here, questions questions...

# specify the cpp standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(${PROJECT_NAME}
        ./source/importer.cpp
        ./source/animations.cpp
        ./source/fonts.cpp
				./source/main.cpp
        ./source/meshes.cpp
        ./source/skinned_meshes.cpp
        ./source/sublevels.cpp
        ./source/textures.cpp
        ./source/topology/brush.cpp
        ./source/topology/point.cpp
        ./source/topology/poly_brush.cpp
        ./source/topology/polygon.cpp
				)

target_link_libraries(${PROJECT_NAME}
        PUBLIC assimp
        PUBLIC math
        PUBLIC library
        PUBLIC entity
        PUBLIC collision
        PUBLIC mesh
        PUBLIC material
        PUBLIC texture
        PUBLIC font
        PUBLIC spatial
        PUBLIC loaders
        PUBLIC level
        PUBLIC props
        PUBLIC skeleton
        PUBLIC animation
				)

target_include_directories(${PROJECT_NAME} PUBLIC
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)