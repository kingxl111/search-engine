#include "index_builder.h"
#include "inverted_index.h"
#include <iostream>
#include <fstream>
#include <cstring>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "\nOptions:\n"
              << "  --input <file>    Input text file (one document per line)\n"
              << "  --output <file>   Output index file\n"
              << "  --stats <file>    Export statistics to file\n"
              << "  --export <file>   Export index to text format\n"
              << "  --help            Show this help message\n"
              << "\nExample:\n"
              << "  " << program_name << " --input docs.txt --output index.bin\n";
}

int main(int argc, char** argv) {
    ds::String input_file;
    ds::String output_file;
    ds::String stats_file;
    ds::String export_file;
    
    // Парсим аргументы командной строки
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            input_file = argv[++i];
        } else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (std::strcmp(argv[i], "--stats") == 0 && i + 1 < argc) {
            stats_file = argv[++i];
        } else if (std::strcmp(argv[i], "--export") == 0 && i + 1 < argc) {
            export_file = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Проверяем обязательные параметры
    if (input_file.empty()) {
        std::cerr << "Error: Input file is required\n";
        print_usage(argv[0]);
        return 1;
    }
    
    if (output_file.empty()) {
        std::cerr << "Error: Output file is required\n";
        print_usage(argv[0]);
        return 1;
    }
    
    std::cout << "=== Boolean Index Builder ===\n\n";
    std::cout << "Input file: " << input_file << "\n";
    std::cout << "Output file: " << output_file << "\n\n";
    
    try {
        // Создаем индекс-билдер
        search::IndexBuilder builder;
        
        std::cout << "Building index from file...\n";
        
        // Строим индекс из файла
        if (!builder.build_from_text_file(input_file)) {
            std::cerr << "Error: Failed to build index from " << input_file << "\n";
            return 1;
        }
        
        std::cout << "\nIndex built successfully!\n\n";
        
        // Оптимизируем индекс
        std::cout << "Optimizing index...\n";
        builder.optimize_index();
        
        // Получаем индекс
        auto index = builder.get_index();
        
        if (!index) {
            std::cerr << "Error: Failed to get index\n";
            return 1;
        }
        
        // Сохраняем индекс
        std::cout << "Saving index to " << output_file << "...\n";
        if (!index->save_to_file(output_file)) {
            std::cerr << "Error: Failed to save index to " << output_file << "\n";
            return 1;
        }
        
        std::cout << "Index saved successfully!\n\n";
        
        // Экспортируем статистику
        if (!stats_file.empty()) {
            std::cout << "Exporting statistics to " << stats_file << "...\n";
            if (!builder.export_stats(stats_file)) {
                std::cerr << "Warning: Failed to export statistics\n";
            } else {
                std::cout << "Statistics exported successfully!\n";
            }
        }
        
        // Экспортируем индекс в текстовый формат
        if (!export_file.empty()) {
            std::cout << "Exporting index to text format: " << export_file << "...\n";
            if (!index->export_to_text(export_file)) {
                std::cerr << "Warning: Failed to export index to text format\n";
            } else {
                std::cout << "Index exported successfully!\n";
            }
        }
        
        // Показываем статистику
        const auto& stats = index->get_stats();
        std::cout << "\n=== Index Statistics ===\n";
        std::cout << "Total documents: " << stats.total_documents << "\n";
        std::cout << "Total terms: " << stats.total_terms << "\n";
        std::cout << "Total postings: " << stats.total_postings << "\n";
        std::cout << "Average document length: " << stats.avg_document_length << " terms\n";
        std::cout << "Average term frequency: " << stats.avg_term_frequency << "\n";
        std::cout << "Most frequent term: '" << stats.most_frequent_term 
                  << "' (in " << stats.most_frequent_term_count << " documents)\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
