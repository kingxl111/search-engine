#include "boolean_search.h"
#include <iostream>

int main() {
    std::cout << "=== Simple Search Test ===\n\n";
    
    try {
        // Создаем движок
        search::BooleanSearch engine;
        
        std::cout << "Loading index from data/indexes/index.bin...\n";
        
        // Загружаем индекс
        if (!engine.load_index("data/indexes/index.bin")) {
            std::cerr << "Error: Failed to load index\n";
            return 1;
        }
        
        std::cout << "Index loaded successfully!\n\n";
        
        // Пробуем простой поиск
        std::cout << "Searching for 'test'...\n";
        
        auto result = engine.search("test", 5);
        
        std::cout << "Search completed!\n";
        std::cout << "Found: " << result.total_found << " documents\n";
        std::cout << "Returned: " << result.doc_ids.size() << " documents\n";
        std::cout << "Time: " << result.time_ms << " ms\n";
        
        if (!result.syntax_valid) {
            std::cout << "Error: " << result.error_message << "\n";
        }
        
        // Показываем результаты
        for (size_t i = 0; i < result.doc_ids.size(); ++i) {
            std::cout << "\n" << (i+1) << ". Document #" << result.doc_ids[i] << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n✅ Test completed!\n";
    return 0;
}
