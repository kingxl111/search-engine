#ifndef INDEX_BUILDER_H
#define INDEX_BUILDER_H

#include "inverted_index.h"
#include <memory>
#include <chrono>

namespace search {

// Класс для управления процессом построения индекса
class IndexBuilder {
private:
    std::unique_ptr<InvertedIndex> index_;
    std::unique_ptr<Tokenizer> tokenizer_;
    
    // Статистика построения
    struct BuildStats {
        size_t documents_processed;
        size_t documents_skipped;
        size_t total_tokens;
        size_t unique_tokens;
        std::chrono::milliseconds build_time;
        double speed_docs_per_sec;
        double speed_tokens_per_sec;
    };
    
    BuildStats stats_;
    
    // Вспомогательные методы
    void update_stats(size_t docs_processed, size_t tokens_processed, 
                      const std::chrono::milliseconds& elapsed);
    
public:
    // Конструкторы
    IndexBuilder();
    explicit IndexBuilder(std::unique_ptr<Tokenizer> tokenizer);
    
    // Деструктор
    ~IndexBuilder() = default;
    
    // Построение индекса из документов
    bool build_from_documents(const ds::Vector<Document>& documents);
    
    // Построение индекса из файла (формат: одна строка = один документ)
    bool build_from_text_file(const ds::String& filepath);
    
    // Построение индекса из нескольких файлов
    bool build_from_directory(const ds::String& dirpath, 
                              const ds::String& extension = ".txt");
    
    // Построение индекса с чанкованием (для больших коллекций)
    bool build_with_chunking(const ds::Vector<Document>& documents, 
                             size_t chunk_size = 1000);
    
    // Получение построенного индекса
    std::unique_ptr<InvertedIndex> get_index();
    
    // Получение статистики построения
    const BuildStats& get_build_stats() const { return stats_; }
    
    // Сброс статистики
    void reset_stats();
    
    // Экспорт статистики в файл
    bool export_stats(const ds::String& filepath) const;
    
    // Оптимизация индекса (сортировка постингов, слияние и т.д.)
    void optimize_index();
};

} // namespace search

#endif // INDEX_BUILDER_H