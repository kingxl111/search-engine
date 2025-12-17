#!/usr/bin/env python3
"""
Скрипт для запуска анализа закона Ципфа
"""

import argparse
import sys
from pathlib import Path

# Добавляем корневую директорию в путь для импорта
sys.path.insert(0, str(Path(__file__).parent.parent))

from src.zipf_analysis.zipf_analyzer import ZipfAnalyzer
from src.zipf_analysis.visualizer import Visualizer
from src.zipf_analysis.statistics_calculator import StatisticsCalculator
from src.utils.logger import setup_logger


def main():
    parser = argparse.ArgumentParser(description='Анализ закона Ципфа для корпуса документов')
    parser.add_argument('--limit', type=int, default=10000,
                       help='Максимальное количество документов для анализа')
    parser.add_argument('--top-terms', type=int, default=50,
                       help='Количество топ-терминов для отображения')
    parser.add_argument('--output', type=str, default='reports/zipf',
                       help='Директория для сохранения результатов')
    parser.add_argument('--config', type=str, default='config.yaml',
                       help='Путь к файлу конфигурации')
    
    args = parser.parse_args()
    
    # Настройка логирования
    logger = setup_logger('zipf_analysis')
    logger.info(f"Starting Zipf analysis with limit={args.limit}")
    
    try:
        # Инициализация анализатора
        analyzer = ZipfAnalyzer(args.config)
        
        # 1. Извлечение терминов
        logger.info("Step 1: Extracting terms from documents...")
        terms = analyzer.extract_terms_from_documents(args.limit)
        
        if not terms:
            logger.error("No terms extracted. Exiting.")
            return 1
        
        # 2. Расчет частот
        logger.info("Step 2: Calculating frequencies...")
        frequencies = analyzer.calculate_frequencies(terms)
        
        # 3. Расчет рангов
        logger.info("Step 3: Calculating ranks...")
        ranks = analyzer.calculate_ranks()
        
        # 4. Подгонка закона Ципфа
        logger.info("Step 4: Fitting Zipf law...")
        zipf_constants = analyzer.fit_zipf_law()
        
        logger.info(f"Zipf constants: C={zipf_constants['C']:.2f}, "
                   f"s={zipf_constants['s']:.2f}, "
                   f"R²={zipf_constants['r_squared']:.3f}")
        
        # 5. Расчет статистики
        logger.info("Step 5: Calculating statistics...")
        stats = analyzer.calculate_statistics()
        
        logger.info(f"Statistics: {stats['unique_terms']} unique terms, "
                   f"{stats['total_terms']} total terms")
        logger.info(f"Top 10 terms cover {stats.get('top_10_terms_ratio', 0)*100:.1f}% of all terms")
        
        # 6. Визуализация
        logger.info("Step 6: Creating visualizations...")
        visualizer = Visualizer(args.output)
        
        # График закона Ципфа
        visualizer.plot_zipf_law(ranks[:1000], zipf_constants)
        
        # График топ-терминов
        visualizer.plot_rank_frequency(frequencies, args.top_terms)
        
        # График роста словаря
        if stats.get('vocabulary_growth'):
            visualizer.plot_vocabulary_growth(stats['vocabulary_growth'])
        
        # 7. Дополнительный анализ
        calculator = StatisticsCalculator()
        entropy = calculator.calculate_entropy(list(frequencies.values()))
        gini = calculator.calculate_gini_coefficient(list(frequencies.values()))
        
        logger.info(f"Additional metrics: Entropy={entropy:.2f} bits, "
                   f"Gini coefficient={gini:.3f}")
        
        # 8. Сохранение результатов
        logger.info("Step 7: Saving results...")
        analyzer.save_results(args.output)
        
        # Создание отчета
        report_path = Path(args.output) / "zipf_report.txt"
        with open(report_path, 'w', encoding='utf-8') as f:
            f.write("=== Zipf Law Analysis Report ===\n\n")
            f.write(f"Documents analyzed: {args.limit}\n")
            f.write(f"Total terms: {stats['total_terms']}\n")
            f.write(f"Unique terms: {stats['unique_terms']}\n")
            f.write(f"Type-token ratio: {stats['unique_terms']/stats['total_terms']:.4f}\n")
            f.write(f"\n--- Zipf Law Constants ---\n")
            f.write(f"C (constant): {zipf_constants['C']:.2f}\n")
            f.write(f"s (exponent): {zipf_constants['s']:.2f}\n")
            f.write(f"R² (goodness of fit): {zipf_constants['r_squared']:.3f}\n")
            f.write(f"\n--- Statistical Metrics ---\n")
            f.write(f"Entropy: {entropy:.2f} bits\n")
            f.write(f"Gini coefficient: {gini:.3f}\n")
            f.write(f"Mean frequency: {stats['mean_frequency']:.2f}\n")
            f.write(f"Median frequency: {stats['median_frequency']:.2f}\n")
            f.write(f"Std frequency: {stats['std_frequency']:.2f}\n")
            
            if 'top_10_terms_ratio' in stats:
                f.write(f"\n--- Concentration Analysis ---\n")
                f.write(f"Top 10 terms cover: {stats['top_10_terms_ratio']*100:.1f}%\n")
                f.write(f"Top 100 terms cover: {stats['top_100_terms_ratio']*100:.1f}%\n")
        
        logger.info(f"Report saved to {report_path}")
        logger.info("Zipf analysis completed successfully!")
        
        # Закрытие соединений
        analyzer.close()
        
        return 0
        
    except Exception as e:
        logger.error(f"Error during Zipf analysis: {e}", exc_info=True)
        return 1


if __name__ == '__main__':
    sys.exit(main())