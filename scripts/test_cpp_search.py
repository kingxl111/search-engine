#!/usr/bin/env python3
"""
Простой тест C++ поискового движка
"""

import subprocess
import sys
from pathlib import Path

def test_search(query: str, index_path: str = "data/indexes/index.bin"):
    """Тестирует поиск через C++ движок"""
    
    search_bin = Path("bin/search_engine")
    
    if not search_bin.exists():
        print(f"❌ Search engine not found: {search_bin}")
        return False
    
    if not Path(index_path).exists():
        print(f"❌ Index file not found: {index_path}")
        return False
    
    try:
        print(f"\n🔍 Searching for: '{query}'")
        print("=" * 60)
        
        cmd = [
            str(search_bin),
            '--index', index_path,
            '--query', query,
            '--limit', '5'
        ]
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=10
        )
        
        print("STDOUT:")
        print(result.stdout)
        
        if result.stderr:
            print("\nSTDERR:")
            print(result.stderr)
        
        if result.returncode != 0:
            print(f"\n❌ Search failed with code: {result.returncode}")
            return False
        
        print("\n✅ Search completed successfully!")
        return True
        
    except subprocess.TimeoutExpired:
        print("❌ Search timeout (>10s)")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

def main():
    """Главная функция"""
    print("\n" + "=" * 60)
    print("🔍 C++ SEARCH ENGINE TEST")
    print("=" * 60)
    
    # Проверяем наличие индекса
    index_path = "data/indexes/index.bin"
    if not Path(index_path).exists():
        print(f"❌ Index file not found: {index_path}")
        print("\nPlease build the index first:")
        print("  ./bin/index_builder --input data/processed/documents.txt --output data/indexes/index.bin")
        return 1
    
    print(f"✅ Index found: {index_path}")
    print(f"   Size: {Path(index_path).stat().st_size / 1024 / 1024:.2f} MB")
    
    # Тестируем разные запросы
    test_queries = [
        "математика",
        "информация",
        "число",
        "алгебра"
    ]
    
    results = []
    for query in test_queries:
        success = test_search(query, index_path)
        results.append((query, success))
    
    # Итоги
    print("\n" + "=" * 60)
    print("📊 TEST RESULTS")
    print("=" * 60)
    
    for query, success in results:
        status = "✅" if success else "❌"
        print(f"{status} '{query}'")
    
    passed = sum(1 for _, s in results if s)
    print(f"\nPassed: {passed}/{len(results)}")
    
    return 0 if passed == len(results) else 1

if __name__ == '__main__':
    sys.exit(main())
