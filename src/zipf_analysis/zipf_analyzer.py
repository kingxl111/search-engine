"""
Анализатор закона Ципфа для корпуса документов
"""

import logging
from typing import Dict, List, Tuple
from collections import Counter
import numpy as np
from pathlib import Path

from src.utils.mongodb_client import MongoDBClient


class ZipfAnalyzer:
    """Класс для анализа распределения терминов по закону Ципфа"""
    
    def __init__(self, config_path: str = "config.yaml"):
        """
        Инициализация анализатора
        
        Args:
            config_path: Путь к файлу конфигурации
        """
        self.config_path = config_path
        self.logger = logging.getLogger(__name__)
        
        # Подключение к MongoDB
        self.mongo_client = MongoDBClient(config_path)
        self.pages_collection = "pages"
        
        # Результаты анализа
        self.frequencies = None
        self.ranks = None
        self.zipf_constants = None
        
    def extract_terms_from_documents(self, limit: int = 10000) -> List[str]:
        """
        Извлекает термины из документов в MongoDB
        
        Args:
            limit: Максимальное количество документов для анализа
            
        Returns:
            Список всех терминов
        """
        self.logger.info(f"Extracting terms from up to {limit} documents...")
        
        all_terms = []
        
        try:
            # Получаем документы
            pages_collection = self.mongo_client.get_collection(self.pages_collection)
            
            # Используем агрегацию для выборки документов
            pipeline = [
                {'$match': {'processed': True}},
                {'$limit': limit},
                {'$project': {'content': 1}}
            ]
            
            documents = list(pages_collection.aggregate(pipeline))
            
            # Простая токенизация (в реальной системе используйте C++ токенизатор)
            for doc in documents:
                content = doc.get('content', '')
                if content:
                    # Базовая токенизация
                    tokens = content.lower().split()
                    # Фильтрация: только слова длиной > 2
                    tokens = [t.strip('.,!?;:"\'()[]{}') for t in tokens if len(t) > 2]
                    all_terms.extend(tokens)
            
            self.logger.info(f"Extracted {len(all_terms)} terms from {len(documents)} documents")
            return all_terms
            
        except Exception as e:
            self.logger.error(f"Error extracting terms: {e}")
            return []
    
    def calculate_frequencies(self, terms: List[str]) -> Dict[str, int]:
        """
        Рассчитывает частоты терминов
        
        Args:
            terms: Список терминов
            
        Returns:
            Словарь {термин: частота}, отсортированный по убыванию частоты
        """
        self.logger.info("Calculating term frequencies...")
        
        # Подсчет частот
        freq_counter = Counter(terms)
        
        # Сортировка по убыванию частоты
        sorted_frequencies = dict(
            sorted(freq_counter.items(), key=lambda x: x[1], reverse=True)
        )
        
        self.frequencies = sorted_frequencies
        return sorted_frequencies
    
    def calculate_ranks(self) -> List[Tuple[int, int]]:
        """
        Рассчитывает ранги и частоты для построения графика
        
        Returns:
            Список кортежей (ранг, частота)
        """
        if not self.frequencies:
            raise ValueError("Frequencies not calculated. Call calculate_frequencies first.")
        
        ranks = []
        frequencies = list(self.frequencies.values())
        
        for i, freq in enumerate(frequencies, 1):
            ranks.append((i, freq))
        
        self.ranks = ranks
        return ranks
    
    def fit_zipf_law(self) -> Dict[str, float]:
        """
        Подгоняет закон Ципфа к данным
        
        Returns:
            Константы закона Ципфа
        """
        if not self.ranks:
            raise ValueError("Ranks not calculated. Call calculate_ranks first.")
        
        ranks = [r for r, _ in self.ranks]
        freqs = [f for _, f in self.ranks]
        
        # Логарифмические значения
        log_ranks = np.log(ranks)
        log_freqs = np.log(freqs)
        
        # Линейная регрессия для log(f) = log(C) - s * log(r)
        # В законе Ципфа s = 1
        coeffs = np.polyfit(log_ranks, log_freqs, 1)
        
        # Коэффициенты: intercept = log(C), slope = -s
        log_C = coeffs[1]
        s = -coeffs[0]
        
        C = np.exp(log_C)
        
        zipf_constants = {
            'C': C,
            's': s,
            'log_C': log_C,
            'slope': coeffs[0],
            'intercept': coeffs[1],
            'r_squared': self._calculate_r_squared(log_ranks, log_freqs, coeffs)
        }
        
        self.zipf_constants = zipf_constants
        return zipf_constants
    
    def _calculate_r_squared(self, x, y, coeffs):
        """Вычисляет R² для оценки качества подгонки"""
        p = np.poly1d(coeffs)
        y_pred = p(x)
        y_mean = np.mean(y)
        
        ss_res = np.sum((y - y_pred) ** 2)
        ss_tot = np.sum((y - y_mean) ** 2)
        
        return 1 - (ss_res / ss_tot) if ss_tot != 0 else 0
    
    def calculate_statistics(self) -> Dict:
        """
        Рассчитывает статистику распределения
        
        Returns:
            Словарь со статистикой
        """
        if not self.frequencies:
            raise ValueError("Frequencies not calculated.")
        
        freqs = list(self.frequencies.values())
        
        stats = {
            'total_terms': sum(freqs),
            'unique_terms': len(freqs),
            'max_frequency': max(freqs) if freqs else 0,
            'min_frequency': min(freqs) if freqs else 0,
            'mean_frequency': np.mean(freqs) if freqs else 0,
            'median_frequency': np.median(freqs) if freqs else 0,
            'std_frequency': np.std(freqs) if freqs else 0,
            'vocabulary_growth': self._calculate_vocabulary_growth(freqs)
        }
        
        # Дополнительные метрики
        if len(freqs) >= 10:
            stats['top_10_terms_ratio'] = sum(freqs[:10]) / sum(freqs)
            stats['top_100_terms_ratio'] = sum(freqs[:100]) / sum(freqs)
        
        return stats
    
    def _calculate_vocabulary_growth(self, freqs):
        """Рассчитывает кривую роста словаря"""
        if not freqs:
            return []
        
        growth = []
        unique_seen = set()
        
        # Для простоты используем кумулятивную сумму частот
        total = 0
        for i, freq in enumerate(freqs, 1):
            total += freq
            growth.append({
                'terms_processed': total,
                'vocabulary_size': i,
                'ratio': i / total if total > 0 else 0
            })
        
        return growth[:100]  # Ограничиваем для графика
    
    def save_results(self, output_dir: str = "reports/zipf"):
        """
        Сохраняет результаты анализа
        
        Args:
            output_dir: Директория для сохранения результатов
        """
        Path(output_dir).mkdir(parents=True, exist_ok=True)
        
        # Сохраняем частоты в CSV
        if self.frequencies:
            csv_path = Path(output_dir) / "term_frequencies.csv"
            with open(csv_path, 'w', encoding='utf-8') as f:
                f.write("rank,term,frequency\n")
                for i, (term, freq) in enumerate(self.frequencies.items(), 1):
                    f.write(f"{i},{term},{freq}\n")
            self.logger.info(f"Frequencies saved to {csv_path}")
        
        # Сохраняем константы закона Ципфа
        if self.zipf_constants:
            json_path = Path(output_dir) / "zipf_constants.json"
            import json
            with open(json_path, 'w', encoding='utf-8') as f:
                json.dump(self.zipf_constants, f, indent=2, ensure_ascii=False)
            self.logger.info(f"Zipf constants saved to {json_path}")
    
    def close(self):
        """Закрывает соединения"""
        self.mongo_client.close()