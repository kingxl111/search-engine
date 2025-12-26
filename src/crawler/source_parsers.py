"""
Специализированные парсеры для разных источников
"""

import logging
from typing import Dict, List, Optional
from bs4 import BeautifulSoup
from urllib.parse import urljoin, urlparse
import re


class BaseSourceParser:
    """Базовый класс для парсеров источников"""
    
    def __init__(self):
        self.logger = logging.getLogger(__name__)
    
    def can_parse(self, url: str) -> bool:
        """
        Проверяет, может ли этот парсер обработать URL
        
        Args:
            url: URL страницы
            
        Returns:
            True если парсер подходит для этого URL
        """
        raise NotImplementedError
    
    def parse(self, url: str, html_text: str, soup: BeautifulSoup) -> Dict:
        """
        Парсит страницу
        
        Args:
            url: URL страницы
            html_text: HTML текст
            soup: BeautifulSoup объект
            
        Returns:
            Словарь с данными страницы
        """
        raise NotImplementedError
    
    def get_delay(self) -> float:
        """Возвращает рекомендуемую задержку для этого источника"""
        return 1.0


class WikipediaParser(BaseSourceParser):
    """Парсер для Wikipedia"""
    
    def can_parse(self, url: str) -> bool:
        parsed = urlparse(url)
        return 'wikipedia.org' in parsed.netloc
    
    def parse(self, url: str, html_text: str, soup: BeautifulSoup) -> Dict:
        """Парсит страницу Wikipedia"""
        
        # Заголовок
        title = ""
        title_tag = soup.find('h1', {'class': 'firstHeading'}) or soup.find('title')
        if title_tag:
            title = title_tag.get_text(strip=True)
            # Убираем суффикс " — Википедия"
            title = re.sub(r'\s*—\s*Википедия\s*$', '', title)
        
        # Контент (основное содержимое статьи)
        content = ""
        content_div = soup.find('div', {'id': 'mw-content-text'})
        if content_div:
            # Убираем навигационные элементы, таблицы оглавления, etc
            for unwanted in content_div.find_all(['table', 'div'], {'class': ['toc', 'navbox', 'vertical-navbox', 'infobox']}):
                unwanted.decompose()
            
            # Извлекаем параграфы
            paragraphs = content_div.find_all('p')
            content = ' '.join([p.get_text(strip=True) for p in paragraphs])
        
        # Метаданные
        meta_description = ""
        meta_tag = soup.find('meta', {'name': 'description'}) or soup.find('meta', {'property': 'og:description'})
        if meta_tag:
            meta_description = meta_tag.get('content', '')
        
        # Ссылки (только внутренние на другие статьи)
        links = []
        if content_div:
            for link in content_div.find_all('a', href=True):
                href = link['href']
                # Только статьи Wikipedia
                if href.startswith('/wiki/') and ':' not in href:
                    full_url = urljoin(url, href)
                    links.append(full_url)
        
        return {
            'url': url,
            'title': title,
            'content': content,
            'meta_description': meta_description,
            'links': list(set(links))[:50],  # Ограничиваем количество ссылок
            'source': 'wikipedia',
            'language': 'ru' if '.ru.' in url else 'en'
        }
    
    def get_delay(self) -> float:
        return 1.0  # Wikipedia просит 1 секунду между запросами


class HabrParser(BaseSourceParser):
    """Парсер для Habr"""
    
    def can_parse(self, url: str) -> bool:
        parsed = urlparse(url)
        return 'habr.com' in parsed.netloc
    
    def parse(self, url: str, html_text: str, soup: BeautifulSoup) -> Dict:
        """Парсит статью с Habr"""
        
        # Заголовок
        title = ""
        title_tag = soup.find('h1', {'class': 'tm-title'}) or soup.find('h1')
        if title_tag:
            title = title_tag.get_text(strip=True)
        
        # Контент статьи
        content = ""
        article_body = soup.find('div', {'class': 'tm-article-body'}) or soup.find('article')
        if article_body:
            # Удаляем рекламу и навигацию
            for unwanted in article_body.find_all(['div', 'aside'], {'class': ['tm-article-poll', 'tm-advertisement']}):
                unwanted.decompose()
            
            content = article_body.get_text(separator=' ', strip=True)
        
        # Метаданные
        meta_description = ""
        meta_tag = soup.find('meta', {'name': 'description'}) or soup.find('meta', {'property': 'og:description'})
        if meta_tag:
            meta_description = meta_tag.get('content', '')
        
        # Теги
        tags = []
        tag_elements = soup.find_all('a', {'class': 'tm-tags-list__link'})
        for tag_el in tag_elements:
            tags.append(tag_el.get_text(strip=True))
        
        # Автор
        author = ""
        author_tag = soup.find('a', {'class': 'tm-user-info__username'})
        if author_tag:
            author = author_tag.get_text(strip=True)
        
        # Дата публикации
        date = ""
        time_tag = soup.find('time')
        if time_tag:
            date = time_tag.get('datetime', '') or time_tag.get('title', '')
        
        # Ссылки (только на другие статьи Habr)
        links = []
        if article_body:
            for link in article_body.find_all('a', href=True):
                href = link['href']
                if '/articles/' in href or '/posts/' in href:
                    full_url = urljoin(url, href)
                    links.append(full_url)
        
        return {
            'url': url,
            'title': title,
            'content': content,
            'meta_description': meta_description,
            'tags': tags,
            'author': author,
            'published_date': date,
            'links': list(set(links))[:30],
            'source': 'habr',
            'language': 'ru'
        }
    
    def get_delay(self) -> float:
        return 2.0  # Habr - будем вежливыми


