#ifndef INVERTED_INDEX_H
#define INVERTED_INDEX_H

#include "../../data_structures/ds_vector.h"
#include "../../data_structures/ds_hash_table.h"
#include "../../data_structures/ds_bit_vector.h"
#include "../../data_structures/ds_string.h"
#include "../../tokenizer/src/tokenizer.h"

#include <cstdint>
#include <utility>
#include <fstream>
#include <memory>

namespace search {

// Структура для хранения документа
struct Document {
    uint32_t id;
    ds::String title;
    ds::String url;
    ds::String content;
    size_t length; // количество токенов
    
    Document() : id(0), length(0) {}
    Document(uint32_t doc_id, const ds::String& doc_title, 
             const ds::String& doc_url, const ds::String& doc_content)
        : id(doc_id), title(doc_title), url(doc_url), 
          content(doc_content), length(0) {}
};

// Структура для статистики индекса
struct IndexStats {
    size_t total_documents;
    size_t total_terms;
    size_t total_postings;
    size_t index_size_bytes;
    double avg_document_length;
    double avg_term_frequency;
    size_t unique_terms_per_document;
    size_t most_frequent_term_count;
    ds::String most_frequent_term;
};

// Структура для хранения постинга (вхождения термина в документ)
struct Posting {
    uint32_t doc_id;
    uint32_t frequency; // частота термина в документе
    ds::Vector<uint32_t> positions; // позиции в документе
    
    Posting() : doc_id(0), frequency(0) {}
    Posting(uint32_t id) : doc_id(id), frequency(1) {}
    
    void add_position(uint32_t position) {
        positions.push_back(position);
        frequency = positions.size();
    }
};

// Класс инвертированного индекса
class InvertedIndex {
private:
    // Термин -> список постингов
    ds::HashTable<ds::String, ds::Vector<Posting>> index_;
    
    // Документ -> метаданные
    ds::Vector<Document> documents_;
    
    // Отображение URL -> doc_id
    ds::HashTable<ds::String, uint32_t> url_to_doc_id_;
    
    // Токенизатор
    std::unique_ptr<Tokenizer> tokenizer_;
    
    // Статистика
    IndexStats stats_;
    
    // Вспомогательные методы
    void update_stats();
    
public:
    // Конструкторы
    InvertedIndex();
    explicit InvertedIndex(std::unique_ptr<Tokenizer> tokenizer);
    
    // Деструктор
    ~InvertedIndex() = default;
    
    // Добавление документа
    uint32_t add_document(const Document& document);
    
    // Пакетное добавление документов
    void add_documents(const ds::Vector<Document>& documents);
    
    // Индексация документа (добавление + токенизация + добавление в индекс)
    uint32_t index_document(const Document& document);
    
    // Поиск документов по термину
    const ds::Vector<Posting>* find_postings(const ds::String& term) const;
    
    // Получение документа по ID
    const Document& get_document(uint32_t doc_id) const;
    
    // Получение всех терминов
    ds::Vector<ds::String> get_all_terms() const;
    
    // Получение частоты термина в коллекции
    size_t get_term_frequency(const ds::String& term) const;
    
    // Получение документа по URL
    const Document* get_document_by_url(const ds::String& url) const;
    
    // Получение документа по позиции в списке
    const Document& get_document_by_position(size_t position) const;
    
    // Получение количества документов
    size_t get_document_count() const { return documents_.size(); }
    
    // Получение количества терминов
    size_t get_term_count() const { return stats_.total_terms; }
    
    // Получение статистики
    const IndexStats& get_stats() const { return stats_; }
    
    // Очистка индекса
    void clear();
    
    // Сохранение индекса в файл
    bool save_to_file(const ds::String& filepath) const;
    
    // Загрузка индекса из файла
    bool load_from_file(const ds::String& filepath);
    
    // Экспорт в текстовый формат (для отладки)
    bool export_to_text(const ds::String& filepath) const;
    
    // Проверка целостности индекса
    bool validate() const;
};

} // namespace search

#endif // INVERTED_INDEX_H