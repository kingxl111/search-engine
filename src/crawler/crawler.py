"""
Главный класс краулера для сбора документов из Википедии
"""

import logging
import time
import json
from typing import List, Dict, Optional
from datetime import datetime
from urllib.parse import urlparse
import requests

from src.utils.logger import logger

from .url_manager import URLManager
from .page_downloader import PageDownloader
from .robots_parser import RobotsParser
from .database_handler import DatabaseHandler
from src.utils.config_loader import ConfigLoader


class WikipediaCrawler:
    """Краулер для сбора статей из Википедии"""
    
    def __init__(self, config: Dict = None):
        """
        Инициализация краулера
        
        Args:
            config: Конфигурация краулера
        """
        if config is None:
            config = ConfigLoader.load_config()
        
        self.config = config
        self.logger = logging.getLogger(__name__)
        
        # Настройки краулера
        crawler_config = config.get('crawler', {})
        wikipedia_config = config.get('wikipedia', {})
        
        self.user_agent = crawler_config.get('user_agent', 'SearchEngineBot/1.0')
        self.delay = crawler_config.get('delay', 1.0)
        self.max_pages = crawler_config.get('max_pages', 30000)
        self.max_depth = crawler_config.get('max_depth', 3)
        
        # Настройки Википедии
        self.api_url = wikipedia_config.get('api_url', 'https://ru.wikipedia.org/w/api.php')
        self.category = wikipedia_config.get('category', 'Наука')
        self.language = wikipedia_config.get('language', 'ru')
        self.namespace = wikipedia_config.get('namespace', 0)
        self.include_subcategories = wikipedia_config.get('include_subcategories', True)
        self.min_article_length = wikipedia_config.get('min_article_length', 1000)
        
        # Инициализация компонентов
        self.robots_parser = RobotsParser(self.user_agent)
        self.page_downloader = PageDownloader(config)
        self.database_handler = DatabaseHandler()
        
        # Менеджер URL
        self.url_manager: Optional[URLManager] = None
        
        # Состояние
        self.is_running = False
        self.start_time = None
        self.pages_collected = 0
        self.save_interval = 100  # Сохранять состояние каждые N страниц
        
        # Пути для сохранения состояния
        self.state_file = 'data/crawler_state.json'
        
        self.logger.info(f"WikipediaCrawler initialized for category: {self.category}")
    
    def start(self) -> None:
        """Запускает процесс сбора документов"""
        if self.is_running:
            self.logger.warning("Crawler is already running")
            return
        
        self.is_running = True
        self.start_time = datetime.now()
        
        self.logger.info("=" * 60)
        self.logger.info(f"Starting Wikipedia crawler")
        self.logger.info(f"Category: {self.category}")
        self.logger.info(f"Max pages: {self.max_pages}")
        self.logger.info(f"Max depth: {self.max_depth}")
        self.logger.info("=" * 60)
        
        try:
            # Получаем начальные URL из категории Википедии
            start_urls = self._get_wikipedia_category_pages()
            
            if not start_urls:
                self.logger.error("No pages found in the specified category")
                return
            
            # Инициализируем менеджер URL
            self.url_manager = URLManager(start_urls, self.max_depth)
            
            # Запускаем основной цикл сканирования
            self._crawl_loop()
            
        except KeyboardInterrupt:
            self.logger.info("Crawler stopped by user")
        except Exception as e:
            self.logger.error(f"Crawler error: {e}", exc_info=True)
        finally:
            self._cleanup()
    
    def resume(self) -> None:
        """Продолжает с предыдущей точки"""
        self.logger.info("Attempting to resume from saved state...")
        
        try:
            # Пытаемся загрузить состояние
            if self.url_manager is None:
                self.url_manager = URLManager([], self.max_depth)
            
            if self.url_manager.load_state(self.state_file):
                self.logger.info("Successfully loaded saved state")
                
                # Получаем статистику из БД
                db_stats = self.database_handler.get_stats()
                self.pages_collected = db_stats.get('total_pages', 0)
                
                # Запускаем цикл сканирования
                self.is_running = True
                self.start_time = datetime.now()
                self._crawl_loop()
            else:
                self.logger.warning("Could not load saved state, starting fresh")
                self.start()
                
        except Exception as e:
            self.logger.error(f"Error resuming: {e}")
            self.start()
    
    def _get_wikipedia_category_pages(self) -> List[str]:
        """
        Получает список страниц из категории Википедии
        
        Returns:
            Список URL страниц
        """
        self.logger.info(f"Fetching pages from Wikipedia category: {self.category}")
        
        pages = []
        continue_token = None
        
        while len(pages) < self.max_pages * 2:  # Получаем больше, чтобы учесть фильтрацию
            params = {
                'action': 'query',
                'format': 'json',
                'list': 'categorymembers',
                'cmtitle': f'Category:{self.category}',
                'cmlimit': 500,  # Максимально разрешенное значение
                'cmnamespace': self.namespace,
                'cmtype': 'page'
            }
            
            if continue_token:
                params['cmcontinue'] = continue_token
            
            try:
                response = requests.get(
                    self.api_url,
                    params=params,
                    headers={'User-Agent': self.user_agent},
                    timeout=30
                )
                
                if response.status_code != 200:
                    self.logger.error(f"Wikipedia API error: {response.status_code}")
                    break
                
                data = response.json()
                
                # Извлекаем страницы
                category_members = data.get('query', {}).get('categorymembers', [])
                
                for member in category_members:
                    title = member.get('title', '')
                    page_id = member.get('pageid', '')
                    
                    if title and page_id:
                        # Создаем URL страницы
                        page_url = f"https://{self.language}.wikipedia.org/wiki/{title.replace(' ', '_')}"
                        pages.append(page_url)
                
                # Проверяем, есть ли еще страницы
                continue_token = data.get('continue', {}).get('cmcontinue')
                if not continue_token:
                    break
                
                self.logger.debug(f"Fetched {len(pages)} pages so far...")
                time.sleep(0.1)  # Небольшая задержка между запросами
                
            except Exception as e:
                self.logger.error(f"Error fetching Wikipedia category: {e}")
                break
        
        self.logger.info(f"Found {len(pages)} pages in category '{self.category}'")
        
        # Если включены подкатегории, получаем и их страницы
        if self.include_subcategories:
            subcategories = self._get_wikipedia_subcategories()
            
            for subcategory in subcategories[:10]:  # Ограничиваем количество подкатегорий
                self.logger.info(f"Fetching pages from subcategory: {subcategory}")
                subcategory_pages = self._get_pages_from_category(subcategory)
                pages.extend(subcategory_pages)
        
        # Ограничиваем количество страниц
        pages = pages[:self.max_pages * 2]
        
        self.logger.info(f"Total pages to process: {len(pages)}")
        return pages
    
    def _get_wikipedia_subcategories(self) -> List[str]:
        """
        Получает подкатегории из указанной категории
        
        Returns:
            Список названий подкатегорий
        """
        subcategories = []
        continue_token = None
        
        while True:
            params = {
                'action': 'query',
                'format': 'json',
                'list': 'categorymembers',
                'cmtitle': f'Category:{self.category}',
                'cmlimit': 500,
                'cmnamespace': 14,  # Пространство имен для категорий
                'cmtype': 'subcat'
            }
            
            if continue_token:
                params['cmcontinue'] = continue_token
            
            try:
                response = requests.get(
                    self.api_url,
                    params=params,
                    headers={'User-Agent': self.user_agent},
                    timeout=30
                )
                
                if response.status_code != 200:
                    break
                
                data = response.json()
                category_members = data.get('query', {}).get('categorymembers', [])
                
                for member in category_members:
                    title = member.get('title', '')
                    if title.startswith('Category:'):
                        subcategories.append(title[9:])  # Убираем 'Category:'
                
                continue_token = data.get('continue', {}).get('cmcontinue')
                if not continue_token:
                    break
                
                time.sleep(0.1)
                
            except Exception as e:
                self.logger.error(f"Error fetching subcategories: {e}")
                break
        
        return subcategories
    
    def _get_pages_from_category(self, category: str) -> List[str]:
        """
        Получает страницы из указанной категории
        
        Args:
            category: Название категории
            
        Returns:
            Список URL страниц
        """
        pages = []
        continue_token = None
        
        while len(pages) < 1000:  # Ограничиваем на подкатегорию
            params = {
                'action': 'query',
                'format': 'json',
                'list': 'categorymembers',
                'cmtitle': f'Category:{category}',
                'cmlimit': 500,
                'cmnamespace': 0,
                'cmtype': 'page'
            }
            
            if continue_token:
                params['cmcontinue'] = continue_token
            
            try:
                response = requests.get(
                    self.api_url,
                    params=params,
                    headers={'User-Agent': self.user_agent},
                    timeout=30
                )
                
                if response.status_code != 200:
                    break
                
                data = response.json()
                category_members = data.get('query', {}).get('categorymembers', [])
                
                for member in category_members:
                    title = member.get('title', '')
                    page_id = member.get('pageid', '')
                    
                    if title and page_id:
                        page_url = f"https://{self.language}.wikipedia.org/wiki/{title.replace(' ', '_')}"
                        pages.append(page_url)
                
                continue_token = data.get('continue', {}).get('cmcontinue')
                if not continue_token:
                    break
                
                time.sleep(0.1)
                
            except Exception as e:
                self.logger.error(f"Error fetching pages from category {category}: {e}")
                break
        
        return pages
    
    def _crawl_loop(self) -> None:
        """Основной цикл сканирования"""
        last_stats_time = time.time()
        last_save_time = time.time()
        
        while self.is_running and self.url_manager.has_pending_urls():
            # Проверяем лимит страниц
            if self.pages_collected >= self.max_pages:
                self.logger.info(f"Reached maximum pages limit: {self.max_pages}")
                break
            
            # Получаем следующий URL
            url_info = self.url_manager.get_next_url()
            logger.info(str(url_info))
            if not url_info:
                break
            
            url, depth = url_info
            
            # Обрабатываем страницу
            success = self._process_page(url, depth)
            
            if success:
                self.pages_collected += 1
            
            # Логируем статистику каждые 10 секунд
            current_time = time.time()
            if current_time - last_stats_time >= 10:
                self._log_stats()
                last_stats_time = current_time
            
            # Сохраняем состояние каждые N страниц
            if self.pages_collected % self.save_interval == 0:
                self._save_state()
                last_save_time = current_time
            
            # Соблюдаем задержку
            time.sleep(self.delay)
        
        self._log_final_stats()
        self._save_state()
    
    def _process_page(self, url: str, depth: int) -> bool:
        """
        Обрабатывает одну страницу
        
        Args:
            url: URL страницы
            depth: Глубина
            
        Returns:
            True если страница успешно обработана
        """
        self.logger.debug(f"Processing page {self.pages_collected + 1}/{self.max_pages}: {url} (depth: {depth})")
        
        try:
            # Загружаем страницу
            page_data = self.page_downloader.download_page(url, self.robots_parser)
            
            if not page_data:
                self.url_manager.mark_url_as_failed(url, "Failed to download")
                return False
            
            # Проверяем длину контента
            content = page_data.get('content', '')
            if len(content) < self.min_article_length:
                self.logger.debug(f"Page too short ({len(content)} chars): {url}")
                self.url_manager.mark_url_as_failed(url, "Content too short")
                return False
            
            # Сохраняем в базу данных
            page_id = self.database_handler.save_page(page_data)
            
            if not page_id:
                self.url_manager.mark_url_as_failed(url, "Failed to save to database")
                return False
            
            # Извлекаем и добавляем новые ссылки
            links = page_data.get('links', [])
            if links and depth < self.max_depth:
                added = self.url_manager.add_urls(links, depth, url)
                self.logger.debug(f"Added {added} new links from {url}")
            
            self.logger.info(f"✓ Processed page {self.pages_collected + 1}: {page_data.get('title', 'No title')}")
            return True
            
        except Exception as e:
            self.logger.error(f"Error processing page {url}: {e}")
            self.url_manager.mark_url_as_failed(url, str(e))
            return False
    
    def _log_stats(self) -> None:
        """Логирует текущую статистику"""
        if not self.url_manager:
            return
        
        stats = self.url_manager.get_stats()
        db_stats = self.database_handler.get_stats()
        
        elapsed = datetime.now() - self.start_time
        pages_per_second = self.pages_collected / elapsed.total_seconds() if elapsed.total_seconds() > 0 else 0
        
        self.logger.info("=" * 60)
        self.logger.info(f"Progress: {self.pages_collected}/{self.max_pages} pages")
        self.logger.info(f"Elapsed time: {str(elapsed).split('.')[0]}")
        self.logger.info(f"Speed: {pages_per_second:.2f} pages/sec")
        self.logger.info(f"Queue size: {stats.get('queue_size', 0)}")
        self.logger.info(f"Visited URLs: {stats.get('visited_count', 0)}")
        self.logger.info(f"Total discovered: {stats.get('total_discovered', 0)}")
        self.logger.info(f"Database pages: {db_stats.get('total_pages', 0)}")
        self.logger.info(f"Content size: {db_stats.get('total_content_length_mb', 0):.2f} MB")
        self.logger.info("=" * 60)
    
    def _log_final_stats(self) -> None:
        """Логирует финальную статистику"""
        elapsed = datetime.now() - self.start_time
        
        self.logger.info("=" * 60)
        self.logger.info("CRAWLING COMPLETED")
        self.logger.info("=" * 60)
        self.logger.info(f"Total pages collected: {self.pages_collected}")
        self.logger.info(f"Total time: {str(elapsed).split('.')[0]}")
        self.logger.info(f"Average speed: {self.pages_collected/elapsed.total_seconds():.2f} pages/sec")
        
        # Статистика базы данных
        db_stats = self.database_handler.get_stats()
        self.logger.info(f"Pages in database: {db_stats.get('total_pages', 0)}")
        self.logger.info(f"Unique domains: {db_stats.get('unique_domains', 0)}")
        self.logger.info(f"Total content size: {db_stats.get('total_content_length_mb', 0):.2f} MB")
        self.logger.info(f"Average page size: {db_stats.get('avg_content_length_bytes', 0):.0f} bytes")
        self.logger.info("=" * 60)
    
    def _save_state(self) -> None:
        """Сохраняет состояние краулера"""
        if self.url_manager:
            self.url_manager.save_state(self.state_file)
            self.logger.debug("Crawler state saved")
    
    def _cleanup(self) -> None:
        """Очищает ресурсы"""
        self.is_running = False
        
        if self.page_downloader:
            self.page_downloader.close()
        
        if self.database_handler:
            self.database_handler.close()
        
        self.logger.info("Crawler cleanup completed")
    
    def get_corpus_info(self) -> Dict:
        """
        Возвращает информацию о собранном корпусе
        
        Returns:
            Словарь с информацией о корпусе
        """
        db_stats = self.database_handler.get_stats()
        
        info = {
            'total_pages': db_stats.get('total_pages', 0),
            'unique_domains': db_stats.get('unique_domains', 0),
            'total_content_length_mb': db_stats.get('total_content_length_mb', 0),
            'avg_content_length_bytes': db_stats.get('avg_content_length_bytes', 0),
            'processed_pages': db_stats.get('processed_pages', 0),
            'unprocessed_pages': db_stats.get('unprocessed_pages', 0),
            'category': self.category,
            'max_pages': self.max_pages,
            'pages_collected': self.pages_collected,
            'crawler_status': 'running' if self.is_running else 'stopped'
        }
        
        return info