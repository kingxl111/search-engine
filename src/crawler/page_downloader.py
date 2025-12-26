"""
Модуль для загрузки и парсинга веб-страниц
"""

import logging
import time
import random
from typing import Dict, Optional, Tuple
from urllib.parse import urlparse
import requests
from bs4 import BeautifulSoup
import re
import chardet

from .source_parsers import SourceParserManager


class PageDownloader:
    """Класс для загрузки и обработки веб-страниц"""
    
    def __init__(self, config: Dict):
        """
        Инициализация загрузчика
        
        Args:
            config: Конфигурация загрузчика
        """
        self.config = config
        self.logger = logging.getLogger(__name__)
        
        # Настройки из конфигурации
        crawler_config = config.get('crawler', {})
        self.user_agent = crawler_config.get('user_agent', 'SearchEngineBot/1.0')
        # self.timeout = crawler_config.get('timeout', 10)
        self.timeout = 10
        self.retry_attempts = crawler_config.get('retry_attempts', 3)
        self.min_delay = crawler_config.get('delay', 1.0) * 0.8
        self.max_delay = crawler_config.get('delay', 1.0) * 1.2
        
        # Сессия requests для сохранения cookies
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': self.user_agent,
            'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
            'Accept-Language': 'ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7',
            'Accept-Encoding': 'gzip, deflate',
            'Connection': 'keep-alive',
        })
        
        # Время последнего запроса для каждого домена
        self.last_request_time = {}
        
        # Менеджер специализированных парсеров
        self.parser_manager = SourceParserManager()
    
    def download_page(self, url: str, robots_parser=None) -> Optional[Dict]:
        """
        Загружает страницу и извлекает контент
        
        Args:
            url: URL страницы
            robots_parser: Парсер robots.txt для проверки разрешений
            
        Returns:
            Словарь с данными страницы или None в случае ошибки
        """
        # Проверяем robots.txt если парсер передан
        if robots_parser and not robots_parser.is_allowed(url):
            self.logger.warning(f"URL not allowed by robots.txt: {url}")
            return None
        
        # Соблюдаем задержку между запросами к одному домену
        self._respect_delay(url)
        
        # Пытаемся загрузить страницу с повторами
        html_content = None
        response = None
        
        for attempt in range(self.retry_attempts):
            try:
                response = self.session.get(
                    url,
                    timeout=self.timeout,
                    allow_redirects=True
                )
                
                # Проверяем статус код
                if response.status_code == 200:
                    html_content = response.content
                    break
                elif response.status_code == 429:  # Too Many Requests
                    wait_time = (attempt + 1) * 30  # Экспоненциальная задержка
                    self.logger.warning(f"Rate limited, waiting {wait_time} seconds")
                    time.sleep(wait_time)
                else:
                    self.logger.warning(f"HTTP {response.status_code} for {url}")
                    return None
                    
            except requests.exceptions.Timeout:
                self.logger.warning(f"Timeout for {url} (attempt {attempt + 1})")
                if attempt == self.retry_attempts - 1:
                    return None
                time.sleep(2 ** attempt)  # Экспоненциальная задержка
            except requests.exceptions.RequestException as e:
                self.logger.warning(f"Request error for {url}: {e}")
                return None
        
        if not html_content:
            return None
        
        # Парсим страницу
        page_data = self._parse_page(url, html_content, response)
        
        return page_data
    
    def _respect_delay(self, url: str) -> None:
        """Соблюдает задержку между запросами к одному домену"""
        parsed = urlparse(url)
        domain = parsed.netloc
        
        current_time = time.time()
        last_time = self.last_request_time.get(domain, 0)
        
        # Получаем рекомендуемую задержку для данного источника
        recommended_delay = self.parser_manager.get_delay_for_url(url)
        min_delay = max(self.min_delay, recommended_delay * 0.8)
        
        # Вычисляем необходимую задержку
        elapsed = current_time - last_time
        if elapsed < min_delay:
            sleep_time = min_delay - elapsed + random.uniform(0, 0.2)
            time.sleep(sleep_time)
        
        # Обновляем время последнего запроса
        self.last_request_time[domain] = time.time()
    
    def _parse_page(self, url: str, html_content: bytes, response: requests.Response) -> Dict:
        """
        Парсит HTML страницу и извлекает данные
        
        Args:
            url: URL страницы
            html_content: HTML контент
            response: Объект ответа
            
        Returns:
            Словарь с данными страницы
        """
        # Определяем кодировку
        encoding = self._detect_encoding(html_content, response)
        
        try:
            # Декодируем контент
            html_text = html_content.decode(encoding, errors='replace')
        except UnicodeDecodeError:
            # Пробуем UTF-8 как запасной вариант
            html_text = html_content.decode('utf-8', errors='replace')
        
        # Парсим HTML
        soup = BeautifulSoup(html_text, 'lxml')
        
        # Получаем специализированный парсер для данного URL
        parser = self.parser_manager.get_parser(url)
        
        try:
            # Используем специализированный парсер
            page_data = parser.parse(url, html_text, soup)
            
            # Добавляем системные поля
            page_data.update({
                'html_content': html_text,
                'encoding': encoding,
                'content_type': response.headers.get('Content-Type', ''),
                'content_length': len(html_content),
                'download_time': time.time(),
                'status_code': response.status_code,
                'headers': dict(response.headers)
            })
            
        except Exception as e:
            self.logger.warning(f"Specialized parser failed for {url}, using fallback: {e}")
            # Fallback на базовый парсинг
            title = self._extract_title(soup)
            text = self._extract_text(soup)
            metadata = self._extract_metadata(soup)
            links = self._extract_links(soup, url)
            
            page_data = {
                'url': url,
                'title': title,
                'content': text,
                'html_content': html_text,
                'metadata': metadata,
                'links': links,
                'encoding': encoding,
                'content_type': response.headers.get('Content-Type', ''),
                'content_length': len(html_content),
                'download_time': time.time(),
                'status_code': response.status_code,
                'headers': dict(response.headers),
                'source': 'fallback',
                'language': 'unknown'
            }
        
        return page_data
    
    def _detect_encoding(self, content: bytes, response: requests.Response) -> str:
        """Определяет кодировку контента"""
        # Сначала проверяем заголовки HTTP
        content_type = response.headers.get('Content-Type', '').lower()
        if 'charset=' in content_type:
            charset = content_type.split('charset=')[-1].strip()
            return charset
        
        # Пробуем определить с помощью chardet
        try:
            result = chardet.detect(content)
            if result['confidence'] > 0.7:
                return result['encoding']
        except:
            pass
        
        # Ищем в meta-тегах
        try:
            # Декодируем как ASCII для поиска meta-тегов
            sample = content[:5000].decode('ascii', errors='ignore')
            soup = BeautifulSoup(sample, 'html.parser')
            meta_charset = soup.find('meta', charset=True)
            if meta_charset:
                return meta_charset['charset']
            
            meta_content = soup.find('meta', attrs={'http-equiv': 'Content-Type'})
            if meta_content and 'charset=' in meta_content.get('content', ''):
                charset = meta_content['content'].split('charset=')[-1].strip()
                return charset
        except:
            pass
        
        # Возвращаем UTF-8 по умолчанию
        return 'utf-8'
    
    def _extract_title(self, soup: BeautifulSoup) -> str:
        """Извлекает заголовок страницы"""
        title_tag = soup.find('title')
        if title_tag:
            return title_tag.get_text(strip=True)
        return ''
    
    def _extract_text(self, soup: BeautifulSoup) -> str:
        """Извлекает основной текст страницы"""
        # Удаляем ненужные элементы
        for element in soup(['script', 'style', 'nav', 'footer', 'header', 'aside']):
            element.decompose()
        
        # Извлекаем текст из основных контейнеров
        text_parts = []
        
        # Проверяем основные контейнеры с контентом
        content_selectors = [
            'article',
            'main',
            '.content',
            '#content',
            '.post-content',
            '.entry-content',
            '.article-content'
        ]
        
        for selector in content_selectors:
            elements = soup.select(selector)
            if elements:
                for element in elements:
                    text = element.get_text(separator=' ', strip=True)
                    if len(text) > 100:  # Минимальная длина для контента
                        text_parts.append(text)
        
        # Если не нашли контент в специфичных контейнерах, берем весь body
        if not text_parts:
            body = soup.find('body')
            if body:
                text = body.get_text(separator=' ', strip=True)
                text_parts.append(text)
        
        # Объединяем и очищаем текст
        full_text = ' '.join(text_parts)
        
        # Удаляем лишние пробелы и переносы строк
        full_text = re.sub(r'\s+', ' ', full_text)
        full_text = re.sub(r'\n+', '\n', full_text)
        
        return full_text.strip()
    
    def _extract_metadata(self, soup: BeautifulSoup) -> Dict[str, str]:
        """Извлекает метаданные страницы"""
        metadata = {}
        
        # Извлекаем meta-теги
        meta_tags = soup.find_all('meta')
        for tag in meta_tags:
            name = tag.get('name') or tag.get('property') or tag.get('http-equiv')
            content = tag.get('content')
            if name and content:
                metadata[name.lower()] = content
        
        # Извлекаем Open Graph данные
        og_tags = soup.find_all('meta', attrs={'property': re.compile(r'^og:')})
        for tag in og_tags:
            prop = tag.get('property', '')
            content = tag.get('content', '')
            if prop and content:
                metadata[prop] = content
        
        return metadata
    
    def _extract_links(self, soup: BeautifulSoup, base_url: str) -> List[str]:
        """Извлекает все ссылки со страницы"""
        links = []
        
        for link_tag in soup.find_all('a', href=True):
            href = link_tag['href']
            
            # Пропускаем пустые ссылки и якоря
            if not href or href.startswith('#'):
                continue
            
            # Пропускаем javascript и mailto ссылки
            if href.startswith(('javascript:', 'mailto:', 'tel:')):
                continue
            
            # Пропускаем ссылки на файлы
            if href.lower().endswith(('.pdf', '.doc', '.docx', '.xls', '.xlsx', 
                                     '.ppt', '.pptx', '.zip', '.rar', '.tar', 
                                     '.gz', '.jpg', '.jpeg', '.png', '.gif')):
                continue
            
            links.append(href)
        
        return links
    
    def close(self):
        """Закрывает сессию requests"""
        self.session.close()