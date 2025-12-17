#include "boolean_search.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace search {

using namespace std::chrono;

// Конструкторы
BooleanSearch::BooleanSearch() {
    initialize();
}

BooleanSearch::BooleanSearch(std::unique_ptr<InvertedIndex> index) 
    : index_(std::move(index)) {
    initialize();
}

// Инициализация
void BooleanSearch::initialize() {
    parser_ = std::make_unique<QueryParser>();
    
    if (index_) {
        evaluator_ = std::make_unique<QueryEvaluator>(index_.get());
    }
}

// Загрузка индекса
bool BooleanSearch::load_index(const ds::String& index_file) {
    auto new_index = std::make_unique<InvertedIndex>();
    
    bool loaded = new_index->load_from_file(index_file);
    if (!loaded) {
        return false;
    }
    
    index_ = std::move(new_index);
    evaluator_ = std::make_unique<QueryEvaluator>(index_.get());
    
    return true;
}

// Поиск по запросу
SearchResult BooleanSearch::search(const ds::String& query, size_t limit) {
    SearchResult result;
    result.query = query;
    
    auto start_time = high_resolution_clock::now();
    
    try {
        // Парсим запрос
        QueryNode* ast = parser_->parse(query);
        
        if (!ast) {
            result.syntax_valid = false;
            result.error_message = "Failed to parse query";
            
            auto end_time = high_resolution_clock::now();
            result.time_ms = duration_cast<milliseconds>(end_time - start_time).count();
            
            stats_.add_query(false, result.time_ms);
            return result;
        }
        
        // Выполняем поиск
        ds::Vector<DocumentResult> doc_results;
        if (evaluator_) {
            doc_results = evaluator_->get_top_results(ast, limit);
        }
        
        // Заполняем результат
        result.total_found = evaluator_ ? evaluator_->count_results(ast) : 0;
        
        for (const auto& doc_result : doc_results) {
            result.doc_ids.push_back(doc_result.doc_id);
            result.scores.push_back(doc_result.score);
        }
        
        // Освобождаем память AST
        delete ast;
        
        auto end_time = high_resolution_clock::now();
        result.time_ms = duration_cast<milliseconds>(end_time - start_time).count();
        
        stats_.add_query(true, result.time_ms);
        
    } catch (const std::exception& e) {
        result.syntax_valid = false;
        result.error_message = ds::String(e.what());
        
        auto end_time = high_resolution_clock::now();
        result.time_ms = duration_cast<milliseconds>(end_time - start_time).count();
        
        stats_.add_query(false, result.time_ms);
    }
    
    return result;
}

// Пакетный поиск
ds::Vector<SearchResult> BooleanSearch::batch_search(const ds::Vector<ds::String>& queries, size_t limit) {
    ds::Vector<SearchResult> results;
    results.reserve(queries.size());
    
    for (const auto& query : queries) {
        results.push_back(search(query, limit));
    }
    
    return results;
}

// Проверка синтаксиса запроса
bool BooleanSearch::validate_query(const ds::String& query) const {
    return parser_->validate(query);
}

// Анализ запроса
BooleanSearch::QueryInfo BooleanSearch::analyze_query(const ds::String& query) const {
    QueryInfo info;
    info.original_query = query;
    
    try {
        QueryNode* ast = parser_->parse(query);
        
        if (ast) {
            info.is_valid = true;
            info.terms = parser_->extract_terms(ast);
            info.complexity = parser_->calculate_complexity(ast);
            info.parse_tree = ast->to_string();
            
            delete ast;
        } else {
            info.is_valid = false;
            info.error_message = "Failed to parse query";
        }
        
    } catch (const std::exception& e) {
        info.is_valid = false;
        info.error_message = ds::String(e.what());
    }
    
    return info;
}

// Сброс статистики
void BooleanSearch::reset_stats() {
    stats_ = SearchStats();
}

// Экспорт статистики
bool BooleanSearch::export_stats(const ds::String& filepath) const {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    file << "=== Boolean Search Statistics ===\n\n";
    file << "Total queries: " << stats_.total_queries << "\n";
    file << "Successful queries: " << stats_.successful_queries << "\n";
    file << "Failed queries: " << stats_.failed_queries << "\n";
    file << "Total search time: " << stats_.total_time_ms << " ms\n";
    file << "Average query time: " << stats_.get_average_time() << " ms\n";
    
    if (!stats_.query_times.empty()) {
        file << "\nQuery times distribution:\n";
        
        // Находим минимальное, максимальное и медианное время
        size_t min_time = *std::min_element(stats_.query_times.begin(), stats_.query_times.end());
        size_t max_time = *std::max_element(stats_.query_times.begin(), stats_.query_times.end());
        
        // Медиана
        ds::Vector<size_t> sorted_times = stats_.query_times;
        std::sort(sorted_times.begin(), sorted_times.end());
        size_t median = sorted_times[sorted_times.size() / 2];
        
        file << "  Min time: " << min_time << " ms\n";
        file << "  Max time: " << max_time << " ms\n";
        file << "  Median time: " << median << " ms\n";
        
        // Гистограмма
        file << "\nTime histogram:\n";
        const size_t bins = 10;
        size_t bin_size = (max_time - min_time + 1) / bins + 1;
        
        for (size_t i = 0; i < bins; ++i) {
            size_t bin_start = min_time + i * bin_size;
            size_t bin_end = bin_start + bin_size;
            
            size_t count = 0;
            for (size_t time : sorted_times) {
                if (time >= bin_start && time < bin_end) {
                    count++;
                }
            }
            
            if (count > 0) {
                file << "  " << bin_start << "-" << (bin_end - 1) << " ms: " 
                     << count << " queries\n";
            }
        }
    }
    
    if (index_) {
        const auto& index_stats = index_->get_stats();
        file << "\n=== Index Statistics ===\n\n";
        file << "Documents: " << index_stats.total_documents << "\n";
        file << "Terms: " << index_stats.total_terms << "\n";
        file << "Postings: " << index_stats.total_postings << "\n";
        file << "Avg document length: " << index_stats.avg_document_length << " terms\n";
    }
    
    return true;
}

