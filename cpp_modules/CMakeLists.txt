cmake_minimum_required(VERSION 3.10)
project(SearchEngine VERSION 1.0.0 LANGUAGES CXX)

# Настройки компилятора
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Флаги компиляции
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O2")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DDEBUG")
    endif()
endif()

# Включаем поддиректории (note: data_stuctures is the actual directory name)
add_subdirectory(data_stuctures)
add_subdirectory(tokenizer)
add_subdirectory(stemmer)
add_subdirectory(boolean_index)
add_subdirectory(boolean_search)

# Тесты
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
endif()

# Инсталляция
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin
        DESTINATION .
        USE_SOURCE_PERMISSIONS)