#include "stemmer.h"
#include <algorithm>
#include <cstring>

namespace search {

ds::String RussianStemmer::stem(const ds::String& word) {
    if (word.empty() || word.size() < 3) {
        return word.to_lower();
    }
    
    // Приводим к нижнему регистру
    ds::String result = word.to_lower();
    
    // Удаляем последние 1-2 символа если слово длинное
    if (result.size() > 5) {
        // Проверяем некоторые распространенные окончания
        // Для полной реализации нужна таблица суффиксов
        size_t len = result.size();
        
        // Удаляем до 2 символов с конца
        if (len > 6) {
            result = result.substr(0, len - 2);
        } else if (len > 4) {
            result = result.substr(0, len - 1);
        }
    }
    
    return result;
}

ds::Vector<ds::String> RussianStemmer::stem_batch(const ds::Vector<ds::String>& words) {
    ds::Vector<ds::String> result;
    result.reserve(words.size());
    
    for (const auto& word : words) {
        result.push_back(stem(word));
    }
    
    return result;
}

bool RussianStemmer::should_stem(const ds::String& word) {
    return word.size() >= 3;
}

ds::String RussianStemmer::get_base_form(const ds::String& word, const ds::String& stemmed) {
    // Простое решение - возвращаем исходное слово
    return word;
}

RussianStemmer::StemStats RussianStemmer::calculate_stats(
    const ds::Vector<ds::String>& original,
    const ds::Vector<ds::String>& stemmed) {
    
    StemStats stats;
    stats.words_processed = original.size();
    stats.words_stemmed = 0;
    stats.chars_removed = 0;
    
    for (size_t i = 0; i < original.size() && i < stemmed.size(); ++i) {
        if (original[i] != stemmed[i]) {
            stats.words_stemmed++;
        }
        
        if (original[i].size() > stemmed[i].size()) {
            stats.chars_removed += original[i].size() - stemmed[i].size();
        }
    }
    
    if (stats.words_processed > 0) {
        size_t total_original_chars = 0;
        size_t total_stemmed_chars = 0;
        
        for (const auto& word : original) {
            total_original_chars += word.size();
        }
        
        for (const auto& word : stemmed) {
            total_stemmed_chars += word.size();
        }
        
        stats.reduction_ratio = total_original_chars > 0 ? 
            static_cast<double>(total_stemmed_chars) / total_original_chars : 1.0;
    } else {
        stats.reduction_ratio = 1.0;
    }
    
    return stats;
}

} // namespace search
