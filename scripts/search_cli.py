#!/usr/bin/env python3
"""
CLI интерфейс для поисковой системы
Позволяет выполнять поиск в индексированных документах
"""

import sys
import os
import argparse
from pathlib import Path
from typing import List, Dict, Any
import json

# Добавляем корневую директорию в путь
sys.path.insert(0, str(Path(__file__).parent.parent))

from src.utils.config_loader import ConfigLoader
from src.utils.logger import setup_logger
from src.utils.mongodb_client import MongoDBClient


class SearchCLI:
    """CLI для поисковой системы"""
    
    def __init__(self, config_path: str = None):
        """Инициализация CLI"""
        # Загружаем конфигурацию
        if config_path is None:
            config_path = Path(__file__).parent.parent / "config.yaml"
        
        self.config = ConfigLoader.load(str(config_path))
        self.logger = setup_logger("search_cli", self.config)
        
        # Подключаемся к MongoDB
        try:
            self.db_client = MongoDBClient(self.config)
            self.pages_collection = self.db_client.get_collection(
                self.config['mongodb']['collections']['pages']
            )
            self.logger.info("Successfully connected to MongoDB")
        except Exception as e:
            self.logger.error(f"Failed to connect to MongoDB: {e}")
            self.db_client = None
    
    def simple_search(self, query: str, limit: int = 10) -> List[Dict[str, Any]]:
        """
        Простой текстовый поиск в MongoDB
        Это временное решение до интеграции с C++ модулями
        """
        if not self.db_client:
            self.logger.error("Database connection not available")
            return []
        
        try:
            # Создаем регулярное выражение для поиска
            regex_pattern = {"$regex": query, "$options": "i"}
            
            # Ищем в title или content
            search_query = {
                "$or": [
                    {"title": regex_pattern},
                    {"content": regex_pattern}
                ]
            }
            
            results = list(self.pages_collection.find(search_query).limit(limit))
            
            self.logger.info(f"Found {len(results)} results for query: '{query}'")
            return results
            
        except Exception as e:
            self.logger.error(f"Search error: {e}")
            return []
    
    def display_results(self, results: List[Dict[str, Any]], query: str):
        """Отображение результатов поиска"""
        if not results:
            print(f"\n❌ No results found for query: '{query}'\n")
            return
        
        print(f"\n✅ Found {len(results)} results for query: '{query}'\n")
        print("=" * 80)
        
        for i, doc in enumerate(results, 1):
            print(f"\n{i}. {doc.get('title', 'No title')}")
            print(f"   URL: {doc.get('url', 'No URL')}")
            
            # Показываем краткий сниппет
            content = doc.get('content', '')
            if content:
                # Ищем первое вхождение запроса
                query_lower = query.lower()
                content_lower = content.lower()
                pos = content_lower.find(query_lower)
                
                if pos != -1:
                    # Показываем контекст вокруг найденного запроса
                    start = max(0, pos - 50)
                    end = min(len(content), pos + len(query) + 50)
                    snippet = content[start:end]
                    
                    if start > 0:
                        snippet = "..." + snippet
                    if end < len(content):
                        snippet = snippet + "..."
                    
                    print(f"   Snippet: {snippet}")
                else:
                    # Показываем начало документа
                    snippet = content[:100]
                    if len(content) > 100:
                        snippet += "..."
                    print(f"   Content: {snippet}")
            
            # Метаданные
            if 'last_crawled' in doc:
                print(f"   Crawled: {doc['last_crawled']}")
            
            print()
        
        print("=" * 80)
    
    def interactive_search(self):
        """Интерактивный режим поиска"""
        print("\n" + "=" * 80)
        print("🔍 Search Engine - Interactive Mode")
        print("=" * 80)
        print("\nCommands:")
        print("  - Enter a search query to search")
        print("  - Type 'quit' or 'exit' to exit")
        print("  - Type 'help' for help")
        print()
        
        while True:
            try:
                query = input("Search> ").strip()
                
                if not query:
                    continue
                
                if query.lower() in ['quit', 'exit']:
                    print("\n👋 Goodbye!")
                    break
                
                if query.lower() == 'help':
                    print("\n📖 Help:")
                    print("  - Simple search: just type your query")
                    print("  - Example: 'python programming'")
                    print("  - Case-insensitive search")
                    print()
                    continue
                
                # Выполняем поиск
                results = self.simple_search(query)
                self.display_results(results, query)
                
            except KeyboardInterrupt:
                print("\n\n👋 Goodbye!")
                break
            except Exception as e:
                self.logger.error(f"Error in interactive mode: {e}")
                print(f"❌ Error: {e}\n")
    
    def single_search(self, query: str, limit: int = 10):
        """Одиночный поиск"""
        results = self.simple_search(query, limit)
        self.display_results(results, query)
    
    def export_results(self, query: str, output_file: str, limit: int = 100):
        """Экспорт результатов поиска в JSON"""
        results = self.simple_search(query, limit)
        
        # Конвертируем ObjectId в строки
        for result in results:
            if '_id' in result:
                result['_id'] = str(result['_id'])
        
        try:
            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump({
                    'query': query,
                    'total_results': len(results),
                    'results': results
                }, f, ensure_ascii=False, indent=2)
            
            print(f"✅ Exported {len(results)} results to {output_file}")
            
        except Exception as e:
            self.logger.error(f"Export error: {e}")
            print(f"❌ Error exporting results: {e}")
    
    def show_stats(self):
        """Показать статистику индекса"""
        if not self.db_client:
            print("❌ Database connection not available")
            return
        
        try:
            total_docs = self.pages_collection.count_documents({})
            
            print("\n" + "=" * 80)
            print("📊 Search Engine Statistics")
            print("=" * 80)
            print(f"\nTotal documents in database: {total_docs}")
            
            # Дополнительная статистика
            if total_docs > 0:
                # Пример документа
                sample = self.pages_collection.find_one()
                if sample:
                    print("\nSample document fields:")
                    for key in sample.keys():
                        if key != '_id':
                            print(f"  - {key}")
            
            print("\n" + "=" * 80)
            
        except Exception as e:
            self.logger.error(f"Stats error: {e}")
            print(f"❌ Error getting statistics: {e}")


def main():
    """Главная функция"""
    parser = argparse.ArgumentParser(
        description='Search Engine CLI',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Interactive search
  %(prog)s --interactive
  
  # Single query
  %(prog)s --query "machine learning"
  
  # Export results
  %(prog)s --query "python" --export results.json --limit 50
  
  # Show statistics
  %(prog)s --stats
        """
    )
    
    parser.add_argument('--config', '-c', type=str,
                       help='Path to config file')
    parser.add_argument('--query', '-q', type=str,
                       help='Search query')
    parser.add_argument('--interactive', '-i', action='store_true',
                       help='Interactive search mode')
    parser.add_argument('--limit', '-l', type=int, default=10,
                       help='Maximum number of results (default: 10)')
    parser.add_argument('--export', '-e', type=str,
                       help='Export results to JSON file')
    parser.add_argument('--stats', '-s', action='store_true',
                       help='Show index statistics')
    
    args = parser.parse_args()
    
    # Создаем CLI
    try:
        cli = SearchCLI(args.config)
    except Exception as e:
        print(f"❌ Error initializing CLI: {e}")
        return 1
    
    # Выполняем команду
    try:
        if args.stats:
            cli.show_stats()
        elif args.interactive:
            cli.interactive_search()
        elif args.query:
            if args.export:
                cli.export_results(args.query, args.export, args.limit)
            else:
                cli.single_search(args.query, args.limit)
        else:
            parser.print_help()
    
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
