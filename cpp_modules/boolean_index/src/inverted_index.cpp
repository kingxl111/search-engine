#include "inverted_index.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace search {

// Конструкторы
InvertedIndex::InvertedIndex() {
    tokenizer_ = std::make_unique<Tokenizer>();
    update_stats();
}

InvertedIndex::InvertedIndex(std::unique_ptr<Tokenizer> tokenizer) 
    : tokenizer_(std::move(tokenizer)) {
    update_stats();
}

// Добавление документа
uint32_t InvertedIndex::add_document(const Document& document) {
    // Проверяем, не добавлен ли уже документ с таким URL
    auto* existing_id = url_to_doc_id_.find(document.url);
    if (existing_id != nullptr) {
        return *existing_id;
    }
    
    // Создаем копию документа с правильным ID
    Document doc = document;
    doc.id = static_cast<uint32_t>(documents_.size());
    
    // Сохраняем документ
    documents_.push_back(doc);
    url_to_doc_id_.insert(doc.url, doc.id);
    
    return doc.id;
}

// Пакетное добавление документов
void InvertedIndex::add_documents(const ds::Vector<Document>& documents) {
    for (const auto& doc : documents) {
        add_document(doc);
    }
}

// Индексация документа
uint32_t InvertedIndex::index_document(const Document& document) {
    // Добавляем документ
    uint32_t doc_id = add_document(document);
    
    // Токенизируем контент документа
    auto token_positions = tokenizer_->tokenize_with_positions(document.content);
    
    // Обрабатываем каждый токен
    ds::HashTable<ds::String, ds::Vector<uint32_t>> term_positions;
    
    for (const auto& token_info : token_positions) {
        const ds::String& term = token_info.token;
        
        // Добавляем позицию в список позиций для термина
        auto* positions = term_positions.find(term);
        if (positions) {
            positions->push_back(token_info.position);
        } else {
            ds::Vector<uint32_t> new_positions;
            new_positions.push_back(token_info.position);
            term_positions.insert(term, new_positions);
        }
    }
    
    // Добавляем термины в инвертированный индекс
    auto term_keys = term_positions.keys();
    for (const auto& term : term_keys) {
        const ds::Vector<uint32_t>* positions_ptr = term_positions.find(term);
        if (!positions_ptr) continue;
        const ds::Vector<uint32_t>& positions = *positions_ptr;
        
        // Ищем или создаем список постингов для термина
        ds::Vector<Posting>* postings = index_.find(term);
        
        if (postings) {
            // Добавляем новый постинг
            Posting posting(doc_id);
            for (uint32_t pos : positions) {
                posting.add_position(pos);
            }
            postings->push_back(posting);
        } else {
            // Создаем новый список постингов
            ds::Vector<Posting> new_postings;
            Posting posting(doc_id);
            for (uint32_t pos : positions) {
                posting.add_position(pos);
            }
            new_postings.push_back(posting);
            index_.insert(term, new_postings);
        }
    }
    
    // Обновляем длину документа (количество уникальных терминов)
    documents_[doc_id].length = term_positions.size();
    
    // Обновляем статистику
    update_stats();
    
    return doc_id;
}

// Поиск документов по термину
const ds::Vector<Posting>* InvertedIndex::find_postings(const ds::String& term) const {
    return index_.find(term);
}

// Получение документа по ID
const Document& InvertedIndex::get_document(uint32_t doc_id) const {
    if (doc_id >= documents_.size()) {
        throw std::out_of_range("Document ID out of range");
    }
    return documents_[doc_id];
}

// Получение всех терминов
ds::Vector<ds::String> InvertedIndex::get_all_terms() const {
    return index_.keys();
}

// Получение частоты термина в коллекции
size_t InvertedIndex::get_term_frequency(const ds::String& term) const {
    const auto* postings = find_postings(term);
    if (!postings) {
        return 0;
    }
    
    size_t total_freq = 0;
    for (const auto& posting : *postings) {
        total_freq += posting.frequency;
    }
    
    return total_freq;
}

