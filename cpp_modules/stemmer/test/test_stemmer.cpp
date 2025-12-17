#include "stemmer.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace search;
using namespace ds;

void test_basic_stemming() {
    std::cout << "Testing basic stemming...\n";
    
    // Тестовые пары: исходное слово -> стем
    std::vector<std::pair<ds::String, ds::String>> test_cases = {
        {"бегая", "бег"},
        {"бегав", "бег"},
        {"бегающий", "бег"},
        {"бегавши", "бег"},
        {"бегавшись", "бег"},
        {"красивый", "красив"},
        {"красивая", "красив"},
        {"красивое", "красив"},
        {"красивые", "красив"},
        {"читающий", "чит"},
        {"читавший", "чит"},
        {"читаемый", "чит"},
        {"прочитанный", "прочит"},
        {"умывающийся", "умыв"},
        {"умывавшийся", "умыв"},
    };
    
    size_t passed = 0;
    for (const auto& test_case : test_cases) {
        ds::String result = RussianStemmer::stem(test_case.first);
        
        if (result == test_case.second) {
            passed++;
        } else {
            std::cout << "  FAIL: " << test_case.first << " -> " << result 
                     << " (expected: " << test_case.second << ")\n";
        }
    }
    
    double accuracy = static_cast<double>(passed) / test_cases.size() * 100;
    std::cout << "  Accuracy: " << passed << "/" << test_cases.size() 
              << " (" << accuracy << "%)\n";
    
    assert(passed > test_cases.size() * 0.7); // Минимум 70% точности
    std::cout << "✓ Basic stemming passed\n\n";
}

void test_noun_stemming() {
    std::cout << "Testing noun stemming...\n";
    
    std::vector<std::pair<ds::String, ds::String>> test_cases = {
        {"студенты", "студент"},
        {"студентами", "студент"},
        {"студента", "студент"},
        {"студентом", "студент"},
        {"институты", "институт"},
        {"института", "институт"},
        {"институтом", "институт"},
        {"книги", "книг"},
        {"книга", "книг"},
        {"книгой", "книг"},
        {"дома", "дом"},
        {"дому", "дом"},
        {"домом", "дом"},
    };
    
    size_t passed = 0;
    for (const auto& test_case : test_cases) {
        ds::String result = RussianStemmer::stem(test_case.first);
        
        if (result == test_case.second) {
            passed++;
        } else {
            std::cout << "  FAIL: " << test_case.first << " -> " << result 
                     << " (expected: " << test_case.second << ")\n";
        }
    }
    
    double accuracy = static_cast<double>(passed) / test_cases.size() * 100;
    std::cout << "  Accuracy: " << passed << "/" << test_cases.size() 
              << " (" << accuracy << "%)\n";
    
    assert(passed > test_cases.size() * 0.7);
    std::cout << "✓ Noun stemming passed\n\n";
}

void test_adjective_stemming() {
    std::cout << "Testing adjective stemming...\n";
    
    std::vector<std::pair<ds::String, ds::String>> test_cases = {
        {"красивые", "красив"},
        {"красивого", "красив"},
        {"красивому", "красив"},
        {"красивым", "красив"},
        {"интересные", "интересн"},
        {"интересного", "интересн"},
        {"интересному", "интересн"},
        {"интересным", "интересн"},
        {"большие", "больш"},
        {"большого", "больш"},
        {"большому", "больш"},
        {"большим", "больш"},
    };
    
    size_t passed = 0;
    for (const auto& test_case : test_cases) {
        ds::String result = RussianStemmer::stem(test_case.first);
        
        if (result == test_case.second) {
            passed++;
        } else {
            std::cout << "  FAIL: " << test_case.first << " -> " << result 
                     << " (expected: " << test_case.second << ")\n";
        }
    }
    
    double accuracy = static_cast<double>(passed) / test_cases.size() * 100;
    std::cout << "  Accuracy: " << passed << "/" << test_cases.size() 
              << " (" << accuracy << "%)\n";
    
    assert(passed > test_cases.size() * 0.7);
    std::cout << "✓ Adjective stemming passed\n\n";
}

