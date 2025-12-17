#include "query_parser.h"
#include <iostream>
#include <cctype>
#include <algorithm>

namespace search {

// Токенизация запроса
ds::Vector<QueryToken> QueryParser::tokenize(const ds::String& query) {
    ds::Vector<QueryToken> tokens;
    
    size_t i = 0;
    size_t length = query.size();
    
    while (i < length) {
        char c = query[i];
        
        // Пропускаем пробелы
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }
        
        // Комментарии (начинаются с # и идут до конца строки)
        if (c == '#') {
            while (i < length && query[i] != '\n') {
                ++i;
            }
            continue;
        }
        
        // Логические операторы
        if (c == '&' && i + 1 < length && query[i + 1] == '&') {
            tokens.push_back(QueryToken(TokenType::AND, i));
            i += 2;
            continue;
        }
        
        if (c == '|' && i + 1 < length && query[i + 1] == '|') {
            tokens.push_back(QueryToken(TokenType::OR, i));
            i += 2;
            continue;
        }
        
        if (c == '!') {
            tokens.push_back(QueryToken(TokenType::NOT, i));
            ++i;
            continue;
        }
        
        // Скобки
        if (c == '(') {
            tokens.push_back(QueryToken(TokenType::LPAREN, i));
            ++i;
            continue;
        }
        
        if (c == ')') {
            tokens.push_back(QueryToken(TokenType::RPAREN, i));
            ++i;
            continue;
        }
        
        // Кавычки для фраз
        if (c == '"') {
            tokens.push_back(QueryToken(TokenType::QUOTE, i));
            ++i;
            
            // Собираем содержимое кавычек
            ds::String phrase_content;
            while (i < length && query[i] != '"') {
                phrase_content.push_back(query[i]);
                ++i;
            }
            
            if (i < length && query[i] == '"') {
                // Закрывающая кавычка
                tokens.push_back(QueryToken(TokenType::TERM, phrase_content, i));
                tokens.push_back(QueryToken(TokenType::QUOTE, i));
                ++i;
            } else {
                // Незакрытая кавычка
                error("Unclosed quote", i);
            }
            
            // Проверяем оператор близости
            while (i < length && std::isspace(static_cast<unsigned char>(query[i]))) {
                ++i;
            }
            
            if (i < length && query[i] == '/') {
                ++i;
                
                // Собираем число для оператора близости
                ds::String distance_str;
                while (i < length && std::isdigit(static_cast<unsigned char>(query[i]))) {
                    distance_str.push_back(query[i]);
                    ++i;
                }
                
                if (!distance_str.empty()) {
                    tokens.push_back(QueryToken(TokenType::PROXIMITY, distance_str, i));
                } else {
                    error("Invalid proximity operator", i);
                }
            }
            
            continue;
        }
        
        // Термин (слово)
        if (std::isalnum(static_cast<unsigned char>(c)) || 
            static_cast<unsigned char>(c) >= 128) { // Для UTF-8 символов
            
            ds::String term;
            while (i < length && 
                   (std::isalnum(static_cast<unsigned char>(query[i])) || 
                    query[i] == '-' || query[i] == '_' ||
                    query[i] == '\'' || 
                    static_cast<unsigned char>(query[i]) >= 128)) {
                term.push_back(query[i]);
                ++i;
            }
            
            // Приводим к нижнему регистру
            term = term.to_lower();
            tokens.push_back(QueryToken(TokenType::TERM, term, i));
            continue;
        }
        
        // Неизвестный символ
        error("Unknown character in query: " + ds::String(1, c), i);
        ++i;
    }
    
    tokens.push_back(QueryToken(TokenType::END, length));
    return tokens;
}

// Проверка совпадения токена
bool QueryParser::match(TokenType type, size_t pos, const ds::Vector<QueryToken>& tokens) const {
    if (check(type, pos, tokens)) {
        return true;
    }
    return false;
}

bool QueryParser::check(TokenType type, size_t pos, const ds::Vector<QueryToken>& tokens) const {
    if (pos >= tokens.size()) {
        return false;
    }
    return tokens[pos].type == type;
}

// Переход к следующему токену
QueryToken QueryParser::advance(size_t& pos, const ds::Vector<QueryToken>& tokens) {
    if (pos < tokens.size()) {
        return tokens[pos++];
    }
    return QueryToken(TokenType::END, pos);
}

// Обработка ошибок
void QueryParser::error(const ds::String& message, size_t position) const {
    throw std::runtime_error("Query parsing error at position " + 
                            std::to_string(position) + ": " + message.c_str());
}

