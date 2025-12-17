#include "query_evaluator.h"
#include <algorithm>
#include <cmath>

namespace search {

// Оценка термина
ds::BitVector QueryEvaluator::evaluate_term(const TermNode* node) {
    if (!index_ || !node) {
        return ds::BitVector();
    }
    
    const ds::String& term = node->get_term();
    const auto* postings = index_->find_postings(term);
    
    if (!postings) {
        // Термин не найден ни в одном документе
        return ds::BitVector(index_->get_document_count());
    }
    
    // Создаем битовый вектор для всех документов
    ds::BitVector result(index_->get_document_count());
    
    // Устанавливаем биты для документов, содержащих термин
    for (const auto& posting : *postings) {
        if (posting.doc_id < result.size()) {
            result.set(posting.doc_id, true);
        }
    }
    
    return result;
}

// Получение позиций термина в документе
ds::Vector<uint32_t> QueryEvaluator::get_term_positions(const ds::String& term, uint32_t doc_id) const {
    ds::Vector<uint32_t> positions;
    
    const auto* postings = index_->find_postings(term);
    if (!postings) {
        return positions;
    }
    
    for (const auto& posting : *postings) {
        if (posting.doc_id == doc_id) {
            return posting.positions;
        }
    }
    
    return positions;
}

// Проверка позиций для фразы
bool QueryEvaluator::check_phrase_positions(uint32_t doc_id, const ds::Vector<ds::String>& terms) {
    if (terms.empty()) {
        return false;
    }
    
    // Получаем позиции первого термина
    ds::Vector<uint32_t> first_positions = get_term_positions(terms[0], doc_id);
    if (first_positions.empty()) {
        return false;
    }
    
    // Для каждой позиции первого термина проверяем, есть ли последующие термины
    for (uint32_t pos : first_positions) {
        bool found = true;
        
        for (size_t i = 1; i < terms.size(); ++i) {
            ds::Vector<uint32_t> term_positions = get_term_positions(terms[i], doc_id);
            
            // Ищем позицию pos + i в списке позиций термина
            bool has_position = false;
            for (uint32_t term_pos : term_positions) {
                if (term_pos == pos + i) {
                    has_position = true;
                    break;
                }
            }
            
            if (!has_position) {
                found = false;
                break;
            }
        }
        
        if (found) {
            return true;
        }
    }
    
    return false;
}

// Проверка позиций для близости
bool QueryEvaluator::check_proximity_positions(uint32_t doc_id, 
                                               const ds::Vector<ds::String>& terms, 
                                               size_t max_distance) {
    if (terms.empty()) {
        return false;
    }
    
    // Получаем позиции для всех терминов
    ds::Vector<ds::Vector<uint32_t>> all_positions;
    for (const auto& term : terms) {
        all_positions.push_back(get_term_positions(term, doc_id));
        
        if (all_positions.back().empty()) {
            return false;
        }
    }
    
    // Ищем такие позиции, чтобы расстояние между первым и последним было <= max_distance
    // Простая проверка: для каждой позиции первого термина ищем подходящие позиции других
    for (uint32_t first_pos : all_positions[0]) {
        bool found_sequence = true;
        
        for (size_t i = 1; i < all_positions.size(); ++i) {
            bool has_close_position = false;
            
            for (uint32_t pos : all_positions[i]) {
                // Проверяем, что позиция идет после first_pos и в пределах расстояния
                if (pos >= first_pos && pos <= first_pos + max_distance) {
                    has_close_position = true;
                    break;
                }
            }
            
            if (!has_close_position) {
                found_sequence = false;
                break;
            }
        }
        
        if (found_sequence) {
            return true;
        }
    }
    
    return false;
}

// Оценка фразы
ds::BitVector QueryEvaluator::evaluate_phrase(const PhraseNode* node) {
    if (!index_ || !node) {
        return ds::BitVector();
    }
    
    const auto& terms = node->get_terms();
    if (terms.empty()) {
        return ds::BitVector(index_->get_document_count());
    }
    
    // Начинаем с документов, содержащих первый термин
    ds::BitVector result = evaluate_term(new TermNode(terms[0]));
    
    // Проверяем каждый документ на наличие полной фразы
    for (size_t doc_id = result.find_first(); doc_id < result.size(); doc_id = result.find_next(doc_id)) {
        if (!check_phrase_positions(static_cast<uint32_t>(doc_id), terms)) {
            result.set(doc_id, false);
        }
    }
    
    return result;
}

// Оценка близости
ds::BitVector QueryEvaluator::evaluate_proximity(const ProximityNode* node) {
    if (!index_ || !node) {
        return ds::BitVector();
    }
    
    const auto& terms = node->get_terms();
    size_t distance = node->get_distance();
    
    if (terms.empty()) {
        return ds::BitVector(index_->get_document_count());
    }
    
    // Начинаем с документов, содержащих первый термин
    ds::BitVector result = evaluate_term(new TermNode(terms[0]));
    
    // Проверяем каждый документ на близость терминов
    for (size_t doc_id = result.find_first(); doc_id < result.size(); doc_id = result.find_next(doc_id)) {
        if (!check_proximity_positions(static_cast<uint32_t>(doc_id), terms, distance)) {
            result.set(doc_id, false);
        }
    }
    
    return result;
}

// Оценка AND
ds::BitVector QueryEvaluator::evaluate_and(const BinaryOpNode* node) {
    if (!node) {
        return ds::BitVector();
    }
    
    ds::BitVector left = evaluate(node->get_left());
    ds::BitVector right = evaluate(node->get_right());
    
    // AND = пересечение битовых векторов
    left &= right;
    return left;
}

// Оценка OR
ds::BitVector QueryEvaluator::evaluate_or(const BinaryOpNode* node) {
    if (!node) {
        return ds::BitVector();
    }
    
    ds::BitVector left = evaluate(node->get_left());
    ds::BitVector right = evaluate(node->get_right());
    
    // OR = объединение битовых векторов
    left |= right;
    return left;
}

// Оценка NOT
ds::BitVector QueryEvaluator::evaluate_not(const UnaryOpNode* node) {
    if (!node) {
        return ds::BitVector();
    }
    
    ds::BitVector operand = evaluate(node->get_operand());
    
    // NOT = инверсия битового вектора
    operand.flip_all();
    
    // Убедимся, что лишние биты (за пределами количества документов) сброшены
    size_t doc_count = index_->get_document_count();
    for (size_t i = doc_count; i < operand.size(); ++i) {
        operand.set(i, false);
    }
    
    return operand;
}

// Основной метод оценки
ds::BitVector QueryEvaluator::evaluate(QueryNode* query) {
    if (!query || !index_) {
        return ds::BitVector();
    }
    
    switch (query->get_type()) {
        case QueryNode::Type::TERM:
            return evaluate_term(static_cast<TermNode*>(query));
            
        case QueryNode::Type::PHRASE:
            return evaluate_phrase(static_cast<PhraseNode*>(query));
            
        case QueryNode::Type::PROXIMITY:
            return evaluate_proximity(static_cast<ProximityNode*>(query));
            
        case QueryNode::Type::AND:
            return evaluate_and(static_cast<BinaryOpNode*>(query));
            
        case QueryNode::Type::OR:
            return evaluate_or(static_cast<BinaryOpNode*>(query));
            
        case QueryNode::Type::NOT:
            return evaluate_not(static_cast<UnaryOpNode*>(query));
            
        default:
            return ds::BitVector();
    }
}

// Подробная оценка с результатами
ds::Vector<DocumentResult> QueryEvaluator::evaluate_detailed(QueryNode* query) {
    ds::Vector<DocumentResult> results;
    
    if (!query || !index_) {
        return results;
    }
    
    // Получаем битовый вектор результатов
    ds::BitVector bit_results = evaluate(query);
    
    // Преобразуем в список документов
    for (size_t doc_id = bit_results.find_first(); 
         doc_id < bit_results.size(); 
         doc_id = bit_results.find_next(doc_id)) {
        
        DocumentResult result(static_cast<uint32_t>(doc_id));
        
        // Вычисляем score (простой вариант: количество совпадающих терминов)
        // В реальной системе здесь была бы более сложная формула
        result.score = 1.0;
        result.matches = 1;
        
        results.push_back(result);
    }
    
    // Сортируем по score
    std::sort(results.begin(), results.end());
    
    return results;
}

// Быстрая оценка наличия результатов
bool QueryEvaluator::evaluate_exists(QueryNode* query) {
    if (!query || !index_) {
        return false;
    }
    
    ds::BitVector results = evaluate(query);
    return results.any();
}

// Подсчет количества результатов
size_t QueryEvaluator::count_results(QueryNode* query) {
    if (!query || !index_) {
        return 0;
    }
    
    ds::BitVector results = evaluate(query);
    return results.count();
}

// Получение топ-N результатов
ds::Vector<DocumentResult> QueryEvaluator::get_top_results(QueryNode* query, size_t n) {
    ds::Vector<DocumentResult> all_results = evaluate_detailed(query);
    
    if (n >= all_results.size()) {
        return all_results;
    }
    
    ds::Vector<DocumentResult> top_results;
    for (size_t i = 0; i < n && i < all_results.size(); ++i) {
        top_results.push_back(all_results[i]);
    }
    
    return top_results;
}

// Проверка конкретного документа
bool QueryEvaluator::document_matches(QueryNode* query, uint32_t doc_id) {
    if (!query || !index_ || doc_id >= index_->get_document_count()) {
        return false;
    }
    
    ds::BitVector results = evaluate(query);
    return doc_id < results.size() ? results[doc_id] : false;
}

} // namespace search