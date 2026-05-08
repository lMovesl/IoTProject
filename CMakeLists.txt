cmake_minimum_required(VERSION 3.25)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" 
    CACHE STRING "Vcpkg toolchain file")

project(IOTProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(Qt6_DIR "C:/Qt/6.9.1/msvc2022_64/lib/cmake/Qt6")
set(Qt6_BIN_DIR "C:/Qt/6.9.1/msvc2022_64/bin")

find_package(Qt6 REQUIRED COMPONENTS Widgets Mqtt Sql Charts SVG)
find_package(OpenSSL REQUIRED)

qt_standard_project_setup()

message(STATUS ">>> Adding general subdirectory...")
add_subdirectory(${CMAKE_SOURCE_DIR}/general)
message(STATUS ">>> General subdirectory added successfully!")
add_subdirectory(client)
add_subdirectory(server)