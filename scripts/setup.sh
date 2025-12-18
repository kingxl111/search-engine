#!/bin/bash

# Скрипт для первоначальной настройки поисковой системы

set -e  # Выход при ошибке

echo "=========================================="
echo "Search Engine Setup Script"
echo "=========================================="
echo ""

# Цвета для вывода
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Функция для вывода с цветом
print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# 1. Проверка Python
echo "1. Checking Python..."
if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version)
    print_success "Python found: $PYTHON_VERSION"
else
    print_error "Python 3 not found. Please install Python 3.8+"
    exit 1
fi

# 2. Создание виртуального окружения
echo ""
echo "2. Setting up virtual environment..."
if [ ! -d "venv" ]; then
    python3 -m venv venv
    print_success "Virtual environment created"
else
    print_warning "Virtual environment already exists"
fi

# Активация виртуального окружения
source venv/bin/activate || {
    print_error "Failed to activate virtual environment"
    exit 1
}

# 3. Обновление pip
echo ""
echo "3. Upgrading pip..."
pip install --upgrade pip setuptools wheel > /dev/null 2>&1
print_success "pip upgraded"

# 4. Установка зависимостей
echo ""
echo "4. Installing Python dependencies..."
if [ -f "requirements.txt" ]; then
    pip install -r requirements.txt
    print_success "Python dependencies installed"
else
    print_error "requirements.txt not found"
    exit 1
fi

# 5. Создание директорий
echo ""
echo "5. Creating directories..."
mkdir -p data/raw
mkdir -p data/processed
mkdir -p data/indexes
mkdir -p data/logs
mkdir -p data/stopwords
mkdir -p reports
mkdir -p bin

print_success "Directories created"

# 6. Проверка MongoDB
echo ""
echo "6. Checking MongoDB..."
if command -v mongosh &> /dev/null || command -v mongo &> /dev/null; then
    print_success "MongoDB CLI found"
    
    # Проверяем, работает ли сервер
    if mongosh --eval "db.version()" > /dev/null 2>&1 || mongo --eval "db.version()" > /dev/null 2>&1; then
        print_success "MongoDB server is running"
    else
        print_warning "MongoDB CLI found but server is not running"
        echo "   Start MongoDB with: brew services start mongodb-community"
        echo "   Or using Docker: docker run -d -p 27017:27017 --name mongodb mongo"
    fi
else
    print_warning "MongoDB not found"
    echo "   Install with: brew install mongodb-community"
    echo "   Or use Docker: docker run -d -p 27017:27017 --name mongodb mongo"
fi

# 7. Проверка CMake (опционально для C++)
echo ""
echo "7. Checking CMake (optional for C++ modules)..."
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n 1)
    print_success "CMake found: $CMAKE_VERSION"
    
    echo ""
    read -p "Do you want to build C++ modules now? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo ""
        echo "Building C++ modules..."
        chmod +x scripts/build_cpp.sh
        ./scripts/build_cpp.sh || {
            print_warning "C++ build failed (this is optional)"
        }
    fi
else
    print_warning "CMake not found (needed for C++ modules)"
    echo "   Install with: brew install cmake"
    echo "   The Python-only version will work without C++ modules"
fi

# 8. Тестирование
echo ""
echo "8. Running system tests..."
python scripts/test_system.py || {
    print_warning "Some tests failed (check MongoDB connection)"
}

# 9. Итоги
echo ""
echo "=========================================="
echo "Setup complete!"
echo "=========================================="
echo ""
echo "Next steps:"
echo ""
echo "1. Activate the virtual environment:"
echo "   source venv/bin/activate"
echo ""
echo "2. Run the crawler to collect documents:"
echo "   python scripts/run_crawler.py"
echo ""
echo "3. Try the search CLI:"
echo "   python scripts/search_cli.py --interactive"
echo ""
echo "4. Run Zipf analysis:"
echo "   python scripts/run_zipf_analysis.py"
echo ""
echo "For more information, see README_SETUP.md"
echo ""
