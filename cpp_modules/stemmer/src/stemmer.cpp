#include "stemmer.h"
#include <algorithm>
#include <cstring>

namespace search {

// Константы для русского языка
const char RussianStemmer::VOWELS[6] = {'а', 'е', 'и', 'о', 'у', 'ы'};

// Суффиксы совершенного вида деепричастий
const char RussianStemmer::PERFECTIVE_GERUND_SUFFIXES[2][7] = {
    "вшись", "вши"
};

// Суффиксы прилагательных
const char RussianStemmer::ADJECTIVE_SUFFIXES[4][8] = {
    "ими", "ыми", "его", "ого"
};

// Суффиксы причастий
const char RussianStemmer::PARTICIPLE_SUFFIXES[2][7] = {
    "ем", "нн", "вш", "ющ", "щ"
};

// Возвратные суффиксы
const char RussianStemmer::REFLEXIVE_SUFFIXES[2][8] = {
    "ся", "сь"
};

// Суффиксы глаголов
const char RussianStemmer::VERB_SUFFIXES[2][7] = {
    "ила", "ыла", "ена", "ейте", "уйте", "ите", "или", "ыли", "ей", "уй", 
    "ил", "ыл", "им", "ым", "ен", "ило", "ыло", "ено", "ят", "ует", "уют",
    "ит", "ыт", "ны", "ть", "ешь", "нно"
};

// Суффиксы существительных
const char RussianStemmer::NOUN_SUFFIXES[3][9] = {
    "иями", "ями", "ами", "ией", "иям", "ием", "иях", "ев", "ов", "ие", "ье",
    "еи", "ии", "и", "ией", "ей", "ой", "ий", "й", "ия", "ья", "ям", "ем",
    "ам", "ом", "о", "у", "ах", "ях", "и", "ь", "ю", "я", "ок", "ек", "ёнок",
    "ёнк", "онок", "ён", "н", "ок"
};

// Суффиксы, образующие отглагольные существительные
const char RussianStemmer::DERIVATIONAL_SUFFIXES[11] = "ост";

// Суффиксы превосходной степени
const char RussianStemmer::SUPERLATIVE_SUFFIXES[2][8] = {
    "ейш", "ейше"
};

// Проверка, является ли символ гласной
bool RussianStemmer::is_vowel(char c) {
    for (int i = 0; i < 6; ++i) {
        if (c == VOWELS[i]) {
            return true;
        }
    }
    return false;
}

// Проверка, является ли символ согласной
bool RussianStemmer::is_consonant(char c) {
    // Русские согласные (кроме ъ, ь, которые обрабатываются отдельно)
    const char consonants[] = "бвгджзйклмнпрстфхцчшщ";
    for (size_t i = 0; i < strlen(consonants); ++i) {
        if (c == consonants[i]) {
            return true;
        }
    }
    return false;
}

// Нахождение позиции начала RV (региона после первой гласной)
size_t RussianStemmer::rv_position(const ds::String& word) {
    for (size_t i = 0; i < word.size(); ++i) {
        if (is_vowel(word[i])) {
            return i + 1;
        }
    }
    return word.size();
}

// Нахождение позиции начала R2 (региона после следующей гласной после RV)
size_t RussianStemmer::r2_position(const ds::String& word, size_t rv) {
    for (size_t i = rv; i < word.size(); ++i) {
        if (is_vowel(word[i])) {
            return i + 1;
        }
    }
    return word.size();
}

// Проверка окончания слова
bool RussianStemmer::ends_with(const ds::String& word, const ds::String& suffix) {
    if (suffix.size() > word.size()) {
        return false;
    }
    
    size_t offset = word.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i) {
        if (word[offset + i] != suffix[i]) {
            return false;
        }
    }
    return true;
}

// Замена окончания
ds::String RussianStemmer::replace_suffix(const ds::String& word, 
                                        const ds::String& old_suffix, 
                                        const ds::String& new_suffix) {
    if (!ends_with(word, old_suffix)) {
        return word;
    }
    
    ds::String result;
    size_t stem_length = word.size() - old_suffix.size();
    
    // Копируем основу
    for (size_t i = 0; i < stem_length; ++i) {
        result.push_back(word[i]);
    }
    
    // Добавляем новое окончание
    result += new_suffix;
    
    return result;
}

