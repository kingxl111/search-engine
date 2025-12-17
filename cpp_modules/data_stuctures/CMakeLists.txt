cmake_minimum_required(VERSION 3.10)

# Проект структур данных
project(data_structures LANGUAGES CXX)

# Исходные файлы
set(SOURCES
    vector.cpp
    string.cpp
    hash_table.cpp
    avl_tree.cpp
    bit_vector.cpp
)

# Заголовочные файлы
set(HEADERS
    vector.h
    string.h
    hash_table.h
    avl_tree.h
    bit_vector.h
)

# Создаем библиотеку
add_library(data_structures ${SOURCES} ${HEADERS})

# Настройки компилятора
target_include_directories(data_structures PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Флаги компиляции
target_compile_features(data_structures PUBLIC cxx_std_17)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(data_structures PUBLIC 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(data_structures PUBLIC 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(data_structures PUBLIC 
        /W4 /O2
    )
endif()

# Устанавливаем
install(TARGETS data_structures
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS}
    DESTINATION include
)