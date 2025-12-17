"""
Визуализация результатов анализа закона Ципфа
"""

import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
from typing import List, Tuple, Dict
import logging


class Visualizer:
    """Класс для визуализации результатов анализа"""
    
    def __init__(self, output_dir: str = "reports/zipf"):
        """
        Инициализация визуализатора
        
        Args:
            output_dir: Директория для сохранения графиков
        """
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.logger = logging.getLogger(__name__)
        
        # Настройка стиля matplotlib
        plt.style.use('seaborn-v0_8-darkgrid')
        self.colors = plt.cm.Set3(np.linspace(0, 1, 12))
    
    def plot_zipf_law(self, ranks: List[Tuple[int, int]], 
                     zipf_constants: Dict = None,
                     title: str = "Закон Ципфа") -> str:
        """
        Строит график закона Ципфа
        
        Args:
            ranks: Список кортежей (ранг, частота)
            zipf_constants: Константы закона Ципфа
            title: Заголовок графика
            
        Returns:
            Путь к сохраненному файлу
        """
        if not ranks:
            raise ValueError("No data to plot")
        
        ranks_np = np.array([r for r, _ in ranks])
        freqs_np = np.array([f for _, f in ranks])
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
        
        # 1. Обычный масштаб
        ax1.loglog(ranks_np, freqs_np, 'o', markersize=4, alpha=0.7, 
                  color=self.colors[0], label='Наблюдаемые данные')
        
        # Теоретическая кривая Ципфа
        if zipf_constants:
            C = zipf_constants.get('C', freqs_np[0])
            zipf_freqs = C / ranks_np
            ax1.loglog(ranks_np, zipf_freqs, 'r-', linewidth=2, 
                      alpha=0.8, label=f'Закон Ципфа (C={C:.1f})')
            
            # Закон Мандельброта
            try:
                from .statistics_calculator import StatisticsCalculator
                mandelbrot_freqs = StatisticsCalculator.calculate_zipf_mandelbrot(
                    freqs_np.tolist()
                )
                ax1.loglog(ranks_np, mandelbrot_freqs, 'g--', linewidth=2,
                          alpha=0.7, label='Закон Мандельброта')
            except:
                pass
        
        ax1.set_xlabel('Ранг (log scale)', fontsize=12)
        ax1.set_ylabel('Частота (log scale)', fontsize=12)
        ax1.set_title(f'{title} - Логарифмическая шкала', fontsize=14)
        ax1.legend()
        ax1.grid(True, alpha=0.3)
        
        # 2. Линейный масштаб (первые 50 терминов)
        top_n = min(50, len(ranks_np))
        ax2.bar(range(1, top_n + 1), freqs_np[:top_n], 
                color=self.colors[1], alpha=0.7)
        
        if zipf_constants and top_n > 0:
            zipf_top = zipf_constants.get('C', freqs_np[0]) / np.arange(1, top_n + 1)
            ax2.plot(range(1, top_n + 1), zipf_top, 'r-', linewidth=2,
                    label='Теоретическая кривая')
        
        ax2.set_xlabel('Ранг', fontsize=12)
        ax2.set_ylabel('Частота', fontsize=12)
        ax2.set_title(f'{title} - Топ {top_n} терминов', fontsize=14)
        ax2.legend()
        ax2.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        # Сохранение
        output_path = self.output_dir / "zipf_law.png"
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        plt.close()
        
        self.logger.info(f"Zipf law plot saved to {output_path}")
        return str(output_path)
    
    def plot_rank_frequency(self, frequencies: Dict[str, int], 
                           top_n: int = 100) -> str:
        """
        Строит график частот терминов
        
        Args:
            frequencies: Словарь {термин: частота}
            top_n: Количество топ-терминов для отображения
            
        Returns:
            Путь к сохраненному файлу
        """
        if not frequencies:
            raise ValueError("No frequencies data")
        
        # Берем топ-N терминов
        top_terms = list(frequencies.items())[:top_n]
        terms = [t for t, _ in top_terms]
        freqs = [f for _, f in top_terms]
        
        fig, ax = plt.subplots(figsize=(12, 8))
        
        bars = ax.bar(range(len(terms)), freqs, color=self.colors[2], alpha=0.7)
        
        # Добавляем значения на столбцы
        for i, (bar, freq) in enumerate(zip(bars, freqs)):
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{freq:,}', ha='center', va='bottom', fontsize=8)
        
        ax.set_xlabel('Термины', fontsize=12)
        ax.set_ylabel('Частота', fontsize=12)
        ax.set_title(f'Топ {top_n} терминов по частоте', fontsize=14)
        ax.set_xticks(range(len(terms)))
        ax.set_xticklabels(terms, rotation=45, ha='right', fontsize=9)
        
        plt.tight_layout()
        
        output_path = self.output_dir / "top_terms.png"
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        plt.close()
        
        self.logger.info(f"Top terms plot saved to {output_path}")
        return str(output_path)
    
    def plot_vocabulary_growth(self, growth_data: List[Dict]) -> str:
        """
        Строит график роста словаря
        
        Args:
            growth_data: Данные роста словаря
            
        Returns:
            Путь к сохраненному файлу
        """
        if not growth_data:
            return ""
        
        terms_processed = [d['terms_processed'] for d in growth_data]
        vocab_size = [d['vocabulary_size'] for d in growth_data]
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
        
        # 1. Рост словаря
        ax1.plot(terms_processed, vocab_size, 'b-', linewidth=2, marker='o', 
                markersize=4, alpha=0.7)
        ax1.set_xlabel('Обработано терминов', fontsize=12)
        ax1.set_ylabel('Размер словаря', fontsize=12)
        ax1.set_title('Рост словаря', fontsize=14)
        ax1.grid(True, alpha=0.3)
        
        # 2. Логарифмическая шкала
        if terms_processed[-1] > 0:
            log_terms = np.log10(terms_processed)
            log_vocab = np.log10(vocab_size)
            
            ax2.plot(log_terms, log_vocab, 'g-', linewidth=2, marker='s', 
                    markersize=4, alpha=0.7)
            ax2.set_xlabel('log(Обработано терминов)', fontsize=12)
            ax2.set_ylabel('log(Размер словаря)', fontsize=12)
            ax2.set_title('Рост словаря (log scale)', fontsize=14)
            ax2.grid(True, alpha=0.3)
            
            # Подгонка закона Хипса
            try:
                from .statistics_calculator import StatisticsCalculator
                # Оцениваем параметры
                coeffs = np.polyfit(log_terms, log_vocab, 1)
                k = 10 ** coeffs[1]
                beta = coeffs[0]
                
                # Теоретическая кривая
                heap_fit = [StatisticsCalculator.calculate_heaps_law(
                    t, k, beta) for t in terms_processed]
                
                ax1.plot(terms_processed, heap_fit, 'r--', linewidth=2,
                        alpha=0.8, label=f'Закон Хипса (k={k:.1f}, β={beta:.2f})')
                ax1.legend()
            except:
                pass
        
        plt.tight_layout()
        
        output_path = self.output_dir / "vocabulary_growth.png"
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        plt.close()
        
        self.logger.info(f"Vocabulary growth plot saved to {output_path}")
        return str(output_path)
    
    def plot_distribution_comparison(self, actual_freqs: List[int],
                                    zipf_freqs: List[float],
                                    mandelbrot_freqs: List[float] = None) -> str:
        """
        Сравнивает распределения
        
        Args:
            actual_freqs: Фактические частоты
            zipf_freqs: Частоты по закону Ципфа
            mandelbrot_freqs: Частоты по закону Мандельброта
            
        Returns:
            Путь к сохраненному файлу
        """
        fig, ax = plt.subplots(figsize=(10, 6))
        
        x = np.arange(1, len(actual_freqs) + 1)
        
        ax.plot(x, actual_freqs, 'b-', linewidth=2, label='Фактические данные', 
                alpha=0.7)
        ax.plot(x, zipf_freqs, 'r-', linewidth=2, label='Закон Ципфа', 
                alpha=0.7)
        
        if mandelbrot_freqs:
            ax.plot(x, mandelbrot_freqs, 'g-', linewidth=2, 
                   label='Закон Мандельброта', alpha=0.7)
        
        ax.set_xlabel('Ранг', fontsize=12)
        ax.set_ylabel('Частота', fontsize=12)
        ax.set_title('Сравнение распределений', fontsize=14)
        ax.legend()
        ax.grid(True, alpha=0.3)
        ax.set_yscale('log')
        ax.set_xscale('log')
        
        plt.tight_layout()
        
        output_path = self.output_dir / "distribution_comparison.png"
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        plt.close()
        
        return str(output_path)