// Шаг 1: Поиск и удаление окончаний и суффиксов
ds::String RussianStemmer::step1(const ds::String& word) {
    ds::String result = word;
    
    // Удаление окончаний совершенного вида деепричастий
    for (int i = 0; i < 2; ++i) {
        ds::String suffix(PERFECTIVE_GERUND_SUFFIXES[i]);
        if (ends_with(result, suffix)) {
            result = replace_suffix(result, suffix, "");
            // После удаления проверяем на наличие возвратных суффиксов
            for (int j = 0; j < 2; ++j) {
                ds::String refl_suffix(REFLEXIVE_SUFFIXES[j]);
                if (ends_with(result, refl_suffix)) {
                    result = replace_suffix(result, refl_suffix, "");
                }
            }
            return result;
        }
    }
    
    // Удаление возвратных суффиксов
    for (int i = 0; i < 2; ++i) {
        ds::String suffix(REFLEXIVE_SUFFIXES[i]);
        if (ends_with(result, suffix)) {
            result = replace_suffix(result, suffix, "");
            break;
        }
    }
    
    // Удаление прилагательных
    for (int i = 0; i < 4; ++i) {
        ds::String suffix(ADJECTIVE_SUFFIXES[i]);
        if (ends_with(result, suffix)) {
            // Проверяем на причастия
            bool has_participle = false;
            for (int j = 0; j < 5; ++j) {
                ds::String part_suffix(PARTICIPLE_SUFFIXES[j/2][j%2]);
                if (ends_with(result, part_suffix)) {
                    result = replace_suffix(result, part_suffix, "");
                    has_participle = true;
                    break;
                }
            }
            
            if (!has_participle) {
                // Удаляем суффиксы прилагательных
                result = replace_suffix(result, suffix, "");
            }
            break;
        }
    }
    
    return result;
}

// Шаг 2: Удаление глагольных окончаний
ds::String RussianStemmer::step2(const ds::String& word) {
    ds::String result = word;
    
    // Проверяем, оканчивается ли слово на гласную
    if (result.empty() || !is_vowel(result.back())) {
        return result;
    }
    
    // Массив глагольных окончаний (часть из VERB_SUFFIXES)
    const char* verb_endings[] = {
        "ила", "ыла", "ена", "ейте", "уйте", "ите", "или", "ыли", 
        "ей", "уй", "ил", "ыл", "им", "ым", "ен", "ило", "ыло", 
        "ено", "ят", "ует", "уют", "ит", "ыт"
    };
    
    for (int i = 0; i < 23; ++i) {
        ds::String suffix(verb_endings[i]);
        if (ends_with(result, suffix)) {
            // Проверяем, что перед суффиксом согласная
            if (result.size() > suffix.size()) {
                char before = result[result.size() - suffix.size() - 1];
                if (is_consonant(before)) {
                    result = replace_suffix(result, suffix, "");
                    return result;
                }
            }
        }
    }
    
    return result;
}

// Шаг 3: Удаление суффиксов существительных
ds::String RussianStemmer::step3(const ds::String& word) {
    ds::String result = word;
    
    // Массив суффиксов существительных (часть из NOUN_SUFFIXES)
    const char* noun_suffixes[] = {
        "иями", "ями", "ами", "ией", "иям", "ием", "иях", 
        "ев", "ов", "ие", "ье", "еи", "ии", "и", "ией", 
        "ей", "ой", "ий", "й", "ия", "ья", "ям", "ем", 
        "ам", "ом", "о", "у", "ах", "ях"
    };
    
    // Сортируем по длине (от самых длинных к коротким)
    for (int i = 0; i < 29; ++i) {
        ds::String suffix(noun_suffixes[i]);
        if (ends_with(result, suffix)) {
            result = replace_suffix(result, suffix, "");
            
            // Если слово стало слишком коротким, восстанавливаем
            if (result.size() < 2) {
                return word;
            }
            break;
        }
    }
    
    return result;
}

