"""
Модуль для загрузки и управления конфигурацией приложения
"""

import os
import yaml
from typing import Dict, Any
from pathlib import Path
from dotenv import load_dotenv


class ConfigLoader:
    """Класс для загрузки конфигурации из YAML файла с поддержкой переменных окружения"""
    
    @staticmethod
    def load_config(config_path: str = "config.yaml") -> Dict[str, Any]:
        """
        Загружает конфигурацию из YAML файла
        
        Args:
            config_path: Путь к файлу конфигурации
            
        Returns:
            Словарь с конфигурацией
        """
        # Загружаем переменные окружения из .env файла
        env_path = Path(__file__).parent.parent.parent / '.env'
        if env_path.exists():
            load_dotenv(dotenv_path=env_path)
            print(f"Загружены переменные из {env_path}")
        else:
            # Пробуем загрузить из текущей директории
            load_dotenv()
            print(" .env файл не найден, используются системные переменные")
        
        config_path = Path(config_path)
        
        if not config_path.exists():
            raise FileNotFoundError(f"Config file not found: {config_path}")
        
        with open(config_path, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        
        # Обрабатываем переменные окружения в конфигурации
        config = ConfigLoader._resolve_env_vars(config)
        
        # Создаем необходимые директории
        ConfigLoader._create_directories(config)
        
        return config
    
    @staticmethod
    def _resolve_env_vars(config: Dict[str, Any]) -> Dict[str, Any]:
        """
        Рекурсивно заменяет переменные окружения в значениях конфигурации
        
        Формат: ${VAR_NAME:default_value}
        """
        if isinstance(config, dict):
            return {k: ConfigLoader._resolve_env_vars(v) for k, v in config.items()}
        elif isinstance(config, list):
            return [ConfigLoader._resolve_env_vars(item) for item in config]
        elif isinstance(config, str):
            if config.startswith('${') and '}' in config:
                # Извлекаем имя переменной и значение по умолчанию
                var_part = config[2:config.find('}')]
                if ':' in var_part:
                    var_name, default_value = var_part.split(':', 1)
                else:
                    var_name, default_value = var_part, None
                
                # Получаем значение из переменных окружения
                env_value = os.environ.get(var_name)
                if env_value is not None:
                    print(f"  ↳ Заменяем ${var_name} = '{env_value}'")
                    return env_value
                elif default_value is not None:
                    print(f"  ↳ ${var_name} не задана, используем значение по умолчанию: '{default_value}'")
                    return default_value
                else:
                    # Если переменная обязательна, создаем ее из .env.example
                    example_file = Path('.env.example')
                    if example_file.exists():
                        with open(example_file, 'r') as f:
                            for line in f:
                                if line.startswith(var_name + '='):
                                    example_value = line.split('=', 1)[1].strip()
                                    os.environ[var_name] = example_value
                                    print(f"  ⚠️  ${var_name} взята из .env.example: '{example_value}'")
                                    return example_value
                    
                    raise ValueError(f"Environment variable {var_name} not set and no default provided. "
                                   f"Please check your .env file")
        return config
    
    @staticmethod
    def _create_directories(config: Dict[str, Any]) -> None:
        """Создает необходимые директории из конфигурации"""
        paths = config.get('paths', {})
        
        directories = [
            paths.get('base_dir', '.'),
            paths.get('data', {}).get('raw', './data/raw'),
            paths.get('data', {}).get('processed', './data/processed'),
            paths.get('data', {}).get('indexes', './data/indexes'),
            paths.get('data', {}).get('logs', './data/logs'),
            paths.get('data', {}).get('stopwords', './data/stopwords'),
            paths.get('reports', './reports'),
            paths.get('scripts', './scripts')
        ]
        
        for dir_path in directories:
            Path(dir_path).mkdir(parents=True, exist_ok=True)
    
    @staticmethod
    def save_config(config: Dict[str, Any], config_path: str = "config.yaml") -> None:
        """
        Сохраняет конфигурацию в YAML файл
        
        Args:
            config: Словарь с конфигурацией
            config_path: Путь для сохранения файла
        """
        with open(config_path, 'w', encoding='utf-8') as f:
            yaml.dump(config, f, default_flow_style=False, allow_unicode=True, sort_keys=False)
    
    @staticmethod
    def get_mongodb_uri(config: Dict[str, Any]) -> str:
        """
        Формирует URI подключения к MongoDB из конфигурации
        
        Returns:
            Строка подключения к MongoDB
        """
        mongodb_config = config.get('mongodb', {})
        
        username = mongodb_config.get('username', 'admin')
        password = mongodb_config.get('password', 'admin')
        host = mongodb_config.get('host', 'localhost')
        port = mongodb_config.get('port', 27017)
        database = mongodb_config.get('database', 'search_engine_db')
        
        if username and password:
            return f"mongodb://{username}:{password}@{host}:{port}/{database}?authSource=admin"
        else:
            return f"mongodb://{host}:{port}/{database}"