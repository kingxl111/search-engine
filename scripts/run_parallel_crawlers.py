#!/usr/bin/env python3
"""
Скрипт для параллельного сбора документов из нескольких источников
"""

import argparse
import logging
import sys
import multiprocessing as mp
from pathlib import Path
from typing import List, Dict

# Добавляем корневую директорию в путь для импорта
sys.path.insert(0, str(Path(__file__).parent.parent))

from src.crawler.universal_crawler import UniversalCrawler
from src.utils.config_loader import ConfigLoader
from src.utils.logger import setup_logger


def crawl_wikipedia(pages_per_source: int, category: str = "Математика"):
    """Собирает страницы из Wikipedia"""
    logger = setup_logger(f'crawler_wikipedia')
    logger.info(f"Starting Wikipedia crawler for category: {category}")
    
    config = ConfigLoader.load_config()
    config['crawler']['max_pages'] = pages_per_source
    config['crawler']['delay'] = 1.0
    config['wikipedia']['category'] = category
    
    # Импортируем Wikipedia crawler
    from src.crawler.crawler import WikipediaCrawler
    
    crawler = WikipediaCrawler(config)
    crawler.start()
    
    logger.info(f"Wikipedia crawler completed: {crawler.pages_collected} pages")


def crawl_habr(pages_per_source: int):
    """Собирает статьи с Habr"""
    logger = setup_logger('crawler_habr')
    logger.info("Starting Habr crawler")
    
    config = ConfigLoader.load_config()
    config['crawler']['max_pages'] = pages_per_source
    config['crawler']['delay'] = 2.0  # Habr требует больше вежливости
    config['crawler']['min_article_length'] = 500  # Статьи на Habr обычно длинные
    
    # Начальные URL - главные разделы Habr
    start_urls = [
        "https://habr.com/ru/flows/develop/articles/",
        "https://habr.com/ru/flows/admin/articles/",
        "https://habr.com/ru/flows/design/articles/",
        "https://habr.com/ru/flows/management/articles/",
        "https://habr.com/ru/flows/popsci/articles/",
    ]
    
    crawler = UniversalCrawler(config, source_name="habr")
    crawler.start(start_urls)
    
    logger.info(f"Habr crawler completed: {crawler.pages_collected} pages")


def crawl_stackoverflow_ru(pages_per_source: int):
    """Собирает вопросы с ru.stackoverflow.com"""
    logger = setup_logger('crawler_stackoverflow')
    logger.info("Starting StackOverflow (ru) crawler")
    
    config = ConfigLoader.load_config()
    config['crawler']['max_pages'] = pages_per_source
    config['crawler']['delay'] = 2.0
    config['crawler']['min_article_length'] = 300  # Вопросы могут быть короче
    
    # Начальные URL - популярные теги
    start_urls = [
        "https://ru.stackoverflow.com/questions/tagged/python",
        "https://ru.stackoverflow.com/questions/tagged/javascript",
        "https://ru.stackoverflow.com/questions/tagged/java",
        "https://ru.stackoverflow.com/questions/tagged/c%2b%2b",
        "https://ru.stackoverflow.com/questions/tagged/алгоритм",
        "https://ru.stackoverflow.com/questions?tab=votes",
    ]
    
    crawler = UniversalCrawler(config, source_name="stackoverflow")
    crawler.start(start_urls)
    
    logger.info(f"StackOverflow crawler completed: {crawler.pages_collected} pages")


def crawl_custom_urls(pages_per_source: int, urls_file: str, source_name: str):
    """Собирает страницы из файла с URL"""
    logger = setup_logger(f'crawler_{source_name}')
    logger.info(f"Starting custom crawler: {source_name}")
    
    # Читаем URL из файла
    try:
        with open(urls_file, 'r', encoding='utf-8') as f:
            start_urls = [line.strip() for line in f if line.strip()]
    except FileNotFoundError:
        logger.error(f"URLs file not found: {urls_file}")
        return
    
    logger.info(f"Loaded {len(start_urls)} URLs from {urls_file}")
    
    config = ConfigLoader.load_config()
    config['crawler']['max_pages'] = pages_per_source
    config['crawler']['delay'] = 1.5
    
    crawler = UniversalCrawler(config, source_name=source_name)
    crawler.start(start_urls)
    
    logger.info(f"{source_name} crawler completed: {crawler.pages_collected} pages")


