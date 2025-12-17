"""
Обработчик базы данных для хранения собранных страниц
"""

import logging
from typing import Dict, List, Optional
from datetime import datetime
from bson import ObjectId

from src.utils.mongodb_client import MongoDBClient


class DatabaseHandler:
    """Класс для работы с базой данных страниц"""
    
    def __init__(self, config_path: str = "config.yaml"):
        """
        Инициализация обработчика БД
        
        Args:
            config_path: Путь к файлу конфигурации
        """
        self.config_path = config_path
        self.logger = logging.getLogger(__name__)
        
        # Инициализация клиента MongoDB
        self.mongo_client = MongoDBClient(config_path)
        
        # Получаем конфигурацию
        self.config = self.mongo_client.config
        mongodb_config = self.config.get('mongodb', {})
        
        # Имена коллекций
        self.pages_collection_name = mongodb_config.get('collections', {}).get('pages', 'pages')
        self.index_metadata_collection_name = mongodb_config.get('collections', {}).get('index_metadata', 'index_metadata')
        
        # Создаем индексы
        self._create_indexes()
    
    def _create_indexes(self) -> None:
        """Создает необходимые индексы в коллекциях"""
        try:
            # Индексы для коллекции страниц
            pages_collection = self.mongo_client.get_collection(self.pages_collection_name)
            
            indexes = [
                [('url', 1)],  # Уникальный индекс на URL
                [('title', 'text'), ('content', 'text')],  # Текстовый индекс
                [('crawled_at', -1)],  # Для сортировки по дате
                [('domain', 1)],  # Для фильтрации по домену
                [('status', 1)],  # Для фильтрации по статусу
                [('links', 1)]  # Для поиска по ссылкам
            ]
            
            for index_spec in indexes:
                try:
                    pages_collection.create_index(index_spec)
                except Exception as e:
                    self.logger.warning(f"Failed to create index {index_spec}: {e}")
            
            # Индексы для коллекции метаданных индекса
            metadata_collection = self.mongo_client.get_collection(self.index_metadata_collection_name)
            metadata_collection.create_index([('type', 1)])
            metadata_collection.create_index([('created_at', -1)])
            
            self.logger.info("Database indexes created successfully")
            
        except Exception as e:
            self.logger.error(f"Error creating database indexes: {e}")
    
    def save_page(self, page_data: Dict) -> str:
        """
        Сохраняет страницу в базу данных
        
        Args:
            page_data: Данные страницы
            
        Returns:
            ID сохраненной страницы
        """
        try:
            # Подготавливаем данные для сохранения
            page_doc = self._prepare_page_document(page_data)
            
            # Проверяем, существует ли уже страница с таким URL
            existing_page = self.mongo_client.find_document(
                self.pages_collection_name,
                {'url': page_doc['url']}
            )
            
            if existing_page:
                # Обновляем существующую страницу
                update_result = self.mongo_client.update_document(
                    self.pages_collection_name,
                    {'_id': existing_page['_id']},
                    {'$set': page_doc}
                )
                
                if update_result:
                    self.logger.debug(f"Updated page: {page_doc['url']}")
                    return str(existing_page['_id'])
                else:
                    self.logger.warning(f"Failed to update page: {page_doc['url']}")
                    return None
            else:
                # Сохраняем новую страницу
                page_id = self.mongo_client.insert_document(
                    self.pages_collection_name,
                    page_doc
                )
                
                self.logger.debug(f"Saved new page: {page_doc['url']} (ID: {page_id})")
                return page_id
                
        except Exception as e:
            self.logger.error(f"Error saving page {page_data.get('url', 'unknown')}: {e}")
            return None
    
    def _prepare_page_document(self, page_data: Dict) -> Dict:
        """
        Подготавливает документ страницы для сохранения в БД
        
        Args:
            page_data: Сырые данные страницы
            
        Returns:
            Подготовленный документ
        """
        from urllib.parse import urlparse
        
        # Извлекаем домен из URL
        parsed_url = urlparse(page_data['url'])
        domain = parsed_url.netloc
        
        # Создаем документ
        doc = {
            'url': page_data['url'],
            'domain': domain,
            'title': page_data.get('title', ''),
            'content': page_data.get('content', ''),
            'html_content': page_data.get('html_content', ''),
            'metadata': page_data.get('metadata', {}),
            'links': page_data.get('links', []),
            'encoding': page_data.get('encoding', 'utf-8'),
            'content_type': page_data.get('content_type', ''),
            'content_length': page_data.get('content_length', 0),
            'download_time': page_data.get('download_time', time.time()),
            'status_code': page_data.get('status_code', 0),
            'headers': page_data.get('headers', {}),
            'crawled_at': datetime.utcnow(),
            'updated_at': datetime.utcnow(),
            'status': 'processed',
            'processed': False  # Для отметки о дальнейшей обработке
        }
        
        return doc
    
    def save_pages_batch(self, pages_data: List[Dict]) -> List[str]:
        """
        Сохраняет несколько страниц в базу данных
        
        Args:
            pages_data: Список данных страниц
            
        Returns:
            Список ID сохраненных страниц
        """
        page_ids = []
        
        for page_data in pages_data:
            page_id = self.save_page(page_data)
            if page_id:
                page_ids.append(page_id)
        
        self.logger.info(f"Saved batch of {len(page_ids)} pages")
        return page_ids
    
    def get_page_by_url(self, url: str) -> Optional[Dict]:
        """
        Получает страницу по URL
        
        Args:
            url: URL страницы
            
        Returns:
            Документ страницы или None
        """
        return self.mongo_client.find_document(
            self.pages_collection_name,
            {'url': url}
        )
    
    def get_pages_by_domain(self, domain: str, limit: int = 100, skip: int = 0) -> List[Dict]:
        """
        Получает страницы по домену
        
        Args:
            domain: Домен для поиска
            limit: Максимальное количество документов
            skip: Количество документов для пропуска
            
        Returns:
            Список документов
        """
        return self.mongo_client.find_documents(
            self.pages_collection_name,
            {'domain': domain},
            limit=limit,
            skip=skip
        )
    
    def get_unprocessed_pages(self, limit: int = 100) -> List[Dict]:
        """
        Получает необработанные страницы
        
        Args:
            limit: Максимальное количество документов
            
        Returns:
            Список необработанных документов
        """
        return self.mongo_client.find_documents(
            self.pages_collection_name,
            {'processed': False},
            limit=limit
        )
    
    def mark_page_as_processed(self, page_id: str) -> bool:
        """
        Помечает страницу как обработанную
        
        Args:
            page_id: ID страницы
            
        Returns:
            True если успешно
        """
        try:
            return self.mongo_client.update_document(
                self.pages_collection_name,
                {'_id': ObjectId(page_id)},
                {'$set': {'processed': True, 'updated_at': datetime.utcnow()}}
            )
        except Exception as e:
            self.logger.error(f"Error marking page as processed: {e}")
            return False
    
    def get_page_count(self, query: Dict = None) -> int:
        """
        Получает количество страниц
        
        Args:
            query: Запрос для фильтрации
            
        Returns:
            Количество страниц
        """
        return self.mongo_client.count_documents(
            self.pages_collection_name,
            query
        )
    
    def get_stats(self) -> Dict:
        """
        Получает статистику по страницам
        
        Returns:
            Словарь со статистикой
        """
        stats = {}
        
        try:
            # Общее количество страниц
            stats['total_pages'] = self.get_page_count()
            
            # Количество обработанных страниц
            stats['processed_pages'] = self.get_page_count({'processed': True})
            
            # Количество необработанных страниц
            stats['unprocessed_pages'] = self.get_page_count({'processed': False})
            
            # Количество уникальных доменов
            pipeline = [
                {'$group': {'_id': '$domain'}},
                {'$count': 'unique_domains'}
            ]
            
            result = list(self.mongo_client.get_collection(
                self.pages_collection_name
            ).aggregate(pipeline))
            
            if result:
                stats['unique_domains'] = result[0]['unique_domains']
            else:
                stats['unique_domains'] = 0
            
            # Общий размер контента
            pipeline = [
                {'$group': {'_id': None, 'total_content_length': {'$sum': '$content_length'}}}
            ]
            
            result = list(self.mongo_client.get_collection(
                self.pages_collection_name
            ).aggregate(pipeline))
            
            if result:
                stats['total_content_length_bytes'] = result[0]['total_content_length']
                stats['total_content_length_mb'] = result[0]['total_content_length'] / (1024 * 1024)
            else:
                stats['total_content_length_bytes'] = 0
                stats['total_content_length_mb'] = 0
            
            # Средняя длина контента
            pipeline = [
                {'$group': {'_id': None, 'avg_content_length': {'$avg': '$content_length'}}}
            ]
            
            result = list(self.mongo_client.get_collection(
                self.pages_collection_name
            ).aggregate(pipeline))
            
            if result:
                stats['avg_content_length_bytes'] = result[0]['avg_content_length']
            else:
                stats['avg_content_length_bytes'] = 0
            
            self.logger.debug(f"Database stats: {stats}")
            
        except Exception as e:
            self.logger.error(f"Error getting database stats: {e}")
            stats['error'] = str(e)
        
        return stats
    
    def close(self) -> None:
        """Закрывает соединение с базой данных"""
        self.mongo_client.close()