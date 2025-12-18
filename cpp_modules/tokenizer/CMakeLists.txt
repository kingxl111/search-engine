cmake_minimum_required(VERSION 3.10)

# Проект токенизатора
project(tokenizer LANGUAGES CXX)

# Исходные файлы
set(SOURCES
    src/tokenizer.cpp
)

# Заголовочные файлы
set(HEADERS
    src/tokenizer.h
)

# Создаем библиотеку
add_library(tokenizer ${SOURCES} ${HEADERS})

# Настройки компилятора
target_include_directories(tokenizer PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../data_structures
)

# Флаги компиляции
target_compile_features(tokenizer PRIVATE cxx_std_17)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tokenizer PRIVATE 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(tokenizer PRIVATE 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(tokenizer PRIVATE 
        /W4 /O2
    )
endif()

# Создаем исполняемый файл для тестирования
if(BUILD_TESTING)
    add_executable(tokenizer_test test/test_tokenizer.cpp)
    target_link_libraries(tokenizer_test tokenizer)
    
    # Тесты
    enable_testing()
    add_test(NAME tokenizer_test COMMAND tokenizer_test)
endif()

# Устанавливаем библиотеку
install(TARGETS tokenizer
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Устанавливаем заголовочные файлы
install(FILES ${HEADERS}
    DESTINATION include
)