"""
Модуль для настройки логирования приложения
"""

import logging
import sys
from pathlib import Path
from logging.handlers import RotatingFileHandler
import colorlog


class LoggerSetup:
    """Класс для настройки системы логирования"""
    
    @staticmethod
    def setup_logger(name: str, config: dict = None) -> logging.Logger:
        """
        Настраивает и возвращает логгер
        
        Args:
            name: Имя логгера
            config: Конфигурация логирования
            
        Returns:
            Настроенный логгер
        """
        logger = logging.getLogger(name)
        
        # Если логгер уже настроен, возвращаем его
        if logger.handlers:
            return logger
        
        # Конфигурация по умолчанию
        default_config = {
            'level': 'INFO',
            'format': '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
            'file': {
                'enabled': True,
                'path': 'logs/search_engine.log',
                'max_size_mb': 100,
                'backup_count': 5
            },
            'console': {
                'enabled': True,
                'colors': True
            }
        }
        
        # Объединяем с переданной конфигурацией
        if config:
            # Простая рекурсивная функция для обновления словаря
            def update_dict(d, u):
                for k, v in u.items():
                    if isinstance(v, dict):
                        d[k] = update_dict(d.get(k, {}), v)
                    else:
                        d[k] = v
                return d
            config = update_dict(default_config, config)
        else:
            config = default_config
        
        # Устанавливаем уровень логирования
        log_level = getattr(logging, config['level'].upper(), logging.INFO)
        logger.setLevel(log_level)
        
        # Форматтер для вывода
        if config['console'].get('colors', True):
            # Цветной форматтер для консоли
            console_formatter = colorlog.ColoredFormatter(
                '%(log_color)s%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                datefmt='%Y-%m-%d %H:%M:%S',
                log_colors={
                    'DEBUG': 'cyan',
                    'INFO': 'green',
                    'WARNING': 'yellow',
                    'ERROR': 'red',
                    'CRITICAL': 'red,bg_white',
                }
            )
        else:
            console_formatter = logging.Formatter(config['format'])
        
        file_formatter = logging.Formatter(config['format'])
        
        # Обработчик для консоли
        if config['console'].get('enabled', True):
            console_handler = logging.StreamHandler(sys.stdout)
            console_handler.setFormatter(console_formatter)
            console_handler.setLevel(log_level)
            logger.addHandler(console_handler)
        
        # Обработчик для файла
        if config['file'].get('enabled', True):
            log_path = Path(config['file']['path'])
            log_path.parent.mkdir(parents=True, exist_ok=True)
            
            max_bytes = config['file'].get('max_size_mb', 100) * 1024 * 1024
            backup_count = config['file'].get('backup_count', 5)
            
            file_handler = RotatingFileHandler(
                log_path,
                maxBytes=max_bytes,
                backupCount=backup_count,
                encoding='utf-8'
            )
            file_handler.setFormatter(file_formatter)
            file_handler.setLevel(log_level)
            logger.addHandler(file_handler)
        
        # Отключаем распространение логов на корневой логгер
        logger.propagate = False
        
        return logger


def setup_logger(name: str, config_path: str = "config.yaml") -> logging.Logger:
    """
    Упрощенная функция для настройки логгера
    
    Args:
        name: Имя логгера
        config_path: Путь к файлу конфигурации
        
    Returns:
        Настроенный логгер
    """
    from .config_loader import ConfigLoader
    
    try:
        config = ConfigLoader.load_config(config_path)
        logging_config = config.get('logging', {})
        return LoggerSetup.setup_logger(name, logging_config)
    except Exception:
        # Если не удалось загрузить конфигурацию, используем настройки по умолчанию
        return LoggerSetup.setup_logger(name)


# Создаем глобальный логгер для удобства
logger = setup_logger('search_engine')