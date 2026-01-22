cmake_minimum_required(VERSION 3.25)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" 
    CACHE STRING "Vcpkg toolchain file")

project(IOTProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(Qt6_DIR "C:/Qt/6.9.1/msvc2022_64/lib/cmake/Qt6")
set(Qt6_BIN_DIR "C:/Qt/6.9.1/msvc2022_64/bin")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Mqtt)
find_package(OpenSSL REQUIRED)

qt_standard_project_setup()

qt_add_executable(IOTProject
    "main.cpp"
    "MQTTConnectionManager.cpp" "MQTTConnectionManager.h"
    "MainWindow.cpp" "MainWindow.h"
    "TreeItem.cpp" "TreeItem.h"
    "TreeModel.cpp" "TreeModel.h"
)

target_link_libraries(IOTProject PRIVATE Qt6::Widgets)
target_link_libraries(IOTProject PRIVATE Qt6::Mqtt)
target_link_libraries(IOTProject PRIVATE OpenSSL::SSL)
target_link_libraries(IOTProject PRIVATE OpenSSL::Crypto)

if (MSVC) 
    target_compile_options(IOTProject PRIVATE
        /W3
        /MP
        /WX
        /ZI
    )
else() 
#todo
endif()


if (WIN32)

    add_custom_command(TARGET IOTProject POST_BUILD
        COMMAND "${Qt6_BIN_DIR}/windeployqt.exe" 
                "$<TARGET_FILE:IOTProject>"

        COMMENT "Deploying Qt DLLs with windeployqt..."

    )

endif()