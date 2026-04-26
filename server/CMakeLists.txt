# iotproject/server/CMakeLists.txt

# Добавляем файлы из папки general напрямую в список исходников сервера
file(GLOB SERVER_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
file(GLOB GENERAL_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/general/*.cpp" "${CMAKE_SOURCE_DIR}/general/*.h")


qt_add_executable(IOTServer
    ${SERVER_SOURCES}
    ${GENERAL_SOURCES}
)

target_link_libraries(IOTServer PRIVATE 
    Qt6::Core
    Qt6::Mqtt
    Qt6::Sql
)

target_include_directories(IOTServer PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/general
)