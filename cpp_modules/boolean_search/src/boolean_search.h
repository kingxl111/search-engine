#ifndef BOOLEAN_SEARCH_H
#define BOOLEAN_SEARCH_H

#include "query_parser.h"
#include "query_evaluator.h"
#include "../../boolean_index/src/inverted_index.h"
#include "../../data_structures/vector.h"
#include <memory>
#include <chrono>

namespace search {

// Результат поиска
struct SearchResult {
    ds::Vector<uint32_t> doc_ids;          // ID найденных документов
    ds::Vector<double> scores;             // Релевантность
    size_t total_found;                    // Всего найдено
    size_t time_ms;                        // Время поиска в мс
    ds::String query;                      // Исходный запрос
    bool syntax_valid;                     // Корректен ли синтаксис
    ds::String error_message;              // Сообщение об ошибке
    
    SearchResult() : total_found(0), time_ms(0), syntax_valid(true) {}
};

// Класс булева поиска
class BooleanSearch {
private:
    std::unique_ptr<InvertedIndex> index_;
    std::unique_ptr<QueryParser> parser_;
    std::unique_ptr<QueryEvaluator> evaluator_;
    
    // Статистика поиска
    struct SearchStats {
        size_t total_queries;
        size_t successful_queries;
        size_t failed_queries;
        size_t total_time_ms;
        ds::Vector<size_t> query_times;
        
        SearchStats() : total_queries(0), successful_queries(0), 
                       failed_queries(0), total_time_ms(0) {}
        
        void add_query(bool success, size_t time_ms) {
            total_queries++;
            if (success) successful_queries++;
            else failed_queries++;
            total_time_ms += time_ms;
            query_times.push_back(time_ms);
        }
        
        double get_average_time() const {
            return total_queries > 0 ? static_cast<double>(total_time_ms) / total_queries : 0.0;
        }
    };
    
    SearchStats stats_;
    
public:
    // Конструкторы
    BooleanSearch();
    explicit BooleanSearch(std::unique_ptr<InvertedIndex> index);
    
    // Деструктор
    ~BooleanSearch() = default;
    
    // Загрузка индекса
    bool load_index(const ds::String& index_file);
    
    // Поиск по запросу
    SearchResult search(const ds::String& query, size_t limit = 100);
    
    // Пакетный поиск
    ds::Vector<SearchResult> batch_search(const ds::Vector<ds::String>& queries, size_t limit = 100);
    
    // Проверка синтаксиса запроса
    bool validate_query(const ds::String& query) const;
    
    // Получение информации о запросе (термины, сложность)
    struct QueryInfo {
        ds::String original_query;
        ds::Vector<ds::String> terms;
        size_t complexity;
        bool is_valid;
        ds::String parse_tree;
    };
    
    QueryInfo analyze_query(const ds::String& query) const;
    
    // Получение статистики поиска
    const SearchStats& get_stats() const { return stats_; }
    
    // Сброс статистики
    void reset_stats();
    
    // Экспорт статистики
    bool export_stats(const ds::String& filepath) const;
    
    // Подсказки по запросу (автодополнение терминов)
    ds::Vector<ds::String> suggest_terms(const ds::String& prefix, size_t max_suggestions = 10) const;
    
    // Поиск похожих документов (по общим терминам)
    ds::Vector<uint32_t> find_similar(uint32_t doc_id, size_t max_results = 10);
    
    // Получение документа по ID
    const Document* get_document(uint32_t doc_id) const;
    
    // Получение сниппета (фрагмента текста с выделением найденных терминов)
    ds::String get_snippet(uint32_t doc_id, const ds::String& query, size_t context_words = 10);
    
private:
    // Инициализация парсера и оценщика
    void initialize();
    
    // Извлечение терминов для сниппета
    ds::Vector<ds::String> extract_query_terms(const ds::String& query) const;
};

} // namespace search

#endif // BOOLEAN_SEARCH_H