#include "inverted_index.h"
#include "index_builder.h"
#include <iostream>
#include <cassert>
#include <memory>

using namespace search;
using namespace ds;

void test_basic_indexing() {
    std::cout << "Testing basic indexing...\n";
    
    InvertedIndex index;
    
    // Создаем тестовые документы
    Document doc1(0, "Документ 1", "http://example.com/doc1", 
                  "Московский авиационный институт лучший");
    Document doc2(1, "Документ 2", "http://example.com/doc2",
                  "Авиационный институт в Москве");
    Document doc3(2, "Документ 3", "http://example.com/doc3",
                  "Технический университет с авиационной специализацией");
    
    // Индексируем документы
    index.index_document(doc1);
    index.index_document(doc2);
    index.index_document(doc3);
    
    // Проверяем количество документов
    assert(index.get_document_count() == 3);
    
    // Проверяем поиск по термину
    const auto* postings = index.find_postings("авиационный");
    assert(postings != nullptr);
    assert(postings->size() == 2); // В документах 1 и 2
    
    postings = index.find_postings("московский");
    assert(postings != nullptr);
    assert(postings->size() == 1); // Только в документе 1
    
    postings = index.find_postings("институт");
    assert(postings != nullptr);
    assert(postings->size() == 2); // В документах 1 и 2
    
    std::cout << "✓ Basic indexing passed\n\n";
}

void test_document_retrieval() {
    std::cout << "Testing document retrieval...\n";
    
    InvertedIndex index;
    
    Document doc1(0, "Test Document", "http://test.com/doc1", "Test content");
    index.index_document(doc1);
    
    // Получаем документ по ID
    const Document& retrieved = index.get_document(0);
    assert(retrieved.title == "Test Document");
    assert(retrieved.url == "http://test.com/doc1");
    
    // Получаем документ по URL
    const Document* by_url = index.get_document_by_url("http://test.com/doc1");
    assert(by_url != nullptr);
    assert(by_url->title == "Test Document");
    
    std::cout << "✓ Document retrieval passed\n\n";
}

void test_index_stats() {
    std::cout << "Testing index statistics...\n";
    
    InvertedIndex index;
    
    Document doc1(0, "Doc 1", "url1", "word1 word2 word3");
    Document doc2(1, "Doc 2", "url2", "word2 word3 word4");
    Document doc3(2, "Doc 3", "url3", "word3 word4 word5");
    
    index.index_document(doc1);
    index.index_document(doc2);
    index.index_document(doc3);
    
    const auto& stats = index.get_stats();
    
    assert(stats.total_documents == 3);
    assert(stats.total_terms >= 5); // word1-word5
    assert(stats.total_postings == 9); // 3 документа * 3 уникальных слова каждый
    
    std::cout << "Statistics:\n";
    std::cout << "  Documents: " << stats.total_documents << "\n";
    std::cout << "  Terms: " << stats.total_terms << "\n";
    std::cout << "  Postings: " << stats.total_postings << "\n";
    std::cout << "  Avg doc length: " << stats.avg_document_length << "\n";
    
    std::cout << "✓ Index statistics passed\n\n";
}

void test_save_load() {
    std::cout << "Testing save/load functionality...\n";
    
    InvertedIndex index;
    
    // Создаем и индексируем тестовые документы
    Document doc1(0, "Save Test 1", "http://save.com/1", "Test content for saving");
    Document doc2(1, "Save Test 2", "http://save.com/2", "Another test content");
    
    index.index_document(doc1);
    index.index_document(doc2);
    
    // Сохраняем индекс
    bool saved = index.save_to_file("test_index.bin");
    assert(saved);
    
    // Загружаем индекс в новый объект
    InvertedIndex loaded_index;
    bool loaded = loaded_index.load_from_file("test_index.bin");
    assert(loaded);
    
    // Проверяем, что данные совпадают
    assert(loaded_index.get_document_count() == 2);
    
    const Document& loaded_doc = loaded_index.get_document(0);
    assert(loaded_doc.title == "Save Test 1");
    assert(loaded_doc.url == "http://save.com/1");
    
    // Проверяем поиск
    const auto* postings = loaded_index.find_postings("test");
    assert(postings != nullptr);
    
    // Удаляем тестовый файл
    std::remove("test_index.bin");
    
    std::cout << "✓ Save/load functionality passed\n\n";
}

