#!/usr/bin/env python3
"""
Тестовый скрипт для проверки работы поисковой системы
"""

import sys
from pathlib import Path

# Добавляем корневую директорию в путь
sys.path.insert(0, str(Path(__file__).parent.parent))

from src.utils.config_loader import ConfigLoader
from src.utils.logger import setup_logger
from src.utils.mongodb_client import MongoDBClient


def test_config():
    """Тест загрузки конфигурации"""
    print("=" * 80)
    print("1. Testing configuration loading...")
    print("=" * 80)
    
    try:
        config_path = Path(__file__).parent.parent / "config.yaml"
        config = ConfigLoader.load(str(config_path))
        
        print(f"✅ Configuration loaded successfully")
        print(f"   App name: {config['app']['name']}")
        print(f"   Version: {config['app']['version']}")
        print(f"   MongoDB database: {config['mongodb']['database']}")
        
        return True, config
    except Exception as e:
        print(f"❌ Configuration test failed: {e}")
        return False, None


def test_mongodb_connection(config):
    """Тест подключения к MongoDB"""
    print("\n" + "=" * 80)
    print("2. Testing MongoDB connection...")
    print("=" * 80)
    
    try:
        db_client = MongoDBClient(config)
        
        # Проверяем подключение
        db_client.client.admin.command('ping')
        
        print(f"✅ MongoDB connection successful")
        print(f"   Host: {config['mongodb']['host']}")
        print(f"   Port: {config['mongodb']['port']}")
        print(f"   Database: {config['mongodb']['database']}")
        
        # Проверяем коллекции
        pages_collection = db_client.get_collection(
            config['mongodb']['collections']['pages']
        )
        
        count = pages_collection.count_documents({})
        print(f"   Total documents in pages collection: {count}")
        
        return True, db_client
    except Exception as e:
        print(f"❌ MongoDB connection test failed: {e}")
        print(f"   Note: Make sure MongoDB is running and accessible")
        return False, None


def test_search(db_client, config):
    """Тест простого поиска"""
    print("\n" + "=" * 80)
    print("3. Testing simple search...")
    print("=" * 80)
    
    try:
        pages_collection = db_client.get_collection(
            config['mongodb']['collections']['pages']
        )
        
        # Проверяем, есть ли документы
        count = pages_collection.count_documents({})
        
        if count == 0:
            print(f"⚠️  No documents in database to search")
            print(f"   Run the crawler first to populate the database")
            return True
        
        # Пробуем простой поиск
        test_query = "test"
        regex_pattern = {"$regex": test_query, "$options": "i"}
        search_query = {
            "$or": [
                {"title": regex_pattern},
                {"content": regex_pattern}
            ]
        }
        
        results = list(pages_collection.find(search_query).limit(5))
        
        print(f"✅ Search test passed")
        print(f"   Test query: '{test_query}'")
        print(f"   Results found: {len(results)}")
        
        if results:
            print(f"\n   Sample result:")
            doc = results[0]
            print(f"   - Title: {doc.get('title', 'No title')}")
            print(f"   - URL: {doc.get('url', 'No URL')}")
        
        return True
    except Exception as e:
        print(f"❌ Search test failed: {e}")
        return False


def test_logger(config):
    """Тест системы логирования"""
    print("\n" + "=" * 80)
    print("4. Testing logger...")
    print("=" * 80)
    
    try:
        logger = setup_logger("test_logger", config)
        
        logger.info("Test INFO message")
        logger.warning("Test WARNING message")
        logger.debug("Test DEBUG message")
        
        print(f"✅ Logger test passed")
        print(f"   Log level: {config['logging']['level']}")
        print(f"   Log file: {config['logging']['file']['path']}")
        
        return True
    except Exception as e:
        print(f"❌ Logger test failed: {e}")
        return False


def main():
    """Главная функция"""
    print("\n" + "=" * 80)
    print("🔍 SEARCH ENGINE SYSTEM TEST")
    print("=" * 80)
    print()
    
    results = []
    
    # 1. Тест конфигурации
    success, config = test_config()
    results.append(("Configuration", success))
    
    if not success or not config:
        print("\n❌ Cannot continue without configuration")
        return 1
    
    # 2. Тест MongoDB
    success, db_client = test_mongodb_connection(config)
    results.append(("MongoDB Connection", success))
    
    # 3. Тест поиска (только если MongoDB работает)
    if success and db_client:
        success = test_search(db_client, config)
        results.append(("Search", success))
    else:
        results.append(("Search", None))  # Пропущен
    
    # 4. Тест логирования
    success = test_logger(config)
    results.append(("Logger", success))
    
    # Итоги
    print("\n" + "=" * 80)
    print("📊 TEST RESULTS SUMMARY")
    print("=" * 80)
    print()
    
    passed = sum(1 for _, result in results if result is True)
    failed = sum(1 for _, result in results if result is False)
    skipped = sum(1 for _, result in results if result is None)
    total = len(results)
    
    for name, result in results:
        if result is True:
            status = "✅ PASSED"
        elif result is False:
            status = "❌ FAILED"
        else:
            status = "⚠️  SKIPPED"
        
        print(f"{status:<12} {name}")
    
    print()
    print(f"Total tests: {total}")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")
    print(f"Skipped: {skipped}")
    print()
    
    if failed == 0:
        print("✅ All tests passed!")
        print()
        print("Next steps:")
        print("  1. Run the crawler to populate the database:")
        print("     python scripts/run_crawler.py")
        print()
        print("  2. Try the search CLI:")
        print("     python scripts/search_cli.py --interactive")
        print()
        return 0
    else:
        print("❌ Some tests failed. Please check the output above.")
        print()
        return 1


if __name__ == '__main__':
    sys.exit(main())