// Парсинг выражения (самый низкий приоритет: OR)
QueryNode* QueryParser::parse_expression(size_t& pos, const ds::Vector<QueryToken>& tokens) {
    QueryNode* left = parse_term(pos, tokens);
    
    while (match(TokenType::OR, pos, tokens)) {
        advance(pos, tokens); // Пропускаем OR
        
        QueryNode* right = parse_term(pos, tokens);
        left = new BinaryOpNode(QueryNode::Type::OR, left, right);
    }
    
    return left;
}

// Парсинг терма (средний приоритет: AND)
QueryNode* QueryParser::parse_term(size_t& pos, const ds::Vector<QueryToken>& tokens) {
    QueryNode* left = parse_factor(pos, tokens);
    
    while (match(TokenType::AND, pos, tokens) || 
           (!match(TokenType::RPAREN, pos, tokens) && 
            !match(TokenType::OR, pos, tokens) &&
            !match(TokenType::END, pos, tokens))) {
        
        // Неявный AND (просто последовательность терминов)
        if (!match(TokenType::AND, pos, tokens)) {
            // Пропускаем только если это не оператор
        } else {
            advance(pos, tokens); // Пропускаем явный AND
        }
        
        QueryNode* right = parse_factor(pos, tokens);
        left = new BinaryOpNode(QueryNode::Type::AND, left, right);
    }
    
    return left;
}

// Парсинг фактора (высокий приоритет: NOT, скобки, термины)
QueryNode* QueryParser::parse_factor(size_t& pos, const ds::Vector<QueryToken>& tokens) {
    if (match(TokenType::NOT, pos, tokens)) {
        advance(pos, tokens); // Пропускаем NOT
        
        QueryNode* operand = parse_factor(pos, tokens);
        return new UnaryOpNode(operand);
    }
    
    return parse_primary(pos, tokens);
}

// Парсинг первичного выражения (термины, фразы, скобки)
QueryNode* QueryParser::parse_primary(size_t& pos, const ds::Vector<QueryToken>& tokens) {
    if (match(TokenType::LPAREN, pos, tokens)) {
        advance(pos, tokens); // Пропускаем '('
        
        QueryNode* expr = parse_expression(pos, tokens);
        
        if (!match(TokenType::RPAREN, pos, tokens)) {
            error("Expected ')'", tokens[pos].position);
        }
        advance(pos, tokens); // Пропускаем ')'
        
        return expr;
    }
    
    if (match(TokenType::QUOTE, pos, tokens)) {
        advance(pos, tokens); // Пропускаем открывающую кавычку
        
        // Должен быть TERM с содержимым фразы
        if (!match(TokenType::TERM, pos, tokens)) {
            error("Expected phrase content", tokens[pos].position);
        }
        
        QueryToken phrase_token = advance(pos, tokens);
        
        // Разбиваем содержимое фразы на термины
        ds::Vector<ds::String> phrase_terms = phrase_token.value.split(' ');
        
        // Проверяем закрывающую кавычку
        if (!match(TokenType::QUOTE, pos, tokens)) {
            error("Expected closing quote", tokens[pos].position);
        }
        advance(pos, tokens); // Пропускаем закрывающую кавычку
        
        // Проверяем оператор близости
        if (match(TokenType::PROXIMITY, pos, tokens)) {
            QueryToken proximity_token = advance(pos, tokens);
            size_t distance = std::stoul(proximity_token.value.c_str());
            
            return new ProximityNode(phrase_terms, distance);
        }
        
        return new PhraseNode(phrase_terms);
    }
    
    if (match(TokenType::TERM, pos, tokens)) {
        QueryToken term_token = advance(pos, tokens);
        return new TermNode(term_token.value);
    }
    
    error("Expected term, phrase, or '('", tokens[pos].position);
    return nullptr; // Недостижимо
}

// Основной метод парсинга
QueryNode* QueryParser::parse(const ds::String& query) {
    try {
        ds::Vector<QueryToken> tokens = tokenize(query);
        
        if (tokens.empty() || 
            (tokens.size() == 1 && tokens[0].type == TokenType::END)) {
            // Пустой запрос
            return nullptr;
        }
        
        size_t pos = 0;
        QueryNode* root = parse_expression(pos, tokens);
        
        // Проверяем, что разобрали все токены (кроме END)
        if (!match(TokenType::END, pos, tokens)) {
            error("Unexpected token", tokens[pos].position);
        }
        
        return optimize(root);
        
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return nullptr;
    }
}

