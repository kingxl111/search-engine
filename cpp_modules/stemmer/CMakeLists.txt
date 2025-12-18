cmake_minimum_required(VERSION 3.10)

# Проект стеммера
project(stemmer LANGUAGES CXX)

# Исходные файлы
set(SOURCES
    src/stemmer.cpp
)

# Заголовочные файлы
set(HEADERS
    src/stemmer.h
)

# Создаем библиотеку
add_library(stemmer ${SOURCES} ${HEADERS})

# Настройки компилятора
target_include_directories(stemmer PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../data_stuctures
)

# Флаги компиляции
target_compile_features(stemmer PRIVATE cxx_std_17)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(stemmer PRIVATE 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(stemmer PRIVATE 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(stemmer PRIVATE 
        /W4 /O2
    )
endif()

# Создаем исполняемый файл для тестирования
if(BUILD_TESTING)
    add_executable(stemmer_test test/test_stemmer.cpp)
    target_link_libraries(stemmer_test stemmer)
    
    # Тесты
    enable_testing()
    add_test(NAME stemmer_test COMMAND stemmer_test)
endif()

# Создаем утилиту командной строки
add_executable(stemmer_cli src/main.cpp)
target_link_libraries(stemmer_cli stemmer)

# Устанавливаем
install(TARGETS stemmer stemmer_cli
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS}
    DESTINATION include
)