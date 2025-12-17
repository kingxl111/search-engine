#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "../../data_structures/string.h"
#include "../../data_structures/vector.h"
#include "../../data_structures/hash_table.h"

#include <cstddef>
#include <utility>
#include <functional>

namespace search {

class Tokenizer {
private:
    // Стоп-слова (частые слова, которые обычно исключаются)
    ds::HashTable<ds::String, bool> stopwords_;
    
    // Минимальная и максимальная длина токена
    size_t min_token_length_;
    size_t max_token_length_;
    
    // Флаги для настройки токенизации
    bool remove_numbers_;
    bool remove_punctuation_;
    bool case_folding_;
    
    // Загружает стоп-слова из файла
    bool load_stopwords(const ds::String& filepath);
    
    // Проверяет, является ли символ пунктуацией
    static bool is_punctuation(char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        return (uc >= 33 && uc <= 47) ||   // !"#$%&'()*+,-./
               (uc >= 58 && uc <= 64) ||   // :;<=>?@
               (uc >= 91 && uc <= 96) ||   // [\]^_`
               (uc >= 123 && uc <= 126);   // {|}~
    }
    
    // Проверяет, является ли символ разделителем
    static bool is_delimiter(char c) {
        return std::isspace(static_cast<unsigned char>(c)) || 
               is_punctuation(c);
    }
    
    // Нормализует токен (приводит к нижнему регистру, удаляет лишние символы)
    ds::String normalize_token(const ds::String& token) const;
    
public:
    // Конструкторы
    Tokenizer();
    
    explicit Tokenizer(const ds::String& stopwords_path);
    
    Tokenizer(const ds::String& stopwords_path, 
              size_t min_token_length, 
              size_t max_token_length,
              bool remove_numbers = false,
              bool remove_punctuation = true,
              bool case_folding = true);
    
    // Деструктор
    ~Tokenizer() = default;
    
    // Токенизация строки
    ds::Vector<ds::String> tokenize(const ds::String& text) const;
    
    // Токенизация с сохранением позиций
    struct TokenWithPosition {
        ds::String token;
        size_t position;
        size_t length;
    };
    
    ds::Vector<TokenWithPosition> tokenize_with_positions(const ds::String& text) const;
    
    // Пакетная токенизация
    ds::Vector<ds::Vector<ds::String>> batch_tokenize(const ds::Vector<ds::String>& texts) const;
    
    // Настройки
    void set_min_token_length(size_t length) { min_token_length_ = length; }
    void set_max_token_length(size_t length) { max_token_length_ = length; }
    void set_remove_numbers(bool remove) { remove_numbers_ = remove; }
    void set_remove_punctuation(bool remove) { remove_punctuation_ = remove; }
    void set_case_folding(bool fold) { case_folding_ = fold; }
    
    size_t get_min_token_length() const { return min_token_length_; }
    size_t get_max_token_length() const { return max_token_length_; }
    bool get_remove_numbers() const { return remove_numbers_; }
    bool get_remove_punctuation() const { return remove_punctuation_; }
    bool get_case_folding() const { return case_folding_; }
    
    // Проверка, является ли слово стоп-словом
    bool is_stopword(const ds::String& word) const {
        return stopwords_.find(word) != nullptr;
    }
    
    // Добавление/удаление стоп-слов
    void add_stopword(const ds::String& word) {
        stopwords_.insert(word, true);
    }
    
    void remove_stopword(const ds::String& word) {
        stopwords_.erase(word);
    }
    
    // Статистика
    struct TokenStats {
        size_t total_tokens;
        size_t unique_tokens;
        size_t filtered_tokens;
        double avg_token_length;
        double tokens_per_document;
    };
    
    TokenStats calculate_stats(const ds::Vector<ds::Vector<ds::String>>& tokenized_docs) const;
    
    // Сохранение/загрузка конфигурации
    bool save_config(const ds::String& filepath) const;
    bool load_config(const ds::String& filepath);
};

} // namespace search

#endif // TOKENIZER_H