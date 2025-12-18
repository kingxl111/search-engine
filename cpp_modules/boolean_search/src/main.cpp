#include "boolean_search.h"
#include <iostream>
#include <fstream>
#include <cstring>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "\nOptions:\n"
              << "  --index <file>    Index file to load\n"
              << "  --query <query>   Search query\n"
              << "  --interactive     Interactive search mode\n"
              << "  --limit <n>       Maximum number of results (default: 10)\n"
              << "  --stats <file>    Export search statistics\n"
              << "  --help            Show this help message\n"
              << "\nQuery Syntax:\n"
              << "  term              Simple term search\n"
              << "  term1 && term2    AND operator (both terms must be present)\n"
              << "  term1 || term2    OR operator (either term must be present)\n"
              << "  !term             NOT operator (term must not be present)\n"
              << "  (query)           Grouping with parentheses\n"
              << "  \"phrase\"          Phrase search (exact match)\n"
              << "\nExample:\n"
              << "  " << program_name << " --index index.bin --query \"search engine\"\n"
              << "  " << program_name << " --index index.bin --interactive\n";
}

void run_interactive(search::BooleanSearch& search_engine, size_t limit) {
    std::cout << "\n=== Interactive Search Mode ===\n";
    std::cout << "Enter queries (or 'quit' to exit):\n\n";
    
    while (true) {
        std::cout << "Query> ";
        std::cout.flush();
        
        ds::String query;
        char ch;
        while (std::cin.get(ch)) {
            if (ch == '\n') {
                break;
            }
            query.push_back(ch);
        }
        
        query = query.trim();
        
        if (query.empty()) {
            continue;
        }
        
        if (query == "quit" || query == "exit") {
            break;
        }
        
        // Выполняем поиск
        auto result = search_engine.search(query, limit);
        
        if (!result.syntax_valid) {
            std::cerr << "Error: " << result.error_message << "\n\n";
            continue;
        }
        
        // Показываем результаты
        std::cout << "\nFound " << result.total_found << " documents ";
        std::cout << "(showing " << result.doc_ids.size() << ")";
        std::cout << " in " << result.time_ms << " ms\n\n";
        
        for (size_t i = 0; i < result.doc_ids.size(); ++i) {
            uint32_t doc_id = result.doc_ids[i];
            const auto* doc = search_engine.get_document(doc_id);
            
            if (doc) {
                std::cout << (i + 1) << ". Document #" << doc_id << "\n";
                std::cout << "   Title: " << doc->title << "\n";
                std::cout << "   URL: " << doc->url << "\n";
                
                // Получаем сниппет
                auto snippet = search_engine.get_snippet(doc_id, query, 10);
                if (!snippet.empty()) {
                    std::cout << "   Snippet: " << snippet << "\n";
                }
                
                std::cout << "\n";
            }
        }
        
        std::cout << "\n";
    }
    
    std::cout << "Goodbye!\n";
}

int main(int argc, char** argv) {
    ds::String index_file;
    ds::String query;
    ds::String stats_file;
    size_t limit = 10;
    bool interactive = false;
    
    // Парсим аргументы командной строки
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--index") == 0 && i + 1 < argc) {
            index_file = argv[++i];
        } else if (std::strcmp(argv[i], "--query") == 0 && i + 1 < argc) {
            query = argv[++i];
        } else if (std::strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            limit = std::stoul(argv[++i]);
        } else if (std::strcmp(argv[i], "--stats") == 0 && i + 1 < argc) {
            stats_file = argv[++i];
        } else if (std::strcmp(argv[i], "--interactive") == 0) {
            interactive = true;
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
    if (index_file.empty()) {
        std::cerr << "Error: Index file is required\n";
        print_usage(argv[0]);
        return 1;
    }
    
    if (!interactive && query.empty()) {
        std::cerr << "Error: Query is required (or use --interactive)\n";
        print_usage(argv[0]);
        return 1;
    }
    
    std::cout << "=== Boolean Search Engine ===\n\n";
    std::cout << "Loading index from " << index_file << "...\n";
    
    try {
        // Создаем поисковый движок
        search::BooleanSearch search_engine;
        
        // Загружаем индекс
        if (!search_engine.load_index(index_file)) {
            std::cerr << "Error: Failed to load index from " << index_file << "\n";
            return 1;
        }
        
        std::cout << "Index loaded successfully!\n\n";
        
        // Интерактивный режим
        if (interactive) {
            run_interactive(search_engine, limit);
        } else {
            // Разовый поиск
            std::cout << "Query: " << query << "\n";
            std::cout << "Searching...\n\n";
            
            auto result = search_engine.search(query, limit);
            
            if (!result.syntax_valid) {
                std::cerr << "Error: " << result.error_message << "\n";
                return 1;
            }
            
            // Показываем результаты
            std::cout << "Found " << result.total_found << " documents ";
            std::cout << "(showing " << result.doc_ids.size() << ")";
            std::cout << " in " << result.time_ms << " ms\n\n";
            
            for (size_t i = 0; i < result.doc_ids.size(); ++i) {
                uint32_t doc_id = result.doc_ids[i];
                const auto* doc = search_engine.get_document(doc_id);
                
                if (doc) {
                    std::cout << (i + 1) << ". Document #" << doc_id << "\n";
                    std::cout << "   Title: " << doc->title << "\n";
                    std::cout << "   URL: " << doc->url << "\n";
                    
                    // Получаем сниппет
                    auto snippet = search_engine.get_snippet(doc_id, query, 10);
                    if (!snippet.empty()) {
                        std::cout << "   Snippet: " << snippet << "\n";
                    }
                    
                    std::cout << "\n";
                }
            }
        }
        
        // Экспортируем статистику
        if (!stats_file.empty()) {
            std::cout << "\nExporting statistics to " << stats_file << "...\n";
            if (!search_engine.export_stats(stats_file)) {
                std::cerr << "Warning: Failed to export statistics\n";
            } else {
                std::cout << "Statistics exported successfully!\n";
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