void test_index_builder() {
    std::cout << "Testing index builder...\n";
    
    IndexBuilder builder;
    
    // Создаем тестовые документы
    ds::Vector<Document> documents;
    
    for (int i = 0; i < 10; ++i) {
        Document doc;
        doc.id = i;
        doc.title = "Document " + ds::String(std::to_string(i).c_str());
        doc.url = "http://test.com/doc" + ds::String(std::to_string(i).c_str());
        doc.content = "This is test document number " + ds::String(std::to_string(i).c_str());
        
        documents.push_back(doc);
    }
    
    // Строим индекс
    bool built = builder.build_from_documents(documents);
    assert(built);
    
    // Получаем статистику построения
    const auto& stats = builder.get_build_stats();
    assert(stats.documents_processed == 10);
    
    // Получаем индекс
    auto index = builder.get_index();
    assert(index->get_document_count() == 10);
    
    std::cout << "Build statistics:\n";
    std::cout << "  Documents processed: " << stats.documents_processed << "\n";
    std::cout << "  Build time: " << stats.build_time.count() << " ms\n";
    std::cout << "  Speed: " << stats.speed_docs_per_sec << " docs/sec\n";
    
    std::cout << "✓ Index builder passed\n\n";
}

void test_term_frequency() {
    std::cout << "Testing term frequency calculation...\n";
    
    InvertedIndex index;
    
    Document doc1(0, "Freq Test", "url1", "word word word repeat repeat");
    Document doc2(1, "Freq Test 2", "url2", "word repeat another");
    
    index.index_document(doc1);
    index.index_document(doc2);
    
    // Проверяем частоту терминов
    size_t word_freq = index.get_term_frequency("word");
    assert(word_freq == 4); // 3 в первом документе + 1 во втором
    
    size_t repeat_freq = index.get_term_frequency("repeat");
    assert(repeat_freq == 3); // 2 в первом + 1 во втором
    
    size_t another_freq = index.get_term_frequency("another");
    assert(another_freq == 1);
    
    size_t missing_freq = index.get_term_frequency("missing");
    assert(missing_freq == 0);
    
    std::cout << "✓ Term frequency calculation passed\n\n";
}

void test_export_text() {
    std::cout << "Testing text export...\n";
    
    InvertedIndex index;
    
    Document doc(0, "Export Test", "http://export.com/test", 
                 "Simple test document for export");
    
    index.index_document(doc);
    
    // Экспортируем в текстовый формат
    bool exported = index.export_to_text("test_export.txt");
    assert(exported);
    
    // Проверяем, что файл создан
    std::ifstream file("test_export.txt");
    assert(file.is_open());
    
    // Читаем первую строку
    std::string line;
    std::getline(file, line);
    assert(line.find("Boolean Index Statistics") != std::string::npos);
    
    file.close();
    
    // Удаляем тестовый файл
    std::remove("test_export.txt");
    
    std::cout << "✓ Text export passed\n\n";
}

void test_index_validation() {
    std::cout << "Testing index validation...\n";
    
    InvertedIndex index;
    
    // Создаем валидный индекс
    Document doc1(0, "Valid Doc", "http://valid.com/1", "Valid content");
    Document doc2(1, "Another Valid", "http://valid.com/2", "More content");
    
    index.index_document(doc1);
    index.index_document(doc2);
    
    // Проверяем валидность
    bool valid = index.validate();
    assert(valid);
    
    std::cout << "✓ Index validation passed\n\n";
}

int main() {
    std::cout << "=== Boolean Index Tests ===\n\n";
    
    try {
        test_basic_indexing();
        test_document_retrieval();
        test_index_stats();
        test_save_load();
        test_index_builder();
        test_term_frequency();
        test_export_text();
        test_index_validation();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}