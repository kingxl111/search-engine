"""
Калькулятор статистических метрик для анализа текста
"""

import numpy as np
from typing import List, Dict
import math


class StatisticsCalculator:
    """Класс для расчета статистических метрик"""
    
    @staticmethod
    def calculate_entropy(frequencies: List[int]) -> float:
        """
        Рассчитывает энтропию распределения
        
        Args:
            frequencies: Список частот
            
        Returns:
            Энтропия в битах
        """
        if not frequencies:
            return 0.0
        
        total = sum(frequencies)
        if total == 0:
            return 0.0
        
        probs = [f / total for f in frequencies]
        entropy = -sum(p * math.log2(p) for p in probs if p > 0)
        return entropy
    
    @staticmethod
    def calculate_gini_coefficient(frequencies: List[int]) -> float:
        """
        Рассчитывает коэффициент Джини для распределения
        
        Args:
            frequencies: Список частот
            
        Returns:
            Коэффициент Джини (0-1)
        """
        if len(frequencies) <= 1:
            return 0.0
        
        sorted_freqs = sorted(frequencies)
        n = len(sorted_freqs)
        total = sum(sorted_freqs)
        
        if total == 0:
            return 0.0
        
        cumulative = 0
        for i, freq in enumerate(sorted_freqs, 1):
            cumulative += (2 * i - n - 1) * freq
        
        gini = cumulative / (n * total)
        return gini
    
    @staticmethod
    def calculate_zipf_mandelbrot(frequencies: List[int], 
                                 a: float = 1.0, 
                                 b: float = 2.7) -> List[float]:
        """
        Рассчитывает ожидаемые частоты по закону Мандельброта
        
        Args:
            frequencies: Список частот
            a, b: Параметры закона Мандельброта
            
        Returns:
            Ожидаемые частоты
        """
        n = len(frequencies)
        total = sum(frequencies)
        
        expected = []
        for r in range(1, n + 1):
            expected_freq = total / ((r + b) ** a)
            expected.append(expected_freq)
        
        return expected
    
    @staticmethod
    def calculate_heaps_law(N: int, k: float = 10, beta: float = 0.5) -> float:
        """
        Рассчитывает ожидаемый размер словаря по закону Хипса
        
        Args:
            N: Количество обработанных токенов
            k, beta: Параметры закона Хипса
            
        Returns:
            Ожидаемый размер словаря
        """
        return k * (N ** beta)