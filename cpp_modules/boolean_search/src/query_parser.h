#ifndef QUERY_PARSER_H
#define QUERY_PARSER_H

#include "../../data_structures/vector.h"
#include "../../data_structures/string.h"
#include <cstddef>
#include <stdexcept>

namespace search {

// Типы токенов запроса
enum class TokenType {
    TERM,           // Поисковый термин
    AND,            // Логическое И (&&)
    OR,             // Логическое ИЛИ (||)
    NOT,            // Логическое НЕТ (!)
    LPAREN,         // Открывающая скобка
    RPAREN,         // Закрывающая скобка
    QUOTE,          // Кавычка для фразового поиска
    PROXIMITY,      // Оператор близости (/N)
    END             // Конец запроса
};

// Токен запроса
struct QueryToken {
    TokenType type;
    ds::String value;   // Для TERM, QUOTE, PROXIMITY
    size_t position;    // Позиция в исходном запросе
    
    QueryToken() : type(TokenType::END), position(0) {}
    QueryToken(TokenType t, size_t pos) : type(t), position(pos) {}
    QueryToken(TokenType t, const ds::String& val, size_t pos) 
        : type(t), value(val), position(pos) {}
};

// Абстрактный узел AST (Abstract Syntax Tree)
class QueryNode {
public:
    enum class Type {
        TERM,
        PHRASE,
        PROXIMITY,
        AND,
        OR,
        NOT
    };
    
    virtual ~QueryNode() = default;
    virtual Type get_type() const = 0;
    virtual ds::String to_string() const = 0;
    
    // Для отладки
    virtual void print_tree(size_t indent = 0) const = 0;
};

// Узел термина
class TermNode : public QueryNode {
private:
    ds::String term_;
    
public:
    explicit TermNode(const ds::String& term) : term_(term) {}
    
    Type get_type() const override { return Type::TERM; }
    
    const ds::String& get_term() const { return term_; }
    
    ds::String to_string() const override {
        return term_;
    }
    
    void print_tree(size_t indent = 0) const override {
        ds::String spaces(indent, ' ');
        std::cout << spaces << "TERM: " << term_ << "\n";
    }
};

// Узел фразы (последовательность терминов)
class PhraseNode : public QueryNode {
private:
    ds::Vector<ds::String> terms_;
    
public:
    explicit PhraseNode(const ds::Vector<ds::String>& terms) : terms_(terms) {}
    
    Type get_type() const override { return Type::PHRASE; }
    
    const ds::Vector<ds::String>& get_terms() const { return terms_; }
    
    ds::String to_string() const override {
        ds::String result = "\"";
        for (size_t i = 0; i < terms_.size(); ++i) {
            if (i > 0) result += " ";
            result += terms_[i];
        }
        result += "\"";
        return result;
    }
    
    void print_tree(size_t indent = 0) const override {
        ds::String spaces(indent, ' ');
        std::cout << spaces << "PHRASE:\n";
        for (const auto& term : terms_) {
            std::cout << spaces << "  - " << term << "\n";
        }
    }
};

// Узел близости (фраза с расстоянием)
class ProximityNode : public QueryNode {
private:
    ds::Vector<ds::String> terms_;
    size_t distance_;
    
public:
    ProximityNode(const ds::Vector<ds::String>& terms, size_t distance) 
        : terms_(terms), distance_(distance) {}
    
    Type get_type() const override { return Type::PROXIMITY; }
    
    const ds::Vector<ds::String>& get_terms() const { return terms_; }
    size_t get_distance() const { return distance_; }
    
    ds::String to_string() const override {
        ds::String result = "\"";
        for (size_t i = 0; i < terms_.size(); ++i) {
            if (i > 0) result += " ";
            result += terms_[i];
        }
        result += "\" / " + ds::String(std::to_string(distance_).c_str());
        return result;
    }
    
