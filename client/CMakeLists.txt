file(GLOB CLIENT_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
file(GLOB GENERAL_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/general/*.cpp" "${CMAKE_SOURCE_DIR}/general/*.h")

qt_add_executable(IOTClient
    ${CLIENT_SOURCES}
    ${GENERAL_SOURCES}
)

target_include_directories(IOTClient PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/general
)


target_link_libraries(IOTClient PRIVATE
    general
    Qt6::Widgets 
    Qt6::Mqtt 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Qt6::Sql
    Qt6::Charts
)


if (MSVC)     
    target_compile_options(IOTClient PRIVATE
        /W3
        /MP
        /WX
        /ZI
    )
else() 
    #todo
endif()

# Деплой Qt-зависимостей после сборки клиента
if (WIN32)    
    add_custom_command(TARGET IOTClient POST_BUILD
        COMMAND "${Qt6_BIN_DIR}/windeployqt.exe" 
                "$<TARGET_FILE:IOTClient>"
        COMMENT "Deploying Qt DLLs with windeployqt..."
    )
endif()