class StackOverflowRuParser(BaseSourceParser):
    """Парсер для StackOverflow на русском (ru.stackoverflow.com)"""
    
    def can_parse(self, url: str) -> bool:
        parsed = urlparse(url)
        return 'ru.stackoverflow.com' in parsed.netloc or 'stackoverflow.com' in parsed.netloc
    
    def parse(self, url: str, html_text: str, soup: BeautifulSoup) -> Dict:
        """Парсит вопрос/ответ со StackOverflow"""
        
        # Заголовок вопроса
        title = ""
        title_tag = soup.find('h1', {'itemprop': 'name'}) or soup.find('a', {'class': 's-link'})
        if title_tag:
            title = title_tag.get_text(strip=True)
        
        # Вопрос
        question = ""
        question_div = soup.find('div', {'class': 's-prose'}) or soup.find('div', {'class': 'question'})
        if question_div:
            question = question_div.get_text(separator=' ', strip=True)
        
        # Ответы
        answers = []
        answer_divs = soup.find_all('div', {'class': 'answer'})
        for answer_div in answer_divs[:3]:  # Берем топ-3 ответа
            answer_body = answer_div.find('div', {'class': 's-prose'})
            if answer_body:
                answers.append(answer_body.get_text(separator=' ', strip=True))
        
        # Объединяем вопрос и ответы
        content = f"{question} {' '.join(answers)}"
        
        # Теги
        tags = []
        tag_elements = soup.find_all('a', {'class': 'post-tag'})
        for tag_el in tag_elements:
            tags.append(tag_el.get_text(strip=True))
        
        # Метаданные
        meta_description = ""
        meta_tag = soup.find('meta', {'name': 'description'}) or soup.find('meta', {'property': 'og:description'})
        if meta_tag:
            meta_description = meta_tag.get('content', '')
        
        # Ссылки на связанные вопросы
        links = []
        related_div = soup.find('div', {'id': 'sidebar'})
        if related_div:
            for link in related_div.find_all('a', href=True):
                href = link['href']
                if '/questions/' in href:
                    full_url = urljoin(url, href)
                    links.append(full_url)
        
        return {
            'url': url,
            'title': title,
            'content': content,
            'meta_description': meta_description,
            'tags': tags,
            'answers_count': len(answers),
            'links': list(set(links))[:20],
            'source': 'stackoverflow',
            'language': 'ru' if 'ru.stackoverflow' in url else 'en'
        }
    
    def get_delay(self) -> float:
        return 2.0  # StackOverflow - разумная задержка


class GenericParser(BaseSourceParser):
    """Универсальный парсер для произвольных сайтов"""
    
    def can_parse(self, url: str) -> bool:
        # Всегда возвращает True как fallback
        return True
    
    def parse(self, url: str, html_text: str, soup: BeautifulSoup) -> Dict:
        """Общий парсинг для любых сайтов"""
        
        # Заголовок
        title = ""
        title_tag = soup.find('h1') or soup.find('title')
        if title_tag:
            title = title_tag.get_text(strip=True)
        
        # Контент - пробуем найти основной контент
        content = ""
        
        # Пробуем найти main, article или body
        main_content = (
            soup.find('main') or 
            soup.find('article') or 
            soup.find('div', {'class': ['content', 'post-content', 'article-content', 'main-content']}) or
            soup.find('body')
        )
        
        if main_content:
            # Удаляем навигацию, футер, сайдбары
            for unwanted in main_content.find_all(['nav', 'aside', 'footer', 'header']):
                unwanted.decompose()
            
            # Извлекаем текст из параграфов
            paragraphs = main_content.find_all(['p', 'div', 'span'])
            texts = []
            for p in paragraphs:
                text = p.get_text(strip=True)
                if len(text) > 50:  # Фильтруем короткие тексты
                    texts.append(text)
            
            content = ' '.join(texts)
        
        # Метаданные
        meta_description = ""
        meta_tag = soup.find('meta', {'name': 'description'}) or soup.find('meta', {'property': 'og:description'})
        if meta_tag:
            meta_description = meta_tag.get('content', '')
        
        # Ссылки
        links = []
        parsed_url = urlparse(url)
        base_domain = parsed_url.netloc
        
        for link in soup.find_all('a', href=True):
            href = link['href']
            full_url = urljoin(url, href)
            link_parsed = urlparse(full_url)
            
            # Только ссылки с того же домена
            if link_parsed.netloc == base_domain:
                links.append(full_url)
        
        return {
            'url': url,
            'title': title,
            'content': content,
            'meta_description': meta_description,
            'links': list(set(links))[:40],
            'source': 'generic',
            'language': 'unknown'
        }
    
    def get_delay(self) -> float:
        return 1.5  # Универсальная задержка


class SourceParserManager:
    """Менеджер для выбора подходящего парсера"""
    
    def __init__(self):
        self.parsers = [
            WikipediaParser(),
            HabrParser(),
            StackOverflowRuParser(),
            GenericParser(),  # Должен быть последним как fallback
        ]
        self.logger = logging.getLogger(__name__)
    
    def get_parser(self, url: str) -> BaseSourceParser:
        """
        Находит подходящий парсер для URL
        
        Args:
            url: URL страницы
            
        Returns:
            Парсер для этого URL
        """
        for parser in self.parsers:
            if parser.can_parse(url):
                self.logger.debug(f"Using {parser.__class__.__name__} for {url}")
                return parser
        
        # Fallback на Generic (не должно произойти, т.к. он всегда True)
        return self.parsers[-1]
    
    def get_delay_for_url(self, url: str) -> float:
        """Возвращает рекомендуемую задержку для URL"""
        parser = self.get_parser(url)
        return parser.get_delay()
