#!/usr/bin/env python3
"""
Экспорт данных из MongoDB в формат для C++ индексатора
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.utils.config_loader import ConfigLoader
from src.utils.logger import setup_logger
from src.utils.mongodb_client import MongoDBClient


def export_documents_for_cpp(output_file: str, max_docs: int = None):
    """
    Экспортирует документы из MongoDB в текстовый файл для C++ индексатора
    Формат: один документ на строку (title + content)
    """
    # Загружаем конфигурацию
    config_path = Path(__file__).parent.parent / "config.yaml"
    config = ConfigLoader.load_config(str(config_path))
    logger = setup_logger("export_to_cpp", config)
    
    # Подключаемся к MongoDB
    try:
        db_client = MongoDBClient(str(config_path))
        pages_collection = db_client.get_collection(
            config['mongodb']['collections']['pages']
        )
        
        logger.info("Connected to MongoDB")
        
        # Получаем документы
        query = {}
        if max_docs:
            cursor = pages_collection.find(query).limit(max_docs)
        else:
            cursor = pages_collection.find(query)
        
        # Экспортируем в файл
        output_path = Path(output_file)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        exported_count = 0
        
        with open(output_file, 'w', encoding='utf-8') as f:
            for doc in cursor:
                # Формируем текст документа
                title = doc.get('title', '').strip()
                content = doc.get('content', '').strip()
                
                if not content:
                    continue
                
                # Объединяем title и content
                # C++ индексатор будет индексировать это как один документ
                full_text = f"{title}. {content}".replace('\n', ' ').replace('\r', '')
                
                # Записываем одной строкой
                f.write(full_text + '\n')
                
                exported_count += 1
                
                if exported_count % 100 == 0:
                    logger.info(f"Exported {exported_count} documents...")
        
        logger.info(f"Successfully exported {exported_count} documents to {output_file}")
        return exported_count
        
    except Exception as e:
        logger.error(f"Export failed: {e}")
        raise


def export_documents_with_metadata(output_file: str, metadata_file: str, max_docs: int = None):
    """
    Экспортирует документы с сохранением метаданных
    """
    import json
    
    # Загружаем конфигурацию
    config_path = Path(__file__).parent.parent / "config.yaml"
    config = ConfigLoader.load_config(str(config_path))
    logger = setup_logger("export_to_cpp", config)
    
    # Подключаемся к MongoDB
    try:
        db_client = MongoDBClient(str(config_path))
        pages_collection = db_client.get_collection(
            config['mongodb']['collections']['pages']
        )
        
        logger.info("Connected to MongoDB")
        
        # Получаем документы
        query = {}
        if max_docs:
            cursor = pages_collection.find(query).limit(max_docs)
        else:
            cursor = pages_collection.find(query)
        
        # Создаем директории
        Path(output_file).parent.mkdir(parents=True, exist_ok=True)
        Path(metadata_file).parent.mkdir(parents=True, exist_ok=True)
        
        exported_count = 0
        metadata = []
        
        with open(output_file, 'w', encoding='utf-8') as f:
            for doc in cursor:
                # Формируем текст документа
                title = doc.get('title', '').strip()
                content = doc.get('content', '').strip()
                url = doc.get('url', '')
                
                if not content:
                    continue
                
                # Сохраняем метаданные
                metadata.append({
                    'doc_id': exported_count,
                    'title': title,
                    'url': url,
                    'mongo_id': str(doc.get('_id', ''))
                })
                
                # Объединяем title и content
                full_text = f"{title}. {content}".replace('\n', ' ').replace('\r', '')
                
                # Записываем одной строкой
                f.write(full_text + '\n')
                
                exported_count += 1
                
                if exported_count % 100 == 0:
                    logger.info(f"Exported {exported_count} documents...")
        
        # Сохраняем метаданные
        with open(metadata_file, 'w', encoding='utf-8') as f:
            json.dump(metadata, f, ensure_ascii=False, indent=2)
        
        logger.info(f"Successfully exported {exported_count} documents")
        logger.info(f"Documents: {output_file}")
        logger.info(f"Metadata: {metadata_file}")
        
        return exported_count
        
    except Exception as e:
        logger.error(f"Export failed: {e}")
        raise


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Export MongoDB documents for C++ indexing')
    parser.add_argument('--output', '-o', type=str, default='data/processed/documents.txt',
                       help='Output file for documents')
    parser.add_argument('--metadata', '-m', type=str, default='data/processed/metadata.json',
                       help='Output file for metadata')
    parser.add_argument('--limit', '-l', type=int,
                       help='Maximum number of documents to export')
    parser.add_argument('--simple', action='store_true',
                       help='Simple export without metadata')
    
    args = parser.parse_args()
    
    try:
        if args.simple:
            count = export_documents_for_cpp(args.output, args.limit)
        else:
            count = export_documents_with_metadata(args.output, args.metadata, args.limit)
        
        print(f"\n✅ Exported {count} documents successfully!")
        print(f"\nNext steps:")
        print(f"1. Compile C++ modules: ./scripts/build_cpp.sh")
        print(f"2. Build index: ./bin/index_builder --input {args.output} --output data/indexes/index.bin")
        print(f"3. Search: ./bin/search_engine --index data/indexes/index.bin --interactive")
        
        return 0
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1


if __name__ == '__main__':
    sys.exit(main())
