#include "../src/tokenizer.h"
#include <iostream>

int main() {
    std::cout << "Tokenizer Test\n";
    
    search::Tokenizer tokenizer;
    
    // Тест 1: базовая токенизация
    ds::String test_text = "This is a simple test";
    auto tokens = tokenizer.tokenize(test_text);
    
    std::cout << "Test 1: Basic tokenization\n";
    std::cout << "  Input: " << test_text << "\n";
    std::cout << "  Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token << " ";
    }
    std::cout << "\n";
    
    // Тест 2: токенизация с позициями
    auto tokens_with_pos = tokenizer.tokenize_with_positions(test_text);
    std::cout << "\nTest 2: Tokenization with positions\n";
    for (const auto& token_info : tokens_with_pos) {
        std::cout << "  '" << token_info.token << "' at position " << token_info.position << "\n";
    }
    
    std::cout << "\n✅ All tests passed!\n";
    
    return 0;
}