    void print_tree(size_t indent = 0) const override {
        ds::String spaces(indent, ' ');
        std::cout << spaces << "PROXIMITY (distance=" << distance_ << "):\n";
        for (const auto& term : terms_) {
            std::cout << spaces << "  - " << term << "\n";
        }
    }
};

// Бинарный узел (AND, OR)
class BinaryOpNode : public QueryNode {
private:
    Type type_;
    QueryNode* left_;
    QueryNode* right_;
    
public:
    BinaryOpNode(Type type, QueryNode* left, QueryNode* right) 
        : type_(type), left_(left), right_(right) {}
    
    ~BinaryOpNode() {
        delete left_;
        delete right_;
    }
    
    Type get_type() const override { return type_; }
    
    QueryNode* get_left() const { return left_; }
    QueryNode* get_right() const { return right_; }
    
    ds::String to_string() const override {
        ds::String op;
        switch (type_) {
            case Type::AND: op = "&&"; break;
            case Type::OR: op = "||"; break;
            default: op = "?";
        }
        return "(" + left_->to_string() + " " + op + " " + right_->to_string() + ")";
    }
    
    void print_tree(size_t indent = 0) const override {
        ds::String spaces(indent, ' ');
        ds::String op_name;
        switch (type_) {
            case Type::AND: op_name = "AND"; break;
            case Type::OR: op_name = "OR"; break;
            default: op_name = "UNKNOWN";
        }
        
        std::cout << spaces << op_name << ":\n";
        left_->print_tree(indent + 2);
        right_->print_tree(indent + 2);
    }
};

// Унарный узел (NOT)
class UnaryOpNode : public QueryNode {
private:
    QueryNode* operand_;
    
public:
    explicit UnaryOpNode(QueryNode* operand) : operand_(operand) {}
    
    ~UnaryOpNode() {
        delete operand_;
    }
    
    Type get_type() const override { return Type::NOT; }
    
    QueryNode* get_operand() const { return operand_; }
    
    ds::String to_string() const override {
        return "!" + operand_->to_string();
    }
    
    void print_tree(size_t indent = 0) const override {
        ds::String spaces(indent, ' ');
        std::cout << spaces << "NOT:\n";
        operand_->print_tree(indent + 2);
    }
};

// Класс парсера запросов
class QueryParser {
private:
    // Токенизатор запроса
    ds::Vector<QueryToken> tokenize(const ds::String& query);
    
    // Парсинг выражения
    QueryNode* parse_expression(size_t& pos, const ds::Vector<QueryToken>& tokens);
    QueryNode* parse_term(size_t& pos, const ds::Vector<QueryToken>& tokens);
    QueryNode* parse_factor(size_t& pos, const ds::Vector<QueryToken>& tokens);
    QueryNode* parse_primary(size_t& pos, const ds::Vector<QueryToken>& tokens);
    
    // Проверка токенов
    bool match(TokenType type, size_t pos, const ds::Vector<QueryToken>& tokens) const;
    bool check(TokenType type, size_t pos, const ds::Vector<QueryToken>& tokens) const;
    QueryToken advance(size_t& pos, const ds::Vector<QueryToken>& tokens);
    
    // Обработка ошибок
    void error(const ds::String& message, size_t position) const;
    
public:
    QueryParser() = default;
    ~QueryParser() = default;
    
    // Парсинг запроса в AST
    QueryNode* parse(const ds::String& query);
    
    // Оптимизация AST (удаление лишних узлов, упрощение выражений)
    QueryNode* optimize(QueryNode* root);
    
    // Валидация запроса
    bool validate(const ds::String& query) const;
    
    // Получение списка уникальных терминов из запроса
    ds::Vector<ds::String> extract_terms(QueryNode* root) const;
    
    // Подсчет сложности запроса (количество операторов)
    size_t calculate_complexity(QueryNode* root) const;
};

} // namespace search

#endif // QUERY_PARSER_H