#include "boolean_search.h"
#include <iostream>
#include <cassert>
#include <memory>

using namespace search;
using namespace ds;

void test_basic_search() {
    std::cout << "Testing basic search...\n";
    
    // Создаем тестовый индекс
    auto index = std::make_unique<InvertedIndex>();
    
    Document doc1(0, "Test Document 1", "http://test.com/1", 
                  "Московский авиационный институт лучший вуз");
    Document doc2(1, "Test Document 2", "http://test.com/2",
                  "Авиационный институт в Москве");
    Document doc3(2, "Test Document 3", "http://test.com/3",
                  "Технический университет с инженерными специальностями");
    
    index->index_document(doc1);
    index->index_document(doc2);
    index->index_document(doc3);
    
    // Создаем поисковую систему
    BooleanSearch search_engine(std::move(index));
    
    // Простой поиск по термину
    SearchResult result = search_engine.search("авиационный");
    
    assert(result.syntax_valid);
    assert(result.total_found == 2); // Документы 1 и 2
    assert(result.doc_ids.size() == 2);
    
    std::cout << "Query: 'авиационный'\n";
    std::cout << "Found: " << result.total_found << " documents\n";
    std::cout << "Time: " << result.time_ms << " ms\n";
    std::cout << "✓ Basic search passed\n\n";
}

void test_boolean_operators() {
    std::cout << "Testing boolean operators...\n";
    
    auto index = std::make_unique<InvertedIndex>();
    
    Document doc1(0, "Doc 1", "url1", "красный автомобиль быстрый");
    Document doc2(1, "Doc 2", "url2", "синий автомобиль медленный");
    Document doc3(2, "Doc 3", "url3", "красный мотоцикл быстрый");
    
    index->index_document(doc1);
    index->index_document(doc2);
    index->index_document(doc3);
    
    BooleanSearch search_engine(std::move(index));
    
    // AND
    SearchResult and_result = search_engine.search("красный && автомобиль");
    assert(and_result.total_found == 1); // Только doc1
    assert(and_result.doc_ids[0] == 0);
    
    // OR
    SearchResult or_result = search_engine.search("красный || синий");
    assert(or_result.total_found == 3); // Все документы
    
    // NOT
    SearchResult not_result = search_engine.search("автомобиль && !красный");
    assert(not_result.total_found == 1); // Только doc2 (синий автомобиль)
    assert(not_result.doc_ids[0] == 1);
    
    std::cout << "✓ Boolean operators passed\n\n";
}

void test_parentheses() {
    std::cout << "Testing parentheses...\n";
    
    auto index = std::make_unique<InvertedIndex>();
    
    Document doc1(0, "Doc 1", "url1", "кошки собаки домашние животные");
    Document doc2(1, "Doc 2", "url2", "кошки тигры дикие животные");
    Document doc3(2, "Doc 3", "url3", "собаки волки дикие животные");
    
    index->index_document(doc1);
    index->index_document(doc2);
    index->index_document(doc3);
    
    BooleanSearch search_engine(std::move(index));
    
    // Сложный запрос со скобками
    SearchResult result = search_engine.search("(кошки || собаки) && домашние");
    assert(result.total_found == 1); // Только doc1
    assert(result.doc_ids[0] == 0);
    
    std::cout << "✓ Parentheses passed\n\n";
}

void test_phrases() {
    std::cout << "Testing phrase search...\n";
    
    auto index = std::make_unique<InvertedIndex>();
    
    Document doc1(0, "Doc 1", "url1", "Московский авиационный институт основан в 1930 году");
    Document doc2(1, "Doc 2", "url2", "Авиационный институт в Москве называется МАИ");
    Document doc3(2, "Doc 3", "url3", "Московский институт авиационный технический");
    
    index->index_document(doc1);
    index->index_document(doc2);
    index->index_document(doc3);
    
    BooleanSearch search_engine(std::move(index));
    
    // Фразовый поиск
    SearchResult phrase_result = search_engine.search("\"московский авиационный институт\"");
    assert(phrase_result.total_found == 1); // Только doc1 (точная фраза)
    assert(phrase_result.doc_ids[0] == 0);
    
    std::cout << "✓ Phrase search passed\n\n";
}