void test_verb_stemming() {
    std::cout << "Testing verb stemming...\n";
    
    std::vector<std::pair<ds::String, ds::String>> test_cases = {
        {"читать", "чит"},
        {"читаю", "чит"},
        {"читаешь", "чит"},
        {"читает", "чит"},
        {"читаем", "чит"},
        {"читаете", "чит"},
        {"читают", "чит"},
        {"писать", "пис"},
        {"пишу", "пис"},
        {"пишешь", "пис"},
        {"пишет", "пис"},
        {"пишем", "пис"},
        {"пишете", "пис"},
        {"пишут", "пис"},
        {"говорить", "говор"},
        {"говорю", "говор"},
        {"говоришь", "говор"},
        {"говорит", "говор"},
        {"говорим", "говор"},
        {"говорите", "говор"},
        {"говорят", "говор"},
    };
    
    size_t passed = 0;
    for (const auto& test_case : test_cases) {
        ds::String result = RussianStemmer::stem(test_case.first);
        
        if (result == test_case.second) {
            passed++;
        } else {
            std::cout << "  FAIL: " << test_case.first << " -> " << result 
                     << " (expected: " << test_case.second << ")\n";
        }
    }
    
    double accuracy = static_cast<double>(passed) / test_cases.size() * 100;
    std::cout << "  Accuracy: " << passed << "/" << test_cases.size() 
              << " (" << accuracy << "%)\n";
    
    assert(passed > test_cases.size() * 0.6); // Глаголы сложнее
    std::cout << "✓ Verb stemming passed\n\n";
}

void test_batch_stemming() {
    std::cout << "Testing batch stemming...\n";
    
    ds::Vector<ds::String> words = {
        "студенты", "института", "читающий", "красивые", 
        "говорить", "большого", "писать", "интересные"
    };
    
    ds::Vector<ds::String> stemmed = RussianStemmer::stem_batch(words);
    
    assert(words.size() == stemmed.size());
    
    std::cout << "Original -> Stemmed:\n";
    for (size_t i = 0; i < words.size(); ++i) {
        std::cout << "  " << words[i] << " -> " << stemmed[i] << "\n";
    }
    
    // Проверяем, что все слова были изменены (кроме возможно очень коротких)
    size_t changed = 0;
    for (size_t i = 0; i < words.size(); ++i) {
        if (words[i] != stemmed[i]) {
            changed++;
        }
    }
    
    assert(changed >= words.size() * 0.8); // Минимум 80% слов должны измениться
    
    std::cout << "✓ Batch stemming passed\n\n";
}

void test_should_stem() {
    std::cout << "Testing should_stem logic...\n";
    
    // Слова, которые ДОЛЖНЫ стеммиться
    ds::Vector<ds::String> should_stem = {
        "студенты", "читающий", "красивые", "говорить", 
        "института", "большого", "интересные"
    };
    
    // Слова, которые НЕ должны стеммиться
    ds::Vector<ds::String> should_not_stem = {
        "кот", "дом", "стол", // Слишком короткие
        "123", "45.6", // Числа
        "C++", "Python", // Иностранные слова
        "и", "в", "на", // Союзы и предлоги
    };
    
    size_t correct_should = 0;
    for (const auto& word : should_stem) {
        if (RussianStemmer::should_stem(word)) {
            correct_should++;
        }
    }
    
    size_t correct_should_not = 0;
    for (const auto& word : should_not_stem) {
        if (!RussianStemmer::should_stem(word)) {
            correct_should_not++;
        }
    }
    
    double accuracy_should = static_cast<double>(correct_should) / should_stem.size() * 100;
    double accuracy_should_not = static_cast<double>(correct_should_not) / should_not_stem.size() * 100;
    
    std::cout << "  Should stem: " << correct_should << "/" << should_stem.size() 
              << " (" << accuracy_should << "%)\n";
    std::cout << "  Should NOT stem: " << correct_should_not << "/" << should_not_stem.size() 
              << " (" << accuracy_should_not << "%)\n";
    
    assert(correct_should >= should_stem.size() * 0.8);
    assert(correct_should_not >= should_not_stem.size() * 0.8);
    
    std::cout << "✓ Should_stem logic passed\n\n";
}

