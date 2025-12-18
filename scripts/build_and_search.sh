#!/bin/bash

# Полный цикл: компиляция -> экспорт -> индексация -> поиск

set -e

# Цвета
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

print_step() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# Переходим в корень проекта
cd "$(dirname "$0")/.."

# Шаг 1: Компиляция C++ модулей
print_step "Step 1: Compiling C++ modules"

if ! command -v cmake &> /dev/null; then
    print_error "CMake not found. Please install:"
    echo "  Mac: brew install cmake"
    echo "  Linux: sudo apt install cmake"
    exit 1
fi

if [ ! -d "build" ]; then
    mkdir build
fi

cd build
cmake ../cpp_modules -DCMAKE_BUILD_TYPE=Release

# Определяем количество ядер
if command -v nproc &> /dev/null; then
    NUM_CORES=$(nproc)
elif command -v sysctl &> /dev/null; then
    NUM_CORES=$(sysctl -n hw.ncpu)  # Mac OS
else
    NUM_CORES=4
fi

make -j"$NUM_CORES"
cd ..

# Копируем исполняемые файлы
mkdir -p bin
cp build/tokenizer/tokenizer bin/ 2>/dev/null || true
cp build/stemmer/stemmer_cli bin/ 2>/dev/null || true
cp build/boolean_index/index_builder bin/ 2>/dev/null || true
cp build/boolean_search/search_engine bin/ 2>/dev/null || true

print_success "C++ modules compiled"

# Шаг 2: Экспорт данных из MongoDB
print_step "Step 2: Exporting data from MongoDB"

python3 scripts/export_to_cpp.py \
    --output data/processed/documents.txt \
    --metadata data/processed/metadata.json \
    --limit 1000

print_success "Data exported"

# Шаг 3: Построение индекса
print_step "Step 3: Building search index"

mkdir -p data/indexes
mkdir -p reports

./bin/index_builder \
    --input data/processed/documents.txt \
    --output data/indexes/index.bin \
    --stats reports/index_stats.txt \
    --export reports/index_text.txt

print_success "Index built"

# Шаг 4: Запуск поиска
print_step "Step 4: Search Engine Ready!"

echo -e "${GREEN}All done! You can now search using:${NC}\n"
echo "1. Interactive C++ search:"
echo "   ./bin/search_engine --index data/indexes/index.bin --interactive"
echo ""
echo "2. Python wrapper (with metadata):"
echo "   python scripts/cpp_search_wrapper.py --index data/indexes/index.bin --interactive"
echo ""
echo "3. Single query:"
echo "   ./bin/search_engine --index data/indexes/index.bin --query \"your query\""
echo ""
echo "4. View statistics:"
echo "   cat reports/index_stats.txt"
echo ""

# Опционально запустить интерактивный поиск
read -p "Launch interactive search now? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    python3 scripts/cpp_search_wrapper.py \
        --index data/indexes/index.bin \
        --interactive
fi
