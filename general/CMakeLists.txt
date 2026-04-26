# Указываем файлы ЯВНО, без GLOB
set(GEN_HEADERS DatabaseManager.h)
set(GEN_SOURCES DatabaseManager.cpp)

# Создаем библиотеку
add_library(general STATIC ${GEN_SOURCES} ${GEN_HEADERS})

# Линкуем Qt
target_link_libraries(general PUBLIC 
    Qt6::Core 
    Qt6::Sql
)

# Настройка путей (чтобы сервер видел заголовки)
target_include_directories(general PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(general INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Принудительно включаем генерацию MOC
set_target_properties(general PROPERTIES 
    AUTOMOC ON
)