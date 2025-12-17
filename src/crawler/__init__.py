"""
Пакет для сбора корпуса документов
"""

from .crawler import WikipediaCrawler
from .url_manager import URLManager
from .page_downloader import PageDownloader
from .robots_parser import RobotsParser
from .database_handler import DatabaseHandler

__all__ = [
    'WikipediaCrawler',
    'URLManager',
    'PageDownloader',
    'RobotsParser',
    'DatabaseHandler'
]