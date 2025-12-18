#!/usr/bin/env python3
"""
Python обертка для C++ поискового движка
Позволяет использовать C++ search_engine из Python кода
"""

import subprocess
import json
import sys
from pathlib import Path
from typing import List, Dict, Any, Optional


class CppSearchEngine:
    """Обертка для C++ поискового движка"""
    
    def __init__(self, index_path: str, search_engine_bin: str = None):
        """
        Инициализация
        
        Args:
            index_path: Путь к файлу индекса
            search_engine_bin: Путь к исполняемому файлу search_engine
        """
        self.index_path = Path(index_path)
        
        if not self.index_path.exists():
            raise FileNotFoundError(f"Index file not found: {index_path}")
        
        # Определяем путь к бинарнику
        if search_engine_bin:
            self.search_engine_bin = Path(search_engine_bin)
        else:
            # Пытаемся найти в стандартных местах
            project_root = Path(__file__).parent.parent
            self.search_engine_bin = project_root / "bin" / "search_engine"
        
        if not self.search_engine_bin.exists():
            raise FileNotFoundError(
                f"Search engine binary not found: {self.search_engine_bin}\n"
                f"Please compile C++ modules first: ./scripts/build_cpp.sh"
            )
        
        # Загружаем метаданные если есть
        metadata_path = Path("data/processed/metadata.json")
        self.metadata = {}
        
        if metadata_path.exists():
            with open(metadata_path, 'r', encoding='utf-8') as f:
                metadata_list = json.load(f)
                self.metadata = {m['doc_id']: m for m in metadata_list}
    
    def search(self, query: str, limit: int = 10) -> List[Dict[str, Any]]:
        """
        Выполняет поиск через C++ движок
        
        Args:
            query: Поисковый запрос (поддерживает AND, OR, NOT, фразы)
            limit: Максимальное количество результатов
        
        Returns:
            Список найденных документов с метаданными
        """
        try:
            # Запускаем C++ search_engine
            cmd = [
                str(self.search_engine_bin),
                '--index', str(self.index_path),
                '--query', query,
                '--limit', str(limit)
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode != 0:
                raise RuntimeError(f"Search failed: {result.stderr}")
            
            # Парсим вывод
            # C++ движок выводит результаты в формате:
            # Found X documents (showing Y) in Z ms
            # 1. Document #ID
            #    Title: ...
            #    URL: ...
            #    Snippet: ...
            
            results = []
            current_doc = None
            
            for line in result.stdout.split('\n'):
                line = line.strip()
                
                if not line:
                    if current_doc:
                        results.append(current_doc)
                        current_doc = None
                    continue
                
                # Начало нового документа
                if line.startswith(tuple(f"{i}." for i in range(1, 1000))):
                    if current_doc:
                        results.append(current_doc)
                    
                    # Извлекаем doc_id
                    try:
                        doc_id_str = line.split('Document #')[1].split()[0]
                        doc_id = int(doc_id_str)
                        
                        current_doc = {
                            'doc_id': doc_id,
                            'rank': len(results) + 1
                        }
                        
                        # Добавляем метаданные если есть
                        if doc_id in self.metadata:
                            meta = self.metadata[doc_id]
                            current_doc.update({
                                'title': meta.get('title', ''),
                                'url': meta.get('url', ''),
                                'mongo_id': meta.get('mongo_id', '')
                            })
                    except (IndexError, ValueError):
                        pass
                
                # Парсим поля документа
                elif current_doc:
                    if line.startswith('Title:'):
                        current_doc['title'] = line.split('Title:', 1)[1].strip()
                    elif line.startswith('URL:'):
                        current_doc['url'] = line.split('URL:', 1)[1].strip()
                    elif line.startswith('Snippet:'):
                        current_doc['snippet'] = line.split('Snippet:', 1)[1].strip()
            
            # Добавляем последний документ
            if current_doc:
                results.append(current_doc)
            
            return results
            
        except subprocess.TimeoutExpired:
            raise RuntimeError("Search timeout (>30s)")
        except Exception as e:
            raise RuntimeError(f"Search error: {e}")
    
    def validate_query(self, query: str) -> bool:
        """
        Проверяет корректность синтаксиса запроса
        
        Args:
            query: Поисковый запрос
        
        Returns:
            True если запрос корректен
        """
        try:
            # Запускаем поиск с лимитом 1 для проверки
            self.search(query, limit=1)
            return True
        except RuntimeError as e:
            if 'syntax' in str(e).lower() or 'parse' in str(e).lower():
                return False
            raise
    
    def get_query_help(self) -> str:
        """Возвращает справку по синтаксису запросов"""
        return """
Синтаксис булевых запросов:

Базовые операторы:
  term              - Простой термин
  term1 && term2    - AND (оба термина должны быть)
  term1 || term2    - OR (хотя бы один термин)
  !term             - NOT (термин не должен быть)
  (query)           - Группировка скобками

Фразовый поиск:
  "exact phrase"    - Точная фраза
  "word1 word2"/5   - Слова в пределах 5 слов друг от друга

Примеры:
  python && программирование
  (машинное || глубокое) && обучение
  "поисковая система" && !google
  информационный && поиск || "text mining"
"""


class CppIndexBuilder:
    """Обертка для построения индекса"""
    
    def __init__(self, index_builder_bin: str = None):
        """Инициализация"""
        if index_builder_bin:
            self.index_builder_bin = Path(index_builder_bin)
        else:
            project_root = Path(__file__).parent.parent
            self.index_builder_bin = project_root / "bin" / "index_builder"
        
        if not self.index_builder_bin.exists():
            raise FileNotFoundError(
                f"Index builder binary not found: {self.index_builder_bin}\n"
                f"Please compile C++ modules first: ./scripts/build_cpp.sh"
            )
    
    def build_index(self, input_file: str, output_file: str, 
                    export_stats: Optional[str] = None,
                    export_text: Optional[str] = None) -> bool:
        """
        Строит индекс из файла документов
        
        Args:
            input_file: Входной файл (один документ на строку)
            output_file: Выходной файл индекса
            export_stats: Опционально, файл для статистики
            export_text: Опционально, текстовый экспорт индекса
        
        Returns:
            True если успешно
        """
        try:
            cmd = [
                str(self.index_builder_bin),
                '--input', input_file,
                '--output', output_file
            ]
            
            if export_stats:
                cmd.extend(['--stats', export_stats])
            
            if export_text:
                cmd.extend(['--export', export_text])
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300  # 5 минут
            )
            
            # Выводим результат
            print(result.stdout)
            
            if result.returncode != 0:
                print(f"Error: {result.stderr}", file=sys.stderr)
                return False
            
            return True
            
        except subprocess.TimeoutExpired:
            print("Error: Index building timeout (>5 minutes)", file=sys.stderr)
            return False
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return False


