#!/usr/bin/env python3
"""
Простой Python-only булев поиск для тестирования
Работает напрямую с MongoDB без C++ модулей
"""

import sys
import re
from pathlib import Path
from typing import List, Dict, Any, Set

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.utils.config_loader import ConfigLoader
from src.utils.logger import setup_logger
from src.utils.mongodb_client import MongoDBClient


class SimpleBooleanSearch:
    """Простой булев поиск на Python"""
    
    def __init__(self, config_path: str):
        """Инициализация"""
        self.config = ConfigLoader.load_config(config_path)
        self.logger = setup_logger("simple_search", self.config)
        
        # Подключаемся к MongoDB
        self.db_client = MongoDBClient(config_path)
        self.pages_collection = self.db_client.db[
            self.config['mongodb']['collections']['pages']
        ]
    
    def tokenize(self, text: str) -> Set[str]:
        """Простая токенизация"""
        # Приводим к нижнему регистру и разбиваем по пробелам/пунктуации
        text = text.lower()
        tokens = re.findall(r'\b\w+\b', text)
        # Фильтруем короткие слова
        return set(t for t in tokens if len(t) >= 2)
    
    def parse_query(self, query: str) -> Dict[str, Any]:
        """
        Парсит простой булев запрос
        Поддерживает: term1 && term2, term1 || term2, !term
        """
        query = query.strip().lower()
        
        # Простой парсинг для AND, OR, NOT
        if '&&' in query:
            parts = [p.strip() for p in query.split('&&')]
            return {'type': 'AND', 'terms': parts}
        elif '||' in query:
            parts = [p.strip() for p in query.split('||')]
            return {'type': 'OR', 'terms': parts}
        elif query.startswith('!'):
            term = query[1:].strip()
            return {'type': 'NOT', 'term': term}
        else:
            # Простой термин или несколько терминов (неявный AND)
            terms = query.split()
            if len(terms) == 1:
                return {'type': 'TERM', 'term': terms[0]}
            else:
                return {'type': 'AND', 'terms': terms}
    
    def search(self, query: str, limit: int = 10) -> List[Dict[str, Any]]:
        """Выполняет поиск"""
        parsed = self.parse_query(query)
        
        if parsed['type'] == 'TERM':
            # Простой поиск одного термина
            term = parsed['term']
            results = self._search_term(term, limit)
        
        elif parsed['type'] == 'AND':
            # AND поиск - все термины должны быть
            results = self._search_and(parsed['terms'], limit)
        
        elif parsed['type'] == 'OR':
            # OR поиск - хотя бы один термин
            results = self._search_or(parsed['terms'], limit)
        
        elif parsed['type'] == 'NOT':
            # NOT поиск - термина не должно быть
            results = self._search_not(parsed['term'], limit)
        
        else:
            results = []
        
        return results
    
    def _search_term(self, term: str, limit: int) -> List[Dict[str, Any]]:
        """Поиск одного термина"""
        regex = {"$regex": term, "$options": "i"}
        query = {
            "$or": [
                {"title": regex},
                {"content": regex}
            ]
        }
        
        results = list(self.pages_collection.find(query).limit(limit))
        return results
    
    def _search_and(self, terms: List[str], limit: int) -> List[Dict[str, Any]]:
        """AND поиск - все термины"""
        and_conditions = []
        for term in terms:
            regex = {"$regex": term, "$options": "i"}
            and_conditions.append({
                "$or": [
                    {"title": regex},
                    {"content": regex}
                ]
            })
        
        query = {"$and": and_conditions}
        results = list(self.pages_collection.find(query).limit(limit))
        return results
    
    def _search_or(self, terms: List[str], limit: int) -> List[Dict[str, Any]]:
        """OR поиск - хотя бы один термин"""
        or_conditions = []
        for term in terms:
            regex = {"$regex": term, "$options": "i"}
            or_conditions.extend([
                {"title": regex},
                {"content": regex}
            ])
        
        query = {"$or": or_conditions}
        results = list(self.pages_collection.find(query).limit(limit))
        return results
    
    def _search_not(self, term: str, limit: int) -> List[Dict[str, Any]]:
        """NOT поиск - термина не должно быть"""
        regex = {"$regex": term, "$options": "i"}
        query = {
            "$and": [
                {"title": {"$not": regex}},
                {"content": {"$not": regex}}
            ]
        }
        
        results = list(self.pages_collection.find(query).limit(limit))
        return results
    
    def display_results(self, results: List[Dict[str, Any]], query: str):
        """Отображение результатов"""
        if not results:
            print(f"\n❌ No results found for: '{query}'\n")
            return
        
        print(f"\n✅ Found {len(results)} results for: '{query}'\n")
        print("=" * 80)
        
        for i, doc in enumerate(results, 1):
            print(f"\n{i}. {doc.get('title', 'No title')}")
            print(f"   URL: {doc.get('url', 'No URL')}")
            
            # Краткий сниппет
            content = doc.get('content', '')
            if content and len(content) > 0:
                snippet = content[:150]
                if len(content) > 150:
                    snippet += "..."
                print(f"   Preview: {snippet}")
        
        print("\n" + "=" * 80)


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Simple Python Boolean Search')
    parser.add_argument('--query', '-q', type=str, help='Search query')
    parser.add_argument('--interactive', '-i', action='store_true',
                       help='Interactive mode')
    parser.add_argument('--limit', '-l', type=int, default=10,
                       help='Max results')
    parser.add_argument('--config', '-c', type=str, default='config.yaml',
                       help='Config file path')
    
    args = parser.parse_args()
    
    try:
        config_path = Path(__file__).parent.parent / args.config
        search_engine = SimpleBooleanSearch(str(config_path))
        
        if args.interactive:
            print("\n=== Simple Boolean Search (Interactive) ===")
            print("Supported operators: && (AND), || (OR), ! (NOT)")
            print("Examples:")
            print("  математика && информация")
            print("  алгебра || геометрия")
            print("  математика && !физика")
            print("\nType 'quit' to exit\n")
            
            while True:
                try:
                    query = input("Query> ").strip()
                    
                    if not query:
                        continue
                    
                    if query.lower() in ['quit', 'exit']:
                        break
                    
                    results = search_engine.search(query, args.limit)
                    search_engine.display_results(results, query)
                    
                except KeyboardInterrupt:
                    print("\n\nGoodbye!")
                    break
                except Exception as e:
                    print(f"❌ Error: {e}\n")
        
        elif args.query:
            results = search_engine.search(args.query, args.limit)
            search_engine.display_results(results, args.query)
        else:
            parser.print_help()
        
        return 0
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
