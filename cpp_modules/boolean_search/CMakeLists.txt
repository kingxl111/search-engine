cmake_minimum_required(VERSION 3.10)

# Проект булева поиска
project(boolean_search LANGUAGES CXX)

# Исходные файлы
set(SOURCES
    src/query_parser.cpp
    src/query_evaluator.cpp
    src/boolean_search.cpp
    ../tokenizer/src/tokenizer.cpp
    ../boolean_index/src/inverted_index.cpp
    ../boolean_index/src/index_builder.cpp
)

# Заголовочные файлы
set(HEADERS
    src/query_parser.h
    src/query_evaluator.h
    src/boolean_search.h
    ../tokenizer/src/tokenizer.h
    ../boolean_index/src/inverted_index.h
    ../boolean_index/src/index_builder.h
)

# Создаем библиотеку
add_library(boolean_search ${SOURCES} ${HEADERS})

# Настройки компилятора
target_include_directories(boolean_search PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../data_stuctures
    ${CMAKE_CURRENT_SOURCE_DIR}/../tokenizer/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../boolean_index/src
)

# Флаги компиляции
target_compile_features(boolean_search PRIVATE cxx_std_17)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(boolean_search PRIVATE 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(boolean_search PRIVATE 
        -Wall -Wextra -Wpedantic -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(boolean_search PRIVATE 
        /W4 /O2
    )
endif()

# Создаем исполняемый файл для тестирования
if(BUILD_TESTING)
    add_executable(search_test test/test_search.cpp)
    target_link_libraries(search_test boolean_search)
    
    # Тесты
    enable_testing()
    add_test(NAME search_test COMMAND search_test)
endif()

# Создаем утилиту командной строки
add_executable(search_engine src/main.cpp)
target_link_libraries(search_engine boolean_search)

# Устанавливаем
install(TARGETS boolean_search search_engine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS}
    DESTINATION include
)