void test_query_analysis() {
    std::cout << "Testing query analysis...\n";
    
    BooleanSearch search_engine;
    
    auto info = search_engine.analyze_query("(красный || синий) && !медленный");
    
    assert(info.is_valid);
    assert(info.complexity > 0);
    assert(info.terms.size() >= 3); // красный, синий, медленный
    
    std::cout << "Query: " << info.original_query << "\n";
    std::cout << "Complexity: " << info.complexity << "\n";
    std::cout << "Terms: ";
    for (const auto& term : info.terms) {
        std::cout << term << " ";
    }
    std::cout << "\n";
    std::cout << "Parse tree: " << info.parse_tree << "\n";
    
    std::cout << "✓ Query analysis passed\n\n";
}

void test_validation() {
    std::cout << "Testing query validation...\n";
    
    BooleanSearch search_engine;
    
    // Валидные запросы
    assert(search_engine.validate_query("термин"));
    assert(search_engine.validate_query("термин1 && термин2"));
    assert(search_engine.validate_query("(термин1 || термин2) && !термин3"));
    
    // Невалидные запросы
    assert(!search_engine.validate_query("")); // Пустой запрос
    assert(!search_engine.validate_query("&& термин")); // Оператор в начале
    assert(!search_engine.validate_query("термин &&")); // Оператор в конце
    assert(!search_engine.validate_query("()")); // Пустые скобки
    
    std::cout << "✓ Query validation passed\n\n";
}

void test_snippets() {
    std::cout << "Testing snippets...\n";
    
    auto index = std::make_unique<InvertedIndex>();
    
    Document doc(0, "Test Document", "http://test.com/doc", 
                 "Московский авиационный институт является одним из ведущих "
                 "технических вузов России. Он был основан в 1930 году и "
                 "с тех пор готовит высококвалифицированных инженеров.");
    
    index->index_document(doc);
    
    BooleanSearch search_engine(std::move(index));
    
    ds::String snippet = search_engine.get_snippet(0, "авиационный институт", 5);
    
    assert(!snippet.empty());
    assert(snippet.find("[авиационный]") != ds::String::npos ||
           snippet.find("[институт]") != ds::String::npos);
    
    std::cout << "Snippet: " << snippet << "\n";
    std::cout << "✓ Snippets passed\n\n";
}

void test_suggestions() {
    std::cout << "Testing term suggestions...\n";
    
    auto index = std::make_unique<InvertedIndex>();
    
    Document doc1(0, "Doc 1", "url1", "apple application applet approval");
    Document doc2(1, "Doc 2", "url2", "banana berry blueberry blackberry");
    Document doc3(2, "Doc 3", "url3", "application software app mobile");
    
    index->index_document(doc1);
    index->index_document(doc2);
    index->index_document(doc3);
    
    BooleanSearch search_engine(std::move(index));
    
    auto suggestions = search_engine.suggest_terms("app", 5);
    
    // Должны быть предложения, начинающиеся с "app"
    assert(!suggestions.empty());
    
    std::cout << "Suggestions for 'app':\n";
    for (const auto& suggestion : suggestions) {
        std::cout << "  - " << suggestion << "\n";
    }
    
    std::cout << "✓ Term suggestions passed\n\n";
}

void test_stats() {
    std::cout << "Testing search statistics...\n";
    
    auto index = std::make_unique<InvertedIndex>();
    
    Document doc1(0, "Doc 1", "url1", "test content one");
    Document doc2(1, "Doc 2", "url2", "test content two");
    
    index->index_document(doc1);
    index->index_document(doc2);
    
    BooleanSearch search_engine(std::move(index));
    
    // Выполняем несколько запросов
    search_engine.search("test");
    search_engine.search("content");
    search_engine.search("test && content");
    search_engine.search("invalid query!");
    
    const auto& stats = search_engine.get_stats();
    
    assert(stats.total_queries == 4);
    assert(stats.successful_queries == 3);
    assert(stats.failed_queries == 1);
    assert(stats.total_time_ms > 0);
    
    std::cout << "Statistics:\n";
    std::cout << "  Total queries: " << stats.total_queries << "\n";
    std::cout << "  Successful: " << stats.successful_queries << "\n";
    std::cout << "  Failed: " << stats.failed_queries << "\n";
    std::cout << "  Avg time: " << stats.get_average_time() << " ms\n";
    
    // Экспорт статистики
    bool exported = search_engine.export_stats("test_stats.txt");
    assert(exported);
    
    // Удаляем тестовый файл
    std::remove("test_stats.txt");
    
    std::cout << "✓ Search statistics passed\n\n";
}

int main() {
    std::cout << "=== Boolean Search Tests ===\n\n";
    
    try {
        test_basic_search();
        test_boolean_operators();
        test_parentheses();
        test_phrases();
        test_query_analysis();
        test_validation();
        test_snippets();
        test_suggestions();
        test_stats();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}