// Получение документа по URL
const Document* InvertedIndex::get_document_by_url(const ds::String& url) const {
    auto* doc_id_ptr = url_to_doc_id_.find(url);
    if (!doc_id_ptr) {
        return nullptr;
    }
    return &documents_[*doc_id_ptr];
}

// Получение документа по позиции в списке
const Document& InvertedIndex::get_document_by_position(size_t position) const {
    if (position >= documents_.size()) {
        throw std::out_of_range("Document position out of range");
    }
    return documents_[position];
}

// Обновление статистики
void InvertedIndex::update_stats() {
    stats_.total_documents = documents_.size();
    stats_.total_terms = index_.size();
    
    // Вычисляем общее количество постингов
    stats_.total_postings = 0;
    size_t total_doc_length = 0;
    
    auto terms = index_.keys();
    for (const auto& term : terms) {
        const auto* postings = index_.find(term);
        if (postings) {
            stats_.total_postings += postings->size();
        }
    }
    
    // Вычисляем среднюю длину документа
    for (const auto& doc : documents_) {
        total_doc_length += doc.length;
    }
    
    stats_.avg_document_length = documents_.empty() ? 0.0 : 
        static_cast<double>(total_doc_length) / documents_.size();
    
    // Вычисляем среднюю частоту термина
    stats_.avg_term_frequency = stats_.total_terms == 0 ? 0.0 :
        static_cast<double>(stats_.total_postings) / stats_.total_terms;
    
    // Находим самый частый термин
    size_t max_freq = 0;
    ds::String most_frequent;
    
    for (const auto& term : terms) {
        const auto* postings = index_.find(term);
        if (postings) {
            size_t freq = postings->size();
            if (freq > max_freq) {
                max_freq = freq;
                most_frequent = term;
            }
        }
    }
    
    stats_.most_frequent_term_count = max_freq;
    stats_.most_frequent_term = most_frequent;
}

// Очистка индекса
void InvertedIndex::clear() {
    index_.clear();
    documents_.clear();
    url_to_doc_id_.clear();
    update_stats();
}

// Сохранение индекса в файл
bool InvertedIndex::save_to_file(const ds::String& filepath) const {
    std::ofstream file(filepath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Заголовок файла
    struct FileHeader {
        char signature[8];        // "BOOLIDX\0"
        uint32_t version;         // 1
        uint32_t doc_count;       // количество документов
        uint32_t term_count;      // количество терминов
        uint32_t posting_count;   // общее количество постингов
        uint32_t reserved[4];     // зарезервировано
    };
    
    FileHeader header;
    std::strcpy(header.signature, "BOOLIDX");
    header.signature[7] = '\0';
    header.version = 1;
    header.doc_count = static_cast<uint32_t>(documents_.size());
    header.term_count = static_cast<uint32_t>(index_.size());
    header.posting_count = static_cast<uint32_t>(stats_.total_postings);
    
    // Записываем заголовок
    file.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));
    
    // Записываем документы
    for (const auto& doc : documents_) {
        // Записываем ID документа
        file.write(reinterpret_cast<const char*>(&doc.id), sizeof(uint32_t));
        
        // Записываем длину заголовка и сам заголовок
        uint32_t title_len = static_cast<uint32_t>(doc.title.size());
        file.write(reinterpret_cast<const char*>(&title_len), sizeof(uint32_t));
        file.write(doc.title.c_str(), title_len);
        
        // Записываем URL
        uint32_t url_len = static_cast<uint32_t>(doc.url.size());
        file.write(reinterpret_cast<const char*>(&url_len), sizeof(uint32_t));
        file.write(doc.url.c_str(), url_len);
        
        // Записываем длину контента (опционально, для экономии места не сохраняем сам контент)
        uint32_t content_len = static_cast<uint32_t>(doc.content.size());
        file.write(reinterpret_cast<const char*>(&content_len), sizeof(uint32_t));
        
        // Записываем длину документа в терминах
        file.write(reinterpret_cast<const char*>(&doc.length), sizeof(uint32_t));
    }
    
    // Записываем инвертированный индекс
    auto terms = index_.keys();
    
    // Сначала записываем таблицу смещений
    struct TermOffset {
        uint32_t term_length;
        uint32_t posting_count;
        uint64_t file_offset;
    };
    
    ds::Vector<TermOffset> term_offsets;
    uint64_t current_offset = sizeof(FileHeader) + 
                              (documents_.size() * (sizeof(uint32_t) * 4)) + // размер блока документов
                              (terms.size() * sizeof(TermOffset)); // место для таблицы смещений
    
    // Записываем термины и их постинги
    for (const auto& term : terms) {
        const auto* postings = index_.find(term);
        if (!postings) continue;
        
        TermOffset offset;
        offset.term_length = static_cast<uint32_t>(term.size());
        offset.posting_count = static_cast<uint32_t>(postings->size());
        offset.file_offset = current_offset;
        
        term_offsets.push_back(offset);
        
        // Вычисляем смещение для следующего термина
        current_offset += term.size() + // сам термин
                         sizeof(uint32_t) + // количество постингов
                         (postings->size() * (sizeof(uint32_t) * 2)); // постинги (doc_id + freq)
    }
    
    // Записываем таблицу смещений
    for (const auto& offset : term_offsets) {
        file.write(reinterpret_cast<const char*>(&offset), sizeof(TermOffset));
    }
    
    // Записываем данные терминов
    for (size_t i = 0; i < terms.size(); ++i) {
        const ds::String& term = terms[i];
        const auto* postings = index_.find(term);
        
        // Записываем термин
        file.write(term.c_str(), term.size());
        
        // Записываем количество постингов
        uint32_t posting_count = static_cast<uint32_t>(postings->size());
        file.write(reinterpret_cast<const char*>(&posting_count), sizeof(uint32_t));
        
        // Записываем постинги
        for (const auto& posting : *postings) {
            file.write(reinterpret_cast<const char*>(&posting.doc_id), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&posting.frequency), sizeof(uint32_t));
        }
    }
    
    return true;
}

