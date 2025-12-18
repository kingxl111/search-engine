cmake_minimum_required(VERSION 3.10)

# Проект структур данных (header-only)
project(data_structures LANGUAGES CXX)

# Заголовочные файлы
set(HEADERS
    vector.h
    string.h
    hash_table.h
    avl_tree.h
    bit_vector.h
)

# Создаем header-only библиотеку
add_library(data_structures INTERFACE)

# Настройки компилятора
target_include_directories(data_structures INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Требуем C++17
target_compile_features(data_structures INTERFACE cxx_std_17)

# Устанавливаем
install(TARGETS data_structures
    EXPORT data_structures-targets
)

install(FILES ${HEADERS}
    DESTINATION include/data_structures
)