def run_parallel_crawlers(sources: List[str], pages_per_source: int, custom_sources: Dict[str, str] = None):
    """
    Запускает несколько crawler'ов параллельно
    
    Args:
        sources: Список источников ('wikipedia', 'habr', 'stackoverflow')
        pages_per_source: Сколько страниц собрать с каждого источника
        custom_sources: Словарь {имя_источника: путь_к_файлу_с_urls}
    """
    processes = []
    
    # Запускаем crawler для каждого источника
    if 'wikipedia' in sources:
        p = mp.Process(target=crawl_wikipedia, args=(pages_per_source,))
        p.start()
        processes.append(('wikipedia', p))
    
    if 'habr' in sources:
        p = mp.Process(target=crawl_habr, args=(pages_per_source,))
        p.start()
        processes.append(('habr', p))
    
    if 'stackoverflow' in sources:
        p = mp.Process(target=crawl_stackoverflow_ru, args=(pages_per_source,))
        p.start()
        processes.append(('stackoverflow', p))
    
    # Пользовательские источники
    if custom_sources:
        for source_name, urls_file in custom_sources.items():
            p = mp.Process(target=crawl_custom_urls, args=(pages_per_source, urls_file, source_name))
            p.start()
            processes.append((source_name, p))
    
    print(f"\n{'='*60}")
    print(f"Started {len(processes)} parallel crawlers:")
    for name, _ in processes:
        print(f"  - {name}")
    print(f"{'='*60}\n")
    
    # Ждем завершения всех процессов
    for name, process in processes:
        print(f"Waiting for {name} crawler to finish...")
        process.join()
        print(f"✓ {name} crawler completed (exit code: {process.exitcode})")
    
    print(f"\n{'='*60}")
    print("All crawlers completed!")
    print(f"{'='*60}\n")


def main():
    parser = argparse.ArgumentParser(
        description='Параллельный сбор документов из нескольких источников',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Примеры использования:

1. Собрать по 10000 страниц из Wikipedia, Habr и StackOverflow:
   python scripts/run_parallel_crawlers.py --sources wikipedia habr stackoverflow --pages 10000

2. Собрать только из Wikipedia и Habr:
   python scripts/run_parallel_crawlers.py --sources wikipedia habr --pages 5000

3. Добавить свой источник:
   python scripts/run_parallel_crawlers.py --sources wikipedia --pages 10000 \\
       --custom-source "nauchpop:data/nauchpop_urls.txt"

Источники:
  wikipedia    - Wikipedia (категория из config.yaml)
  habr         - Habr.com (статьи)
  stackoverflow - ru.stackoverflow.com (вопросы)
        """
    )
    
    parser.add_argument('--sources', nargs='+', 
                       choices=['wikipedia', 'habr', 'stackoverflow'],
                       default=['wikipedia'],
                       help='Источники для сбора')
    
    parser.add_argument('--pages', type=int, default=10000,
                       help='Количество страниц с каждого источника (default: 10000)')
    
    parser.add_argument('--custom-source', action='append',
                       help='Дополнительный источник в формате "имя:путь_к_urls.txt"')
    
    parser.add_argument('--sequential', action='store_true',
                       help='Запустить последовательно вместо параллельно (для отладки)')
    
    args = parser.parse_args()
    
    # Парсим пользовательские источники
    custom_sources = {}
    if args.custom_source:
        for custom in args.custom_source:
            if ':' not in custom:
                print(f"Error: Invalid custom source format: {custom}")
                print("Use format: name:path/to/urls.txt")
                sys.exit(1)
            name, path = custom.split(':', 1)
            custom_sources[name] = path
    
    total_sources = len(args.sources) + len(custom_sources)
    total_pages = args.pages * total_sources
    
    print(f"\n{'='*60}")
    print("PARALLEL CRAWLER CONFIGURATION")
    print(f"{'='*60}")
    print(f"Sources: {', '.join(args.sources + list(custom_sources.keys()))}")
    print(f"Pages per source: {args.pages}")
    print(f"Total pages target: {total_pages}")
    print(f"Mode: {'Sequential' if args.sequential else 'Parallel'}")
    print(f"{'='*60}\n")
    
    if args.sequential:
        # Последовательный запуск (для отладки)
        if 'wikipedia' in args.sources:
            crawl_wikipedia(args.pages)
        if 'habr' in args.sources:
            crawl_habr(args.pages)
        if 'stackoverflow' in args.sources:
            crawl_stackoverflow_ru(args.pages)
        
        for name, path in custom_sources.items():
            crawl_custom_urls(args.pages, path, name)
    else:
        # Параллельный запуск
        run_parallel_crawlers(args.sources, args.pages, custom_sources)
    
    print("\n✓ All done! Check MongoDB for collected documents.")
    print(f"  Total expected documents: ~{total_pages}")


if __name__ == '__main__':
    # Для multiprocessing на macOS
    mp.set_start_method('spawn', force=True)
    main()
