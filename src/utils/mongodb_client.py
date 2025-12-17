"""
Клиент для работы с MongoDB
"""

import logging
from typing import Dict, List, Any, Optional
from datetime import datetime
from pymongo import MongoClient, errors
from pymongo.collection import Collection
from pymongo.database import Database

from .config_loader import ConfigLoader


class MongoDBClient:
    """Класс для управления подключением и операциями с MongoDB"""
    
    def __init__(self, config_path: str = "config.yaml"):
        """
        Инициализация клиента MongoDB
        
        Args:
            config_path: Путь к файлу конфигурации
        """
        self.config = ConfigLoader.load_config(config_path)
        self.mongodb_config = self.config.get('mongodb', {})
        self.uri = ConfigLoader.get_mongodb_uri(self.config)
        
        self.client: Optional[MongoClient] = None
        self.db: Optional[Database] = None
        self.logger = logging.getLogger(__name__)
        
        self._connect()
    
    def _connect(self) -> None:
        """Устанавливает соединение с MongoDB"""
        try:
            self.logger.info(f"Connecting to MongoDB at {self.mongodb_config.get('host', 'localhost')}")
            
            self.client = MongoClient(
                self.uri,
                serverSelectionTimeoutMS=5000,
                connectTimeoutMS=10000,
                socketTimeoutMS=30000
            )
            
            # Тестируем подключение
            self.client.admin.command('ping')
            
            database_name = self.mongodb_config.get('database', 'search_engine_db')
            self.db = self.client[database_name]
            
            self.logger.info("Successfully connected to MongoDB")
            
        except errors.ServerSelectionTimeoutError as e:
            self.logger.error(f"Cannot connect to MongoDB: {e}")
            raise
        except errors.ConnectionFailure as e:
            self.logger.error(f"Connection failed: {e}")
            raise
        except Exception as e:
            self.logger.error(f"Unexpected error: {e}")
            raise
    
    def get_collection(self, collection_name: str) -> Collection:
        """
        Получает коллекцию по имени
        
        Args:
            collection_name: Имя коллекции
            
        Returns:
            Объект коллекции MongoDB
        """
        if not self.db:
            raise ConnectionError("Database connection not established")
        
        return self.db[collection_name]
    
    def insert_document(self, collection_name: str, document: Dict[str, Any]) -> str:
        """
        Вставляет документ в коллекцию
        
        Args:
            collection_name: Имя коллекции
            document: Документ для вставки
            
        Returns:
            ID вставленного документа
        """
        collection = self.get_collection(collection_name)
        
        # Добавляем метаданные
        document['created_at'] = datetime.utcnow()
        document['updated_at'] = datetime.utcnow()
        
        result = collection.insert_one(document)
        return str(result.inserted_id)
    
    def insert_many_documents(self, collection_name: str, documents: List[Dict[str, Any]]) -> List[str]:
        """
        Вставляет несколько документов в коллекцию
        
        Args:
            collection_name: Имя коллекции
            documents: Список документов
            
        Returns:
            Список ID вставленных документов
        """
        collection = self.get_collection(collection_name)
        
        # Добавляем метаданные
        for doc in documents:
            doc['created_at'] = datetime.utcnow()
            doc['updated_at'] = datetime.utcnow()
        
        result = collection.insert_many(documents)
        return [str(id) for id in result.inserted_ids]
    
    def find_document(self, collection_name: str, query: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """
        Находит один документ по запросу
        
        Args:
            collection_name: Имя коллекции
            query: Запрос для поиска
            
        Returns:
            Найденный документ или None
        """
        collection = self.get_collection(collection_name)
        return collection.find_one(query)
    
    def find_documents(self, collection_name: str, query: Dict[str, Any], 
                       limit: int = 0, skip: int = 0) -> List[Dict[str, Any]]:
        """
        Находит несколько документов по запросу
        
        Args:
            collection_name: Имя коллекции
            query: Запрос для поиска
            limit: Максимальное количество документов
            skip: Количество документов для пропуска
            
        Returns:
            Список найденных документов
        """
        collection = self.get_collection(collection_name)
        cursor = collection.find(query).skip(skip).limit(limit)
        return list(cursor)
    
    def update_document(self, collection_name: str, query: Dict[str, Any], 
                        update: Dict[str, Any]) -> bool:
        """
        Обновляет документ
        
        Args:
            collection_name: Имя коллекции
            query: Запрос для поиска документа
            update: Обновления
            
        Returns:
            True если документ был обновлен
        """
        collection = self.get_collection(collection_name)
        
        # Добавляем метаданные
        update['$set'] = update.get('$set', {})
        update['$set']['updated_at'] = datetime.utcnow()
        
        result = collection.update_one(query, update)
        return result.modified_count > 0
    
    def delete_document(self, collection_name: str, query: Dict[str, Any]) -> bool:
        """
        Удаляет документ
        
        Args:
            collection_name: Имя коллекции
            query: Запрос для поиска документа
            
        Returns:
            True если документ был удален
        """
        collection = self.get_collection(collection_name)
        result = collection.delete_one(query)
        return result.deleted_count > 0
    
    def count_documents(self, collection_name: str, query: Optional[Dict[str, Any]] = None) -> int:
        """
        Подсчитывает количество документов в коллекции
        
        Args:
            collection_name: Имя коллекции
            query: Запрос для фильтрации
            
        Returns:
            Количество документов
        """
        collection = self.get_collection(collection_name)
        if query is None:
            query = {}
        return collection.count_documents(query)
    
    def create_index(self, collection_name: str, index_spec: List[tuple]) -> str:
        """
        Создает индекс в коллекции
        
        Args:
            collection_name: Имя коллекции
            index_spec: Спецификация индекса
            
        Returns:
            Имя созданного индекса
        """
        collection = self.get_collection(collection_name)
        return collection.create_index(index_spec)
    
    def get_collection_stats(self, collection_name: str) -> Dict[str, Any]:
        """
        Получает статистику коллекции
        
        Args:
            collection_name: Имя коллекция
            
        Returns:
            Статистика коллекции
        """
        collection = self.get_collection(collection_name)
        return collection.stats()
    
    def close(self) -> None:
        """Закрывает соединение с MongoDB"""
        if self.client:
            self.client.close()
            self.logger.info("MongoDB connection closed")
    
    def __enter__(self):
        """Контекстный менеджер для автоматического закрытия"""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Контекстный менеджер для автоматического закрытия"""
        self.close()