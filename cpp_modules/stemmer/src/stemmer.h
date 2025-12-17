#ifndef STEMMER_H
#define STEMMER_H

#include "../../data_structures/string.h"
#include "../../data_structures/vector.h"

namespace search {

// Класс для стемминга (нормализации) русских слов по алгоритму Портера
class RussianStemmer {
private:
    // Вспомогательные структуры и константы
    static const char VOWELS[6];
    static const char PERFECTIVE_GERUND_SUFFIXES[2][7];
    static const char ADJECTIVE_SUFFIXES[4][8];
    static const char PARTICIPLE_SUFFIXES[2][7];
    static const char REFLEXIVE_SUFFIXES[2][8];
    static const char VERB_SUFFIXES[2][7];
    static const char NOUN_SUFFIXES[3][9];
    static const char DERIVATIONAL_SUFFIXES[11];
    static const char SUPERLATIVE_SUFFIXES[2][8];
    
    // Вспомогательные функции
    static bool is_vowel(char c);
    static bool is_consonant(char c);
    static size_t rv_position(const ds::String& word);
    static size_t r2_position(const ds::String& word, size_t rv);
    static bool ends_with(const ds::String& word, const ds::String& suffix);
    static ds::String replace_suffix(const ds::String& word, 
                                   const ds::String& old_suffix, 
                                   const ds::String& new_suffix);
    
    // Основные шаги алгоритма Портера
    static ds::String step1(const ds::String& word);
    static ds::String step2(const ds::String& word);
    static ds::String step3(const ds::String& word);
    static ds::String step4(const ds::String& word);
    
public:
    RussianStemmer() = default;
    ~RussianStemmer() = default;
    
    // Основной метод стемминга
    static ds::String stem(const ds::String& word);
    
    // Пакетный стемминг
    static ds::Vector<ds::String> stem_batch(const ds::Vector<ds::String>& words);
    
    // Проверка, нужно ли применять стемминг к слову
    static bool should_stem(const ds::String& word);
    
    // Восстановление исходной формы (простейший вариант)
    static ds::String get_base_form(const ds::String& word, const ds::String& stemmed);
    
    // Статистика стемминга
    struct StemStats {
        size_t words_processed;
        size_t words_stemmed;
        size_t chars_removed;
        double reduction_ratio;
    };
    
    static StemStats calculate_stats(const ds::Vector<ds::String>& original,
                                    const ds::Vector<ds::String>& stemmed);
};

} // namespace search

#endif // STEMMER_H
