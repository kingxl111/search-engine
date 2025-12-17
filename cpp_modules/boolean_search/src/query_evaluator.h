#ifndef QUERY_EVALUATOR_H
#define QUERY_EVALUATOR_H

#include "query_parser.h"
#include "../../boolean_index/src/inverted_index.h"
#include "../../data_structures/bit_vector.h"
#include <memory>
#include <functional>

namespace search {

// Результат оценки запроса для документа
struct DocumentResult {
    uint32_t doc_id;
    double score;           // Релевантность (для будущего использования)
    size_t matches;         // Количество совпадений
    ds::Vector<size_t> positions; // Позиции совпадений (для выделения сниппетов)
    
    DocumentResult(uint32_t id) : doc_id(id), score(0.0), matches(0) {}
    
    bool operator<(const DocumentResult& other) const {
        // Сначала по score, потом по doc_id
        if (score != other.score) {
            return score > other.score; // Больше score = выше
        }
        return doc_id < other.doc_id;
    }
};

// Класс для оценки запросов над индексом
class QueryEvaluator {
private:
    const InvertedIndex* index_;
    
    // Вспомогательные методы для оценки разных типов узлов
    ds::BitVector evaluate_term(const TermNode* node);
    ds::BitVector evaluate_phrase(const PhraseNode* node);
    ds::BitVector evaluate_proximity(const ProximityNode* node);
    ds::BitVector evaluate_and(const BinaryOpNode* node);
    ds::BitVector evaluate_or(const BinaryOpNode* node);
    ds::BitVector evaluate_not(const UnaryOpNode* node);
    
    // Оценка позиций для фраз и близости
    bool check_phrase_positions(uint32_t doc_id, const ds::Vector<ds::String>& terms);
    bool check_proximity_positions(uint32_t doc_id, const ds::Vector<ds::String>& terms, size_t max_distance);
    
    // Получение позиций термина в документе
    ds::Vector<uint32_t> get_term_positions(const ds::String& term, uint32_t doc_id) const;
    
public:
    QueryEvaluator(const InvertedIndex* index) : index_(index) {}
    ~QueryEvaluator() = default;
    
    // Основной метод оценки запроса
    ds::BitVector evaluate(QueryNode* query);
    
    // Оценка с возвратом подробных результатов
    ds::Vector<DocumentResult> evaluate_detailed(QueryNode* query);
    
    // Быстрая оценка (только наличие документов)
    bool evaluate_exists(QueryNode* query);
    
    // Подсчет количества документов в результате
    size_t count_results(QueryNode* query);
    
    // Получение топ-N документов
    ds::Vector<DocumentResult> get_top_results(QueryNode* query, size_t n);
    
    // Проверка, содержит ли документ результаты запроса
    bool document_matches(QueryNode* query, uint32_t doc_id);
};

} // namespace search

#endif // QUERY_EVALUATOR_H