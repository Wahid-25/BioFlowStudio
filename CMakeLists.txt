cmake_minimum_required(VERSION 3.20)

project(BioFlowStudio VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Charts
)

qt_standard_project_setup()

qt_add_executable(BioFlowStudio
    src/main.cpp
    src/core/Project.cpp
    src/core/Dataset.cpp
)

target_link_libraries(BioFlowStudio PRIVATE
    Qt6::Widgets
    Qt6::Charts
)