void test_stats_calculation() {
    std::cout << "Testing statistics calculation...\n";
    
    ds::Vector<ds::String> original = {
        "студенты", "института", "читающий", "красивые", 
        "говорить", "большого", "интересные"
    };
    
    ds::Vector<ds::String> stemmed = RussianStemmer::stem_batch(original);
    
    auto stats = RussianStemmer::calculate_stats(original, stemmed);
    
    std::cout << "Statistics:\n";
    std::cout << "  Words processed: " << stats.words_processed << "\n";
    std::cout << "  Words stemmed: " << stats.words_stemmed << "\n";
    std::cout << "  Characters removed: " << stats.chars_removed << "\n";
    std::cout << "  Reduction ratio: " << stats.reduction_ratio << " chars/word\n";
    
    assert(stats.words_processed == original.size());
    assert(stats.words_stemmed > 0);
    assert(stats.chars_removed > 0);
    assert(stats.reduction_ratio > 0.5); // В среднем удаляем хотя бы 0.5 символа на слово
    
    std::cout << "✓ Statistics calculation passed\n\n";
}

void test_edge_cases() {
    std::cout << "Testing edge cases...\n";
    
    // Пустая строка
    assert(RussianStemmer::stem("") == "");
    
    // Одна буква
    assert(RussianStemmer::stem("а") == "а");
    
    // Две буквы
    assert(RussianStemmer::stem("он") == "он");
    
    // Строка без русских букв
    assert(RussianStemmer::stem("hello") == "hello");
    
    // Строка с цифрами
    assert(RussianStemmer::stem("word123") == "word123");
    
    // Смешанный регистр
    assert(RussianStemmer::stem("СтУдЕнТы") == "студент");
    
    // Слово с ё
    ds::String with_yo = RussianStemmer::stem("ёжик");
    assert(!with_yo.empty());
    
    std::cout << "✓ Edge cases passed\n\n";
}

void test_performance() {
    std::cout << "Testing performance...\n";
    
    // Генерируем тестовые слова
    ds::Vector<ds::String> test_words;
    const char* bases[] = {"студент", "институт", "книг", "дом", "красив", 
                          "интересн", "чит", "пис", "говор", "больш"};
    const char* endings[] = {"", "а", "у", "ом", "ы", "ов", "ами", "ах"};
    
    for (const char* base : bases) {
        for (const char* ending : endings) {
            ds::String word = ds::String(base) + ending;
            test_words.push_back(word);
        }
    }
    
    // Добавляем еще слова для разнообразия
    const char* extra_words[] = {
        "читающий", "читавший", "читаемый", "прочитанный",
        "умывающийся", "умывавшийся", "умывающаяся", "умывавшаяся",
        "самый", "самая", "самое", "самые",
        "лучший", "лучшая", "лучшее", "лучшие",
        nullptr
    };
    
    for (int i = 0; extra_words[i] != nullptr; ++i) {
        test_words.push_back(extra_words[i]);
    }
    
    std::cout << "  Testing " << test_words.size() << " words...\n";
    
    // Измеряем время
    auto start_time = std::chrono::high_resolution_clock::now();
    
    ds::Vector<ds::String> results = RussianStemmer::stem_batch(test_words);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double words_per_second = test_words.size() / (duration.count() / 1000000.0);
    
    std::cout << "  Time: " << duration.count() << " microseconds\n";
    std::cout << "  Speed: " << words_per_second << " words/sec\n";
    
    // Проверяем, что все слова обработаны
    assert(results.size() == test_words.size());
    
    // Проверяем, что стемминг что-то изменил хотя бы в части слов
    size_t changed = 0;
    for (size_t i = 0; i < test_words.size(); ++i) {
        if (test_words[i] != results[i]) {
            changed++;
        }
    }
    
    std::cout << "  Words changed: " << changed << "/" << test_words.size() 
              << " (" << (changed * 100.0 / test_words.size()) << "%)\n";
    
    assert(changed > test_words.size() * 0.5); // Хотя бы половина слов должна измениться
    
    std::cout << "✓ Performance test passed\n\n";
}

int main() {
    std::cout << "=== Russian Stemmer Tests ===\n\n";
    
    try {
        test_basic_stemming();
        test_noun_stemming();
        test_adjective_stemming();
        test_verb_stemming();
        test_batch_stemming();
        test_should_stem();
        test_stats_calculation();
        test_edge_cases();
        test_performance();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}