// Подсказки по терминам
ds::Vector<ds::String> BooleanSearch::suggest_terms(const ds::String& prefix, size_t max_suggestions) const {
    ds::Vector<ds::String> suggestions;
    
    if (!index_ || prefix.empty()) {
        return suggestions;
    }
    
    ds::String prefix_lower = prefix.to_lower();
    auto all_terms = index_->get_all_terms();
    
    for (const auto& term : all_terms) {
        if (term.starts_with(prefix_lower)) {
            suggestions.push_back(term);
            
            if (suggestions.size() >= max_suggestions) {
                break;
            }
        }
    }
    
    return suggestions;
}

// Поиск похожих документов
ds::Vector<uint32_t> BooleanSearch::find_similar(uint32_t doc_id, size_t max_results) {
    ds::Vector<uint32_t> similar_docs;
    
    if (!index_ || doc_id >= index_->get_document_count()) {
        return similar_docs;
    }
    
    // Получаем документ
    const Document& doc = index_->get_document(doc_id);
    
    // Создаем простой запрос из терминов документа
    // В реальной системе здесь была бы более сложная логика
    ds::Vector<ds::String> terms = parser_->extract_terms(parser_->parse(doc.content));
    
    if (terms.empty()) {
        return similar_docs;
    }
    
    // Строим запрос OR из уникальных терминов
    ds::String query;
    for (size_t i = 0; i < terms.size(); ++i) {
        if (i > 0) query += " || ";
        query += terms[i];
    }
    
    // Исключаем исходный документ из результатов
    SearchResult result = search(query, max_results + 1);
    
    for (uint32_t found_doc_id : result.doc_ids) {
        if (found_doc_id != doc_id) {
            similar_docs.push_back(found_doc_id);
            
            if (similar_docs.size() >= max_results) {
                break;
            }
        }
    }
    
    return similar_docs;
}

// Получение документа по ID
const Document* BooleanSearch::get_document(uint32_t doc_id) const {
    if (!index_ || doc_id >= index_->get_document_count()) {
        return nullptr;
    }
    
    return &index_->get_document(doc_id);
}

// Извлечение терминов из запроса для сниппета
ds::Vector<ds::String> BooleanSearch::extract_query_terms(const ds::String& query) const {
    ds::Vector<ds::String> terms;
    
    try {
        QueryNode* ast = parser_->parse(query);
        if (ast) {
            terms = parser_->extract_terms(ast);
            delete ast;
        }
    } catch (...) {
        // В случае ошибки парсинга возвращаем пустой список
    }
    
    return terms;
}

// Получение сниппета
ds::String BooleanSearch::get_snippet(uint32_t doc_id, const ds::String& query, size_t context_words) {
    if (!index_ || doc_id >= index_->get_document_count()) {
        return "";
    }
    
    const Document& doc = index_->get_document(doc_id);
    ds::String content = doc.content;
    
    // Извлекаем термины из запроса
    ds::Vector<ds::String> query_terms = extract_query_terms(query);
    if (query_terms.empty()) {
        // Если не удалось извлечь термины, возвращаем начало документа
        size_t end_pos = std::min(content.size(), size_t(200));
        return content.substr(0, end_pos) + (content.size() > 200 ? "..." : "");
    }
    
    // Токенизируем контент документа
    Tokenizer tokenizer;
    auto token_positions = tokenizer.tokenize_with_positions(content);
    
    if (token_positions.empty()) {
        return "";
    }
    
    // Находим позиции, где встречаются термины запроса
    ds::Vector<size_t> match_positions;
    
    for (size_t i = 0; i < token_positions.size(); ++i) {
        const auto& token_info = token_positions[i];
        ds::String token_lower = token_info.token.to_lower();
        
        for (const auto& query_term : query_terms) {
            if (token_lower == query_term.to_lower()) {
                match_positions.push_back(i);
                break;
            }
        }
    }
    
    if (match_positions.empty()) {
        // Не нашли совпадений, возвращаем начало
        size_t end_pos = std::min(content.size(), size_t(200));
        return content.substr(0, end_pos) + (content.size() > 200 ? "..." : "");
    }
    
    // Выбираем лучшую позицию для сниппета (первое совпадение)
    size_t snippet_start_pos = match_positions[0];
    
    // Вычисляем границы сниппета
    size_t start_idx = (snippet_start_pos > context_words) ? 
                       snippet_start_pos - context_words : 0;
    
    size_t end_idx = std::min(token_positions.size(), snippet_start_pos + context_words + 1);
    
    // Собираем сниппет
    ds::String snippet;
    for (size_t i = start_idx; i < end_idx; ++i) {
        if (i > start_idx) snippet += " ";
        
        // Выделяем найденные термины
        ds::String token = token_positions[i].token;
        bool is_match = false;
        
        for (const auto& query_term : query_terms) {
            if (token.to_lower() == query_term.to_lower()) {
                is_match = true;
                break;
            }
        }
        
        if (is_match) {
            snippet += "[" + token + "]";  // Простое выделение
        } else {
            snippet += token;
        }
    }
    
    // Добавляем многоточие, если сниппет обрезан
    if (start_idx > 0) {
        snippet = "..." + snippet;
    }
    if (end_idx < token_positions.size()) {
        snippet += "...";
    }
    
    return snippet;
}

} // namespace search