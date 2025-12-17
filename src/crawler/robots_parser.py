"""
Парсер файлов robots.txt для соблюдения правил веб-сайтов
"""

import re
import logging
from typing import Set, List, Optional
from urllib.parse import urlparse
import requests
from time import sleep


class RobotsParser:
    """Класс для парсинга и анализа robots.txt"""
    
    def __init__(self, user_agent: str = "SearchEngineBot"):
        """
        Инициализация парсера
        
        Args:
            user_agent: Имя пользовательского агента для проверки правил
        """
        self.user_agent = user_agent
        self.rules_cache = {}  # Кэш правил для доменов
        self.logger = logging.getLogger(__name__)
    
    def fetch_robots_txt(self, url: str) -> Optional[str]:
        """
        Загружает robots.txt с сайта
        
        Args:
            url: URL сайта
            
        Returns:
            Содержимое robots.txt или None в случае ошибки
        """
        try:
            parsed = urlparse(url)
            robots_url = f"{parsed.scheme}://{parsed.netloc}/robots.txt"
            
            response = requests.get(
                robots_url,
                headers={'User-Agent': self.user_agent},
                timeout=10
            )
            
            if response.status_code == 200:
                return response.text
            else:
                self.logger.debug(f"robots.txt not found at {robots_url}, status: {response.status_code}")
                return None
                
        except Exception as e:
            self.logger.warning(f"Error fetching robots.txt for {url}: {e}")
            return None
    
    def parse_robots_txt(self, content: str) -> Dict[str, Set[str]]:
        """
        Парсит содержимое robots.txt
        
        Args:
            content: Содержимое robots.txt
            
        Returns:
            Словарь с правилами для разных User-Agent
        """
        rules = {}
        current_agents = []
        
        for line in content.split('\n'):
            line = line.strip()
            
            # Пропускаем комментарии и пустые строки
            if not line or line.startswith('#'):
                continue
            
            # Парсим директиву
            parts = re.split(r':\s*', line, maxsplit=1)
            if len(parts) != 2:
                continue
            
            directive = parts[0].lower()
            value = parts[1].strip()
            
            if directive == 'user-agent':
                # Новый User-Agent
                agent = value.lower()
                current_agents = [agent]
                if agent not in rules:
                    rules[agent] = set()
            
            elif directive == 'disallow' and current_agents:
                # Правило запрета
                for agent in current_agents:
                    if value and value != '/':
                        rules[agent].add(value)
            
            elif directive == 'allow' and current_agents:
                # Правило разрешения (обрабатываем как исключение)
                pass  # Упрощенная реализация, можно расширить
    
        return rules
    
    def is_allowed(self, url: str, user_agent: str = None) -> bool:
        """
        Проверяет, разрешен ли доступ к URL согласно robots.txt
        
        Args:
            url: URL для проверки
            user_agent: User-Agent (если None, используется self.user_agent)
            
        Returns:
            True если доступ разрешен
        """
        if user_agent is None:
            user_agent = self.user_agent
        
        parsed = urlparse(url)
        domain = parsed.netloc
        
        # Получаем правила из кэша или загружаем
        if domain not in self.rules_cache:
            content = self.fetch_robots_txt(url)
            if content:
                self.rules_cache[domain] = self.parse_robots_txt(content)
            else:
                self.rules_cache[domain] = {}
        
        rules = self.rules_cache[domain]
        
        # Проверяем правила для всех агентов (*) и для нашего агента
        all_agents_rules = rules.get('*', set())
        our_agent_rules = rules.get(user_agent.lower(), set())
        
        # Объединяем правила
        disallowed_paths = all_agents_rules.union(our_agent_rules)
        
        # Проверяем, соответствует ли путь какому-либо запрету
        path = parsed.path
        for disallowed_path in disallowed_paths:
            if path.startswith(disallowed_path):
                return False
        
        return True
    
    def get_crawl_delay(self, url: str, user_agent: str = None) -> Optional[float]:
        """
        Получает рекомендуемую задержку между запросами из robots.txt
        
        Args:
            url: URL сайта
            user_agent: User-Agent
            
        Returns:
            Задержка в секундах или None если не указана
        """
        if user_agent is None:
            user_agent = self.user_agent
        
        parsed = urlparse(url)
        domain = parsed.netloc
        
        # Загружаем robots.txt если еще нет в кэше
        if domain not in self.rules_cache:
            content = self.fetch_robots_txt(url)
            if content:
                self.rules_cache[domain] = self.parse_robots_txt(content)
            else:
                return None
        
        # Здесь можно добавить парсинг директивы Crawl-delay
        # Для упрощения возвращаем None, используем задержку из конфигурации
        return None