// Шаг 4: Окончательная нормализация
ds::String RussianStemmer::step4(const ds::String& word) {
    ds::String result = word;
    
    // Удаление мягкого знака на конце
    if (!result.empty() && result.back() == 'ь') {
        result.pop_back();
    }
    
    // Удаление удвоенных согласных на конце
    if (result.size() >= 2) {
        char last = result.back();
        char prev = result[result.size() - 2];
        
        // Проверяем на удвоенные согласные
        if (last == prev && is_consonant(last)) {
            // Удаляем последнюю согласную
            result.pop_back();
        }
    }
    
    // Удаление суффиксов превосходной степени
    for (int i = 0; i < 2; ++i) {
        ds::String suffix(SUPERLATIVE_SUFFIXES[i]);
        if (ends_with(result, suffix)) {
            result = replace_suffix(result, suffix, "");
            break;
        }
    }
    
    // Удаление производных суффиксов
    ds::String deriv_suffix(DERIVATIONAL_SUFFIXES);
    if (ends_with(result, deriv_suffix)) {
        result = replace_suffix(result, deriv_suffix, "");
    }
    
    return result;
}

// Основной метод стемминга
ds::String RussianStemmer::stem(const ds::String& word) {
    if (word.empty() || word.size() < 2) {
        return word;
    }
    
    // Приводим к нижнему регистру
    ds::String lower_word = word.to_lower();
    
    // Проверяем, нужно ли применять стемминг
    if (!should_stem(lower_word)) {
        return lower_word;
    }
    
    // Применяем шаги алгоритма Портера
    ds::String result = lower_word;
    
    // Шаг 1
    ds::String step1_result = step1(result);
    if (step1_result != result) {
        result = step1_result;
    }
    
    // Шаг 2
    ds::String step2_result = step2(result);
    if (step2_result != result) {
        result = step2_result;
    } else {
        // Если шаг 2 не применился, пробуем шаг 2а (альтернативные окончания)
        // Здесь можно добавить дополнительные правила
    }
    
    // Шаг 3
    ds::String step3_result = step3(result);
    if (step3_result != result) {
        result = step3_result;
    }
    
    // Шаг 4
    ds::String step4_result = step4(result);
    if (step4_result != result) {
        result = step4_result;
    }
    
    // Окончательная проверка: если слово стало слишком коротким, возвращаем исходное
    if (result.size() < 2) {
        return lower_word;
    }
    
    return result;
}

// Пакетный стемминг
ds::Vector<ds::String> RussianStemmer::stem_batch(const ds::Vector<ds::String>& words) {
    ds::Vector<ds::String> result;
    result.reserve(words.size());
    
    for (const auto& word : words) {
        result.push_back(stem(word));
    }
    
    return result;
}

// Проверка, нужно ли применять стемминг к слову
bool RussianStemmer::should_stem(const ds::String& word) {
    if (word.empty() || word.size() < 3) {
        return false;
    }
    
    // Проверяем, содержит ли слово русские буквы
    bool has_russian = false;
    for (char c : word) {
        if ((c >= 'а' && c <= 'я') || c == 'ё') {
            has_russian = true;
            break;
        }
    }
    
    if (!has_russian) {
        return false;
    }
    
    // Не стеммим короткие слова и аббревиатуры
    if (word.size() <= 3) {
        return false;
    }
    
    // Проверяем, не является ли слово числом
    bool is_number = true;
    for (char c : word) {
        if (!(c >= '0' && c <= '9')) {
            is_number = false;
            break;
        }
    }
    
    if (is_number) {
        return false;
    }
    
    // Проверяем на наличие заглавных букв (возможно, аббревиатура)
    for (char c : word) {
        if (c >= 'А' && c <= 'Я') {
            return false;
        }
    }
    
    return true;
}

// Восстановление исходной формы (упрощенный вариант)
ds::String RussianStemmer::get_base_form(const ds::String& word, const ds::String& stemmed) {
    // В реальном стеммере это сложная задача
    // Здесь просто возвращаем стем, так как это учебный пример
    return stemmed;
}

// Статистика стемминга
RussianStemmer::StemStats RussianStemmer::calculate_stats(
    const ds::Vector<ds::String>& original,
    const ds::Vector<ds::String>& stemmed) {
    
    StemStats stats = {0, 0, 0, 0.0};
    
    if (original.size() != stemmed.size()) {
        return stats;
    }
    
    stats.words_processed = original.size();
    
    for (size_t i = 0; i < original.size(); ++i) {
        if (original[i] != stemmed[i]) {
            stats.words_stemmed++;
            stats.chars_removed += (original[i].size() - stemmed[i].size());
        }
    }
    
    if (stats.words_stemmed > 0) {
        stats.reduction_ratio = static_cast<double>(stats.chars_removed) / 
                               (stats.words_stemmed * 1.0);
    }
    
    return stats;
}

} // namespace search