def main():
    """Пример использования"""
    import argparse
    
    parser = argparse.ArgumentParser(description='C++ Search Engine Python Wrapper')
    parser.add_argument('--index', '-i', type=str, required=True,
                       help='Path to index file')
    parser.add_argument('--query', '-q', type=str,
                       help='Search query')
    parser.add_argument('--interactive', action='store_true',
                       help='Interactive mode')
    parser.add_argument('--limit', '-l', type=int, default=10,
                       help='Max results')
    parser.add_argument('--help-query', action='store_true',
                       help='Show query syntax help')
    
    args = parser.parse_args()
    
    try:
        engine = CppSearchEngine(args.index)
        
        if args.help_query:
            print(engine.get_query_help())
            return 0
        
        if args.interactive:
            print("\n=== C++ Search Engine (Interactive Mode) ===")
            print("Enter queries (or 'quit' to exit)\n")
            
            while True:
                try:
                    query = input("Query> ").strip()
                    
                    if not query:
                        continue
                    
                    if query.lower() in ['quit', 'exit']:
                        break
                    
                    if query == 'help':
                        print(engine.get_query_help())
                        continue
                    
                    # Поиск
                    results = engine.search(query, args.limit)
                    
                    print(f"\n✅ Found {len(results)} results:\n")
                    
                    for doc in results:
                        print(f"{doc['rank']}. {doc.get('title', 'No title')}")
                        if 'url' in doc:
                            print(f"   URL: {doc['url']}")
                        if 'snippet' in doc:
                            print(f"   {doc['snippet']}")
                        print()
                    
                except KeyboardInterrupt:
                    print("\n\nGoodbye!")
                    break
                except Exception as e:
                    print(f"❌ Error: {e}\n")
        
        elif args.query:
            results = engine.search(args.query, args.limit)
            
            print(f"\n✅ Found {len(results)} results:\n")
            
            for doc in results:
                print(f"{doc['rank']}. {doc.get('title', 'No title')}")
                if 'url' in doc:
                    print(f"   URL: {doc['url']}")
                if 'snippet' in doc:
                    print(f"   {doc['snippet']}")
                print()
        else:
            parser.print_help()
        
        return 0
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1


if __name__ == '__main__':
    sys.exit(main())
