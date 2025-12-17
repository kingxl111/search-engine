#include "index_builder.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace search {

using namespace std::chrono;

// Конструкторы
IndexBuilder::IndexBuilder() {
    tokenizer_ = std::make_unique<Tokenizer>();
    index_ = std::make_unique<InvertedIndex>(std::make_unique<Tokenizer>());
    reset_stats();
}

IndexBuilder::IndexBuilder(std::unique_ptr<Tokenizer> tokenizer) 
    : tokenizer_(std::move(tokenizer)) {
    index_ = std::make_unique<InvertedIndex>(std::move(tokenizer_));
    reset_stats();
}

// Обновление статистики
void IndexBuilder::update_stats(size_t docs_processed, size_t tokens_processed,
                                const milliseconds& elapsed) {
    stats_.documents_processed += docs_processed;
    stats_.total_tokens += tokens_processed;
    stats_.build_time += elapsed;
    
    if (elapsed.count() > 0) {
        double seconds = elapsed.count() / 1000.0;
        stats_.speed_docs_per_sec = docs_processed / seconds;
        stats_.speed_tokens_per_sec = tokens_processed / seconds;
    }
}

// Построение индекса из документов
bool IndexBuilder::build_from_documents(const ds::Vector<Document>& documents) {
    if (documents.empty()) {
        return false;
    }
    
    auto start_time = high_resolution_clock::now();
    
    // Сбрасываем индекс
    index_->clear();
    
    size_t processed_count = 0;
    size_t total_tokens = 0;
    
    for (const auto& doc : documents) {
        try {
            // Индексируем документ
            uint32_t doc_id = index_->index_document(doc);
            processed_count++;
            
            // Получаем количество токенов в документе (приблизительно)
            auto tokens = tokenizer_->tokenize(doc.content);
            total_tokens += tokens.size();
            
            // Прогресс
            if (processed_count % 100 == 0) {
                auto current_time = high_resolution_clock::now();
                auto elapsed = duration_cast<milliseconds>(current_time - start_time);
                
                double progress = (processed_count * 100.0) / documents.size();
                double speed = processed_count / (elapsed.count() / 1000.0);
                
                std::cout << "Processed " << processed_count << "/" << documents.size() 
                         << " documents (" << progress << "%) - "
                         << speed << " docs/sec\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error indexing document: " << e.what() << "\n";
            stats_.documents_skipped++;
        }
    }
    
    auto end_time = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(end_time - start_time);
    
    update_stats(processed_count, total_tokens, elapsed);
    
    // Обновляем статистику уникальных токенов
    stats_.unique_tokens = index_->get_term_count();
    
    return processed_count > 0;
}

// Построение индекса из файла
bool IndexBuilder::build_from_text_file(const ds::String& filepath) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    ds::Vector<Document> documents;
    ds::String line;
    char ch;
    uint32_t doc_id = 0;
    
    auto start_time = high_resolution_clock::now();
    
    while (file.get(ch)) {
        if (ch == '\n') {
            if (!line.empty()) {
                // Создаем документ из строки
                Document doc;
                doc.id = doc_id++;
                doc.title = "Document " + ds::String(std::to_string(doc_id).c_str());
                doc.url = "file://" + filepath + "#" + ds::String(std::to_string(doc_id).c_str());
                doc.content = line;
                
                documents.push_back(doc);
                line.clear();
                
                // Ограничиваем размер для обработки
                if (documents.size() >= 10000) {
                    build_from_documents(documents);
                    documents.clear();
                }
            }
        } else {
            line.push_back(ch);
        }
    }
    
    // Обрабатываем последнюю строку
    if (!line.empty()) {
        Document doc;
        doc.id = doc_id++;
        doc.title = "Document " + ds::String(std::to_string(doc_id).c_str());
        doc.url = "file://" + filepath + "#" + ds::String(std::to_string(doc_id).c_str());
        doc.content = line;
        documents.push_back(doc);
    }
    
    // Обрабатываем оставшиеся документы
    if (!documents.empty()) {
        build_from_documents(documents);
    }
    
    auto end_time = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(end_time - start_time);
    
    std::cout << "Built index from " << filepath << " in " 
              << elapsed.count() << " ms\n";
    
    return true;
}

// Построение индекса из директории
bool IndexBuilder::build_from_directory(const ds::String& dirpath, 
                                        const ds::String& extension) {
    // В реальной реализации здесь был бы код для обхода директории
    // Для простоты предполагаем, что все файлы уже загружены
    std::cout << "Directory building not fully implemented in this example\n";
    return false;
}

