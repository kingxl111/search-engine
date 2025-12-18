cmake_minimum_required(VERSION 3.10)

# Проект булева индекса
project(boolean_index LANGUAGES CXX)

# Исходные файлы
set(SOURCES
    src/inverted_index.cpp
    src/index_builder.cpp
    ../tokenizer/src/tokenizer.cpp
)

# Заголовочные файлы
set(HEADERS
    src/inverted_index.h
    src/index_builder.h
    ../tokenizer/src/tokenizer.h
)

# Создаем библиотеку
add_library(boolean_index ${SOURCES} ${HEADERS})

# Настройки компилятора
target_include_directories(boolean_index PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../data_structures
    ${CMAKE_CURRENT_SOURCE_DIR}/../tokenizer/src
)

# Флаги компиляции
target_compile_features(boolean_index PRIVATE cxx_std_17)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(boolean_index PRIVATE 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(boolean_index PRIVATE 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(boolean_index PRIVATE 
        /W4 /O2
    )
endif()

# Создаем исполняемый файл для тестирования
if(BUILD_TESTING)
    add_executable(index_builder_test test/test_index.cpp)
    target_link_libraries(index_builder_test boolean_index)
    
    # Тесты
    enable_testing()
    add_test(NAME index_builder_test COMMAND index_builder_test)
endif()

# Создаем утилиту командной строки
add_executable(index_builder src/main.cpp)
target_link_libraries(index_builder boolean_index)

# Устанавливаем
install(TARGETS boolean_index index_builder
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS}
    DESTINATION include
)