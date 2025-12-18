#!/bin/bash

# Скрипт для сборки всех C++ модулей поисковой системы

set -e  # Выйти при первой ошибке

echo "==========================================="
echo "Building Search Engine C++ modules"
echo "==========================================="

# Проверяем наличие компилятора
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ compiler not found. Please install g++"
    exit 1
fi

# Создаем директорию для сборки
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Генерируем Makefile с помощью CMake
echo "Generating build files..."
cmake ../cpp_modules -DCMAKE_BUILD_TYPE=Release

# Определяем количество ядер для параллельной сборки
if command -v nproc &> /dev/null; then
    NUM_CORES=$(nproc)
elif command -v sysctl &> /dev/null; then
    NUM_CORES=$(sysctl -n hw.ncpu)  # Mac OS
else
    NUM_CORES=4
fi

echo "Building with $NUM_CORES cores..."

# Собираем проект
make -j"$NUM_CORES"

echo "==========================================="
echo "Build completed successfully!"
echo "==========================================="

# Копируем исполняемые файлы в нужные директории
echo "Copying executables..."

# Создаем директории для бинарников
mkdir -p ../bin
mkdir -p ../data/indexes

# Копируем исполняемые файлы
cp tokenizer/tokenizer ../bin/
cp stemmer/stemmer ../bin/
cp boolean_index/index_builder ../bin/
cp boolean_search/search_engine ../bin/

echo "Executables copied to ./bin directory"

# Проверяем наличие исполняемых файлов
echo ""
echo "Checking executables:"
for exec in tokenizer stemmer index_builder search_engine; do
    if [ -f "../bin/$exec" ]; then
        echo "✓ $exec - OK"
    else
        echo "✗ $exec - NOT FOUND"
    fi
done

echo ""
echo "To run the tokenizer: ./bin/tokenizer --help"
echo "To run the search engine: ./bin/search_engine --help"