// Оптимизация AST
QueryNode* QueryParser::optimize(QueryNode* root) {
    if (!root) return nullptr;
    
    // Рекурсивно оптимизируем детей
    if (root->get_type() == QueryNode::Type::AND || 
        root->get_type() == QueryNode::Type::OR) {
        BinaryOpNode* bin_node = static_cast<BinaryOpNode*>(root);
        
        QueryNode* left = optimize(bin_node->get_left());
        QueryNode* right = optimize(bin_node->get_right());
        
        // Если оба операнда одинаковые (A AND A) или (A OR A)
        if (left && right && left->to_string() == right->to_string()) {
            delete right;
            delete bin_node;
            return left;
        }
        
        // NOT NOT A = A
        if (root->get_type() == QueryNode::Type::NOT) {
            UnaryOpNode* not_node = static_cast<UnaryOpNode*>(root);
            if (not_node->get_operand()->get_type() == QueryNode::Type::NOT) {
                UnaryOpNode* inner_not = static_cast<UnaryOpNode*>(not_node->get_operand());
                QueryNode* result = inner_not->get_operand();
                
                // Не удаляем inner_not, так как он будет удален при удалении not_node
                delete not_node;
                return optimize(result);
            }
        }
        
        return root;
    }
    
    return root;
}

// Валидация запроса
bool QueryParser::validate(const ds::String& query) const {
    try {
        QueryParser parser;
        QueryNode* root = parser.parse(query);
        
        if (root) {
            delete root;
            return true;
        }
        return false;
        
    } catch (...) {
        return false;
    }
}

// Извлечение уникальных терминов из запроса
ds::Vector<ds::String> QueryParser::extract_terms(QueryNode* root) const {
    ds::Vector<ds::String> terms;
    ds::HashTable<ds::String, bool> unique_terms;
    
    // Вспомогательная функция для обхода дерева
    std::function<void(QueryNode*)> collect_terms = [&](QueryNode* node) {
        if (!node) return;
        
        switch (node->get_type()) {
            case QueryNode::Type::TERM: {
                TermNode* term_node = static_cast<TermNode*>(node);
                if (!unique_terms.find(term_node->get_term())) {
                    terms.push_back(term_node->get_term());
                    unique_terms.insert(term_node->get_term(), true);
                }
                break;
            }
            case QueryNode::Type::PHRASE: {
                PhraseNode* phrase_node = static_cast<PhraseNode*>(node);
                for (const auto& term : phrase_node->get_terms()) {
                    if (!unique_terms.find(term)) {
                        terms.push_back(term);
                        unique_terms.insert(term, true);
                    }
                }
                break;
            }
            case QueryNode::Type::PROXIMITY: {
                ProximityNode* prox_node = static_cast<ProximityNode*>(node);
                for (const auto& term : prox_node->get_terms()) {
                    if (!unique_terms.find(term)) {
                        terms.push_back(term);
                        unique_terms.insert(term, true);
                    }
                }
                break;
            }
            case QueryNode::Type::AND:
            case QueryNode::Type::OR: {
                BinaryOpNode* bin_node = static_cast<BinaryOpNode*>(node);
                collect_terms(bin_node->get_left());
                collect_terms(bin_node->get_right());
                break;
            }
            case QueryNode::Type::NOT: {
                UnaryOpNode* unary_node = static_cast<UnaryOpNode*>(node);
                collect_terms(unary_node->get_operand());
                break;
            }
        }
    };
    
    collect_terms(root);
    return terms;
}

// Подсчет сложности запроса
size_t QueryParser::calculate_complexity(QueryNode* root) const {
    if (!root) return 0;
    
    size_t complexity = 0;
    
    switch (root->get_type()) {
        case QueryNode::Type::TERM:
        case QueryNode::Type::PHRASE:
        case QueryNode::Type::PROXIMITY:
            return 1;
            
        case QueryNode::Type::AND:
        case QueryNode::Type::OR: {
            BinaryOpNode* bin_node = static_cast<BinaryOpNode*>(root);
            complexity = 1 + calculate_complexity(bin_node->get_left()) 
                            + calculate_complexity(bin_node->get_right());
            break;
        }
        case QueryNode::Type::NOT: {
            UnaryOpNode* unary_node = static_cast<UnaryOpNode*>(root);
            complexity = 1 + calculate_complexity(unary_node->get_operand());
            break;
        }
    }
    
    return complexity;
}

} // namespace search