// Построение индекса с чанкованием
bool IndexBuilder::build_with_chunking(const ds::Vector<Document>& documents, 
                                       size_t chunk_size) {
    if (documents.empty()) {
        return false;
    }
    
    auto start_time = high_resolution_clock::now();
    
    // Сбрасываем статистику
    reset_stats();
    
    size_t total_chunks = (documents.size() + chunk_size - 1) / chunk_size;
    size_t processed_total = 0;
    size_t tokens_total = 0;
    
    for (size_t chunk_start = 0; chunk_start < documents.size(); chunk_start += chunk_size) {
        size_t chunk_end = std::min(chunk_start + chunk_size, documents.size());
        
        // Создаем подмассив документов для этого чанка
        ds::Vector<Document> chunk;
        for (size_t i = chunk_start; i < chunk_end; ++i) {
            chunk.push_back(documents[i]);
        }
        
        // Строим индекс для чанка
        size_t chunk_processed = 0;
        size_t chunk_tokens = 0;
        
        auto chunk_start_time = high_resolution_clock::now();
        
        for (const auto& doc : chunk) {
            try {
                uint32_t doc_id = index_->index_document(doc);
                chunk_processed++;
                
                auto tokens = tokenizer_->tokenize(doc.content);
                chunk_tokens += tokens.size();
            } catch (...) {
                stats_.documents_skipped++;
            }
        }
        
        auto chunk_end_time = high_resolution_clock::now();
        auto chunk_elapsed = duration_cast<milliseconds>(chunk_end_time - chunk_start_time);
        
        update_stats(chunk_processed, chunk_tokens, chunk_elapsed);
        processed_total += chunk_processed;
        tokens_total += chunk_tokens;
        
        // Прогресс
        std::cout << "Chunk " << (chunk_start / chunk_size + 1) << "/" << total_chunks
                 << " processed (" << chunk_processed << " documents, "
                 << chunk_elapsed.count() << " ms)\n";
    }
    
    auto end_time = high_resolution_clock::now();
    auto total_elapsed = duration_cast<milliseconds>(end_time - start_time);
    
    stats_.build_time = total_elapsed;
    stats_.unique_tokens = index_->get_term_count();
    
    std::cout << "Total: " << processed_total << " documents, " 
              << tokens_total << " tokens, " 
              << stats_.unique_tokens << " unique terms\n";
    
    return processed_total > 0;
}

// Получение построенного индекса
std::unique_ptr<InvertedIndex> IndexBuilder::get_index() {
    return std::move(index_);
}

// Сброс статистики
void IndexBuilder::reset_stats() {
    stats_.documents_processed = 0;
    stats_.documents_skipped = 0;
    stats_.total_tokens = 0;
    stats_.unique_tokens = 0;
    stats_.build_time = milliseconds(0);
    stats_.speed_docs_per_sec = 0;
    stats_.speed_tokens_per_sec = 0;
}

// Экспорт статистики в файл
bool IndexBuilder::export_stats(const ds::String& filepath) const {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    file << "=== Index Build Statistics ===\n\n";
    
    file << "Documents processed: " << stats_.documents_processed << "\n";
    file << "Documents skipped: " << stats_.documents_skipped << "\n";
    file << "Total tokens: " << stats_.total_tokens << "\n";
    file << "Unique tokens: " << stats_.unique_tokens << "\n";
    file << "Build time: " << stats_.build_time.count() << " ms\n";
    file << "Average speed: " << stats_.speed_docs_per_sec << " docs/sec\n";
    file << "Token processing speed: " << stats_.speed_tokens_per_sec << " tokens/sec\n";
    
    if (index_) {
        const auto& index_stats = index_->get_stats();
        file << "\n=== Index Statistics ===\n\n";
        file << "Total documents in index: " << index_stats.total_documents << "\n";
        file << "Total terms in index: " << index_stats.total_terms << "\n";
        file << "Total postings: " << index_stats.total_postings << "\n";
        file << "Average document length: " << index_stats.avg_document_length << "\n";
        file << "Average term frequency: " << index_stats.avg_term_frequency << "\n";
        file << "Most frequent term: '" << index_stats.most_frequent_term 
             << "' (in " << index_stats.most_frequent_term_count << " documents)\n";
    }
    
    return true;
}

// Оптимизация индекса
void IndexBuilder::optimize_index() {
    if (!index_) {
        return;
    }
    
    auto start_time = high_resolution_clock::now();
    
    std::cout << "Optimizing index...\n";
    
    // Сортируем постинги для каждого термина по doc_id
    auto terms = index_->get_all_terms();
    
    for (const auto& term : terms) {
        auto* postings = const_cast<ds::Vector<Posting>*>(index_->find_postings(term));
        if (!postings) continue;
        
        // Сортировка пузырьком (для простоты)
        for (size_t i = 0; i < postings->size(); ++i) {
            for (size_t j = i + 1; j < postings->size(); ++j) {
                if ((*postings)[j].doc_id < (*postings)[i].doc_id) {
                    std::swap((*postings)[i], (*postings)[j]);
                }
            }
        }
    }
    
    auto end_time = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(end_time - start_time);
    
    std::cout << "Index optimized in " << elapsed.count() << " ms\n";
}

} // namespace search