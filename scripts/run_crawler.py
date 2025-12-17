#!/usr/bin/env python3
"""
Скрипт для запуска краулера и сбора корпуса документов
"""

import argparse
import logging
import sys
from pathlib import Path

# Добавляем корневую директорию в путь для импорта
sys.path.insert(0, str(Path(__file__).parent.parent))

from src.crawler.crawler import WikipediaCrawler
from src.utils.config_loader import ConfigLoader
from src.utils.logger import setup_logger


def main():
    parser = argparse.ArgumentParser(description='Запуск краулера для сбора корпуса документов')
    parser.add_argument('--category', type=str, default='Наука',
                       help='Категория статей Википедии для сбора')
    parser.add_argument('--pages', type=int, default=30000,
                       help='Максимальное количество страниц для сбора')
    parser.add_argument('--depth', type=int, default=3,
                       help='Максимальная глубина обхода')
    parser.add_argument('--delay', type=float, default=1.0,
                       help='Задержка между запросами в секундах')
    parser.add_argument('--config', type=str, default='config.yaml',
                       help='Путь к файлу конфигурации')
    parser.add_argument('--resume', action='store_true',
                       help='Продолжить с предыдущей точки')
    
    args = parser.parse_args()
    
    # Настраиваем логирование
    logger = setup_logger('crawler_script')
    
    # Загружаем конфигурацию
    config = ConfigLoader.load_config(args.config)
    
    # Обновляем параметры из командной строки
    config['crawler']['max_pages'] = args.pages
    config['crawler']['max_depth'] = args.depth
    config['crawler']['delay'] = args.delay
    config['wikipedia']['category'] = args.category
    
    logger.info(f"Starting crawler for category: {args.category}")
    logger.info(f"Target pages: {args.pages}")
    logger.info(f"Max depth: {args.depth}")
    
    try:
        # Создаем и запускаем краулер
        crawler = WikipediaCrawler(config)
        
        if args.resume:
            logger.info("Resuming from last checkpoint...")
            crawler.resume()
        else:
            logger.info("Starting new crawl...")
            crawler.start()
            
    except KeyboardInterrupt:
        logger.info("Crawler stopped by user")
        sys.exit(0)
    except Exception as e:
        logger.error(f"Error during crawling: {e}", exc_info=True)
        sys.exit(1)
    
    logger.info("Crawling completed successfully!")


if __name__ == '__main__':
    main()