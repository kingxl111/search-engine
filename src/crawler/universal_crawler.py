"""
Универсальный краулер для сбора документов из разных источников
"""

import logging
import time
import json
from typing import List, Dict, Optional
from datetime import datetime
from pathlib import Path

from src.utils.logger import logger
from src.crawler.url_manager import URLManager
from src.crawler.page_downloader import PageDownloader
from src.crawler.robots_parser import RobotsParser
from src.crawler.database_handler import DatabaseHandler
from src.utils.config_loader import ConfigLoader


class UniversalCrawler:
    """Универсальный краулер для разных источников"""
    
    def __init__(self, config: Dict = None, source_name: str = "unknown"):
        """
        Инициализация краулера
        
        Args:
            config: Конфигурация краулера
            source_name: Название источника для логирования
        """
        if config is None:
            config = ConfigLoader.load_config()
        
        self.config = config
        self.source_name = source_name
        self.logger = logging.getLogger(f"{__name__}.{source_name}")
        
        # Настройки краулера
        crawler_config = config.get('crawler', {})
        
        self.user_agent = crawler_config.get('user_agent', 'SearchEngineBot/1.0')
        self.delay = crawler_config.get('delay', 1.0)
        self.max_pages = crawler_config.get('max_pages', 10000)
        self.max_depth = crawler_config.get('max_depth', 3)
        self.min_article_length = crawler_config.get('min_article_length', 1000)
        
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
        self.save_interval = 50  # Сохранять состояние каждые N страниц
        
        # Пути для сохранения состояния
        Path('data').mkdir(exist_ok=True)
        self.state_file = f'data/crawler_state_{source_name}.json'
        
        self.logger.info(f"UniversalCrawler initialized for source: {source_name}")
    
    def start(self, start_urls: List[str]) -> None:
        """
        Запускает процесс сбора документов
        
        Args:
            start_urls: Список начальных URL для сбора
        """
        if self.is_running:
            self.logger.warning("Crawler is already running")
            return
        
        if not start_urls:
            self.logger.error("No start URLs provided")
            return
        
        self.is_running = True
        self.start_time = datetime.now()
        
        self.logger.info("=" * 60)
        self.logger.info(f"Starting crawler for source: {self.source_name}")
        self.logger.info(f"Start URLs count: {len(start_urls)}")
        self.logger.info(f"Max pages: {self.max_pages}")
        self.logger.info(f"Max depth: {self.max_depth}")
        self.logger.info("=" * 60)
        
        try:
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
                self.logger.warning("Could not load saved state")
                
        except Exception as e:
            self.logger.error(f"Error resuming: {e}")
    
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
            
            # Добавляем метку источника
            page_data['crawler_source'] = self.source_name
            
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
            
            self.logger.info(f"✓ [{self.source_name}] Processed page {self.pages_collected + 1}: {page_data.get('title', 'No title')}")
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
        self.logger.info(f"[{self.source_name}] Progress: {self.pages_collected}/{self.max_pages} pages")
        self.logger.info(f"Elapsed time: {str(elapsed).split('.')[0]}")
        self.logger.info(f"Speed: {pages_per_second:.2f} pages/sec")
        self.logger.info(f"Queue size: {stats.get('queue_size', 0)}")
        self.logger.info(f"Visited URLs: {stats.get('visited_count', 0)}")
        self.logger.info("=" * 60)
    
    def _log_final_stats(self) -> None:
        """Логирует финальную статистику"""
        elapsed = datetime.now() - self.start_time
        
        self.logger.info("=" * 60)
        self.logger.info(f"[{self.source_name}] CRAWLING COMPLETED")
        self.logger.info("=" * 60)
        self.logger.info(f"Total pages collected: {self.pages_collected}")
        self.logger.info(f"Total time: {str(elapsed).split('.')[0]}")
        self.logger.info(f"Average speed: {self.pages_collected/elapsed.total_seconds():.2f} pages/sec")
        self.logger.info("=" * 60)
    
    def _save_state(self) -> None:
        """Сохраняет состояние краулера"""
        if self.url_manager:
            self.url_manager.save_state(self.state_file)
            self.logger.debug(f"[{self.source_name}] Crawler state saved")
    
    def _cleanup(self) -> None:
        """Очищает ресурсы"""
        self.is_running = False
        
        if self.page_downloader:
            self.page_downloader.close()
        
        if self.database_handler:
            self.database_handler.close()
        
        self.logger.info(f"[{self.source_name}] Crawler cleanup completed")