// Загрузка индекса из файла
bool InvertedIndex::load_from_file(const ds::String& filepath) {
    std::ifstream file(filepath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Читаем заголовок
    struct FileHeader {
        char signature[8];
        uint32_t version;
        uint32_t doc_count;
        uint32_t term_count;
        uint32_t posting_count;
        uint32_t reserved[4];
    };
    
    FileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
    
    // Проверяем сигнатуру
    if (std::strcmp(header.signature, "BOOLIDX") != 0) {
        return false;
    }
    
    // Очищаем текущий индекс
    clear();
    
    // Читаем документы
    documents_.reserve(header.doc_count);
    
    for (uint32_t i = 0; i < header.doc_count; ++i) {
        Document doc;
        
        // Читаем ID документа
        file.read(reinterpret_cast<char*>(&doc.id), sizeof(uint32_t));
        
        // Читаем заголовок
        uint32_t title_len;
        file.read(reinterpret_cast<char*>(&title_len), sizeof(uint32_t));
        
        ds::Vector<char> title_buf(title_len);
        file.read(title_buf.data(), title_len);
        doc.title = ds::String(title_buf.data(), title_len);
        
        // Читаем URL
        uint32_t url_len;
        file.read(reinterpret_cast<char*>(&url_len), sizeof(uint32_t));
        
        ds::Vector<char> url_buf(url_len);
        file.read(url_buf.data(), url_len);
        doc.url = ds::String(url_buf.data(), url_len);
        
        // Читаем длину контента (пропускаем)
        uint32_t content_len;
        file.read(reinterpret_cast<char*>(&content_len), sizeof(uint32_t));
        
        // Читаем длину документа в терминах
        file.read(reinterpret_cast<char*>(&doc.length), sizeof(uint32_t));
        
        // Сохраняем документ
        documents_.push_back(doc);
        url_to_doc_id_.insert(doc.url, doc.id);
    }
    
    // Читаем таблицу смещений терминов
    struct TermOffset {
        uint32_t term_length;
        uint32_t posting_count;
        uint64_t file_offset;
    };
    
    ds::Vector<TermOffset> term_offsets(header.term_count);
    for (uint32_t i = 0; i < header.term_count; ++i) {
        file.read(reinterpret_cast<char*>(&term_offsets[i]), sizeof(TermOffset));
    }
    
    // Читаем термины и постинги
    for (const auto& offset : term_offsets) {
        // Перемещаемся к позиции термина
        file.seekg(static_cast<std::streamoff>(offset.file_offset));
        
        // Читаем термин
        ds::Vector<char> term_buf(offset.term_length);
        file.read(term_buf.data(), offset.term_length);
        ds::String term(term_buf.data(), offset.term_length);
        
        // Читаем количество постингов
        uint32_t posting_count;
        file.read(reinterpret_cast<char*>(&posting_count), sizeof(uint32_t));
        
        // Читаем постинги
        ds::Vector<Posting> postings;
        postings.reserve(posting_count);
        
        for (uint32_t i = 0; i < posting_count; ++i) {
            Posting posting;
            file.read(reinterpret_cast<char*>(&posting.doc_id), sizeof(uint32_t));
            file.read(reinterpret_cast<char*>(&posting.frequency), sizeof(uint32_t));
            
            // Пропускаем позиции (они не сохраняются в базовой версии)
            posting.positions.reserve(posting.frequency);
            for (uint32_t j = 0; j < posting.frequency; ++j) {
                posting.positions.push_back(0);
            }
            
            postings.push_back(posting);
        }
        
        // Добавляем в индекс
        index_.insert(term, postings);
    }
    
    // Обновляем статистику
    update_stats();
    
    return true;
}

// Экспорт в текстовый формат
bool InvertedIndex::export_to_text(const ds::String& filepath) const {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    // Записываем статистику
    file << "=== Boolean Index Statistics ===\n";
    file << "Total documents: " << stats_.total_documents << "\n";
    file << "Total terms: " << stats_.total_terms << "\n";
    file << "Total postings: " << stats_.total_postings << "\n";
    file << "Avg document length: " << stats_.avg_document_length << "\n";
    file << "Avg term frequency: " << stats_.avg_term_frequency << "\n";
    file << "Most frequent term: '" << stats_.most_frequent_term 
         << "' (appears in " << stats_.most_frequent_term_count << " documents)\n\n";
    
    // Записываем документы
    file << "=== Documents ===\n";
    for (const auto& doc : documents_) {
        file << "Document #" << doc.id << ":\n";
        file << "  Title: " << doc.title << "\n";
        file << "  URL: " << doc.url << "\n";
        file << "  Length (unique terms): " << doc.length << "\n\n";
    }
    
    // Записываем инвертированный индекс
    file << "=== Inverted Index ===\n";
    auto terms = get_all_terms();
    
    // Сортируем термины для удобства чтения
    // Простая пузырьковая сортировка
    for (size_t i = 0; i < terms.size(); ++i) {
        for (size_t j = i + 1; j < terms.size(); ++j) {
            if (terms[j] < terms[i]) {
                std::swap(terms[i], terms[j]);
            }
        }
    }
    
    for (const auto& term : terms) {
        const auto* postings = find_postings(term);
        if (!postings) continue;
        
        file << "Term: '" << term << "' (appears in " << postings->size() << " documents)\n";
        
        for (const auto& posting : *postings) {
            const auto& doc = get_document(posting.doc_id);
            file << "  Doc #" << posting.doc_id << " (" << doc.title << "): "
                 << "frequency=" << posting.frequency << "\n";
        }
        file << "\n";
    }
    
    return true;
}

// Проверка целостности индекса
bool InvertedIndex::validate() const {
    // Проверяем соответствие документов и их ID
    for (size_t i = 0; i < documents_.size(); ++i) {
        if (documents_[i].id != i) {
            return false;
        }
        
        // Проверяем, что URL существует в хэш-таблице
        auto* doc_id = url_to_doc_id_.find(documents_[i].url);
        if (!doc_id || *doc_id != documents_[i].id) {
            return false;
        }
    }
    
    // Проверяем постинги
    auto all_terms = index_.keys();
    for (const auto& term : all_terms) {
        const auto* postings = index_.find(term);
        if (postings) {
            for (const auto& posting : *postings) {
                if (posting.doc_id >= documents_.size()) {
                    return false;
                }
                
                if (posting.frequency != posting.positions.size()) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

} // namespace search