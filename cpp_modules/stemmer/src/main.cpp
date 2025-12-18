#include "stemmer.h"
#include <iostream>
#include <fstream>
#include <cstring>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "\nOptions:\n"
              << "  --word <word>     Stem a single word\n"
              << "  --file <file>     Stem words from file (one per line)\n"
              << "  --interactive     Interactive stemming mode\n"
              << "  --output <file>   Output file for stemmed words\n"
              << "  --stats           Show stemming statistics\n"
              << "  --help            Show this help message\n"
              << "\nExample:\n"
              << "  " << program_name << " --word программирование\n"
              << "  " << program_name << " --file words.txt --output stemmed.txt\n";
}

void run_interactive() {
    std::cout << "\n=== Interactive Stemming Mode ===\n";
    std::cout << "Enter words (or 'quit' to exit):\n\n";
    
    while (true) {
        std::cout << "Word> ";
        std::cout.flush();
        
        ds::String word;
        char ch;
        while (std::cin.get(ch)) {
            if (ch == '\n') {
                break;
            }
            word.push_back(ch);
        }
        
        word = word.trim();
        
        if (word.empty()) {
            continue;
        }
        
        if (word == "quit" || word == "exit") {
            break;
        }
        
        // Выполняем стемминг
        ds::String stemmed = search::RussianStemmer::stem(word);
        
        std::cout << "Original: " << word << "\n";
        std::cout << "Stemmed:  " << stemmed << "\n\n";
    }
    
    std::cout << "Goodbye!\n";
}

int main(int argc, char** argv) {
    ds::String word;
    ds::String input_file;
    ds::String output_file;
    bool interactive = false;
    bool show_stats = false;
    
    // Парсим аргументы командной строки
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--word") == 0 && i + 1 < argc) {
            word = argv[++i];
        } else if (std::strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            input_file = argv[++i];
        } else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (std::strcmp(argv[i], "--interactive") == 0) {
            interactive = true;
        } else if (std::strcmp(argv[i], "--stats") == 0) {
            show_stats = true;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    std::cout << "=== Russian Stemmer (Porter Algorithm) ===\n\n";
    
    try {
        // Интерактивный режим
        if (interactive) {
            run_interactive();
            return 0;
        }
        
        // Обработка одного слова
        if (!word.empty()) {
            ds::String stemmed = search::RussianStemmer::stem(word);
            std::cout << "Original: " << word << "\n";
            std::cout << "Stemmed:  " << stemmed << "\n";
            return 0;
        }
        
        // Обработка файла
        if (!input_file.empty()) {
            std::ifstream input(input_file.c_str());
            if (!input.is_open()) {
                std::cerr << "Error: Cannot open input file: " << input_file << "\n";
                return 1;
            }
            
            ds::Vector<ds::String> words;
            ds::Vector<ds::String> stemmed_words;
            
            // Читаем слова из файла
            ds::String line;
            char ch;
            while (input.get(ch)) {
                if (ch == '\n') {
                    if (!line.empty()) {
                        line = line.trim();
                        if (!line.empty()) {
                            words.push_back(line);
                            line.clear();
                        }
                    }
                } else {
                    line.push_back(ch);
                }
            }
            
            // Последняя строка
            if (!line.empty()) {
                line = line.trim();
                if (!line.empty()) {
                    words.push_back(line);
                }
            }
            
            std::cout << "Read " << words.size() << " words from " << input_file << "\n";
            std::cout << "Stemming...\n";
            
            // Стеммим слова
            stemmed_words = search::RussianStemmer::stem_batch(words);
            
            std::cout << "Stemmed " << stemmed_words.size() << " words\n";
            
            // Сохраняем результат
            if (!output_file.empty()) {
                std::ofstream output(output_file.c_str());
                if (!output.is_open()) {
                    std::cerr << "Error: Cannot open output file: " << output_file << "\n";
                    return 1;
                }
                
                for (const auto& stemmed : stemmed_words) {
                    output << stemmed.c_str() << "\n";
                }
                
                std::cout << "Saved stemmed words to " << output_file << "\n";
            } else {
                // Выводим на экран
                for (size_t i = 0; i < words.size() && i < stemmed_words.size(); ++i) {
                    std::cout << words[i] << " -> " << stemmed_words[i] << "\n";
                }
            }
            
            // Статистика
            if (show_stats) {
                auto stats = search::RussianStemmer::calculate_stats(words, stemmed_words);
                std::cout << "\n=== Stemming Statistics ===\n";
                std::cout << "Words processed: " << stats.words_processed << "\n";
                std::cout << "Words stemmed: " << stats.words_stemmed << "\n";
                std::cout << "Characters removed: " << stats.chars_removed << "\n";
                std::cout << "Reduction ratio: " << stats.reduction_ratio << "\n";
            }
            
            return 0;
        }
        
        // Если ничего не указано, показываем справку
        print_usage(argv[0]);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
