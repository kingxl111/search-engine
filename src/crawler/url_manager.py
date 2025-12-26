"""
Менеджер URL для управления очередью и обработкой ссылок
"""

import logging
from typing import List, Set, Dict, Optional, Tuple
from urllib.parse import urlparse, urljoin, urldefrag
from collections import deque
import hashlib
from datetime import datetime


class URLManager:
    """Класс для управления URL в процессе сканирования"""
    
    def __init__(self, start_urls: List[str], max_depth: int = 3):
        """
        Инициализация менеджера URL
        
        Args:
            start_urls: Начальные URL для сканирования
            max_depth: Максимальная глубина сканирования
        """
        self.logger = logging.getLogger(__name__)
        
        # Очередь URL для обработки: (url, depth)
        self.url_queue = deque()
        
        # Множества для отслеживания посещенных URL
        self.visited_urls: Set[str] = set()
        self.pending_urls: Set[str] = set()
        
        # Статистика
        self.stats = {
            'total_discovered': 0,
            'total_visited': 0,
            'total_skipped': 0,
        }
        
        # Максимальная глубина
        self.max_depth = max_depth
        
        # Инициализация начальными URL
        self._initialize_queue(start_urls)
    
    def _initialize_queue(self, start_urls: List[str]) -> None:
        """Инициализирует очередь начальными URL"""
        for url in start_urls:
            normalized_url = self._normalize_url(url)
            if normalized_url:
                self.url_queue.append((normalized_url, 0))
                self.pending_urls.add(normalized_url)
                self.stats['total_discovered'] += 1
        
        self.logger.info(f"Initialized with {len(start_urls)} start URLs")
    
    def _normalize_url(self, url: str) -> Optional[str]:
        """
        Нормализует URL: удаляет фрагменты, приводит к нижнему регистру
        
        Args:
            url: URL для нормализации
            
        Returns:
            Нормализованный URL или None если URL невалидный
        """
        try:
            # Удаляем фрагмент
            url, _ = urldefrag(url)
            
            # Парсим URL
            parsed = urlparse(url)
            
            # Проверяем наличие схемы
            if not parsed.scheme:
                url = 'http://' + url
                parsed = urlparse(url)
            
            # Приводим к нижнему регистру
            normalized = parsed.geturl().lower()
            
            return normalized
        except Exception as e:
            self.logger.warning(f"Invalid URL {url}: {e}")
            return None
    
    def get_next_url(self) -> Optional[Tuple[str, int]]:
        """
        Получает следующий URL для обработки
        
        Returns:
            Кортеж (url, depth) или None если очередь пуста
        """
        if not self.url_queue:
            return None
        
        url, depth = self.url_queue.popleft()
        
        # Приводим depth к int (может прийти как строка из JSON)
        depth = int(depth)
        
        # Помечаем как обрабатываемый
        self.pending_urls.remove(url)
        self.visited_urls.add(url)
        self.stats['total_visited'] += 1
        
        return url, depth
    
    def add_urls(self, urls: List[str], current_depth: int, 
                 base_url: str = None) -> int:
        """
        Добавляет новые URL в очередь
        
        Args:
            urls: Список новых URL
            current_depth: Текущая глубина
            base_url: Базовый URL для относительных ссылок
            
        Returns:
            Количество добавленных URL
        """
        added_count = 0
        
        # Приводим depth к int (может прийти как строка из JSON)
        current_depth = int(current_depth)
        
        if current_depth >= self.max_depth:
            return added_count
        
        for url in urls:
            # Преобразуем относительные URL в абсолютные
            if base_url and not url.startswith(('http://', 'https://')):
                url = urljoin(base_url, url)
            
            # Нормализуем URL
            normalized_url = self._normalize_url(url)
            if not normalized_url:
                continue
            
            # Проверяем, не посещали ли уже этот URL
            if (normalized_url in self.visited_urls or 
                normalized_url in self.pending_urls):
                self.stats['total_skipped'] += 1
                continue
            
            # Добавляем в очередь
            self.url_queue.append((normalized_url, current_depth + 1))
            self.pending_urls.add(normalized_url)
            added_count += 1
            self.stats['total_discovered'] += 1
        
        return added_count
    
    def mark_url_as_failed(self, url: str, error: str = None) -> None:
        """
        Помечает URL как неудачный (но все равно посещенный)
        
        Args:
            url: URL который не удалось обработать
            error: Описание ошибки
        """
        normalized_url = self._normalize_url(url)
        if normalized_url:
            if normalized_url in self.pending_urls:
                self.pending_urls.remove(normalized_url)
            self.visited_urls.add(normalized_url)
            self.stats['total_visited'] += 1
            
            if error:
                self.logger.warning(f"Failed to process {url}: {error}")
    
    def has_pending_urls(self) -> bool:
        """Проверяет, есть ли URL в очереди"""
        return len(self.url_queue) > 0
    
    def get_queue_size(self) -> int:
        """Возвращает размер очереди"""
        return len(self.url_queue)
    
    def get_visited_count(self) -> int:
        """Возвращает количество посещенных URL"""
        return len(self.visited_urls)
    
    def get_stats(self) -> Dict[str, any]:
        """Возвращает статистику"""
        stats = self.stats.copy()
        stats['queue_size'] = self.get_queue_size()
        stats['visited_count'] = self.get_visited_count()
        stats['pending_count'] = len(self.pending_urls)
        
        # Вычисляем время работы
        # elapsed = datetime.now() - stats['start_time']
        # stats['elapsed_time'] = str(elapsed)
        
        return stats
    
    def save_state(self, filepath: str) -> None:
        """
        Сохраняет состояние в файл
        
        Args:
            filepath: Путь к файлу для сохранения
        """
        import json
        
        state = {
            'url_queue': list(self.url_queue),
            'visited_urls': list(self.visited_urls),
            'pending_urls': list(self.pending_urls),
            'stats': self.stats,
            'max_depth': self.max_depth
        }
        
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(state, f, ensure_ascii=False, indent=2)
        
        self.logger.info(f"State saved to {filepath}")
    
    def load_state(self, filepath: str) -> bool:
        """
        Загружает состояние из файла
        
        Args:
            filepath: Путь к файлу для загрузки
            
        Returns:
            True если загрузка успешна
        """
        import json
        
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                state = json.load(f)
            
            # Восстанавливаем очередь
            self.url_queue = deque([tuple(item) for item in state['url_queue']])
            
            # Восстанавливаем множества
            self.visited_urls = set(state['visited_urls'])
            self.pending_urls = set(state['pending_urls'])
            
            # Восстанавливаем статистику
            self.stats = state['stats']
            self.max_depth = state['max_depth']
            
            self.logger.info(f"State loaded from {filepath}")
            return True
            
        except Exception as e:
            self.logger.error(f"Failed to load state from {filepath}: {e}")
            return False