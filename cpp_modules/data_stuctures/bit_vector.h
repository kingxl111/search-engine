#ifndef BIT_VECTOR_H
#define BIT_VECTOR_H

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace ds {

class BitVector {
private:
    uint64_t* data_;
    size_t size_;        // Количество битов
    size_t capacity_;    // Количество 64-битных слов
    
    static const size_t BITS_PER_WORD = 64;
    
    // Вычисляет количество слов для заданного количества битов
    static size_t words_for_bits(size_t bits) {
        return (bits + BITS_PER_WORD - 1) / BITS_PER_WORD;
    }
    
    // Проверяет валидность индекса
    void check_index(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("BitVector index out of range");
        }
    }
    
    // Реаллокация
    void reallocate(size_t new_capacity) {
        uint64_t* new_data = new uint64_t[new_capacity](); // Инициализируем нулями
        
        // Копируем существующие данные
        size_t words_to_copy = std::min(capacity_, new_capacity);
        for (size_t i = 0; i < words_to_copy; ++i) {
            new_data[i] = data_[i];
        }
        
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }

public:
    // Конструкторы
    BitVector() : data_(nullptr), size_(0), capacity_(0) {}
    
    explicit BitVector(size_t size, bool value = false) : 
        size_(size), capacity_(words_for_bits(size)) {
        data_ = new uint64_t[capacity_]();
        
        if (value) {
            // Устанавливаем все биты в 1
            uint64_t pattern = ~static_cast<uint64_t>(0);
            for (size_t i = 0; i < capacity_; ++i) {
                data_[i] = pattern;
            }
            
            // Обнуляем лишние биты в последнем слове
            size_t extra_bits = size_ % BITS_PER_WORD;
            if (extra_bits > 0) {
                data_[capacity_ - 1] &= (static_cast<uint64_t>(1) << extra_bits) - 1;
            }
        }
    }
    
    BitVector(const BitVector& other) : 
        size_(other.size_), capacity_(other.capacity_) {
        data_ = new uint64_t[capacity_];
        for (size_t i = 0; i < capacity_; ++i) {
            data_[i] = other.data_[i];
        }
    }
    
    BitVector(BitVector&& other) noexcept : 
        data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    
    // Деструктор
    ~BitVector() {
        delete[] data_;
    }
    
    // Операторы присваивания
    BitVector& operator=(const BitVector& other) {
        if (this != &other) {
            delete[] data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            data_ = new uint64_t[capacity_];
            for (size_t i = 0; i < capacity_; ++i) {
                data_[i] = other.data_[i];
            }
        }
        return *this;
    }
    
    BitVector& operator=(BitVector&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }
    
    // Доступ к битам
    bool operator[](size_t index) const {
        check_index(index);
        size_t word_index = index / BITS_PER_WORD;
        size_t bit_index = index % BITS_PER_WORD;
        return (data_[word_index] >> bit_index) & 1;
    }
    
    class BitReference {
    private:
        uint64_t* word_ptr_;
        uint64_t mask_;
        
    public:
        BitReference(uint64_t* word_ptr, size_t bit_index) : 
            word_ptr_(word_ptr), mask_(static_cast<uint64_t>(1) << bit_index) {}
        
        operator bool() const {
            return (*word_ptr_ & mask_) != 0;
        }
        
        BitReference& operator=(bool value) {
            if (value) {
                *word_ptr_ |= mask_;
            } else {
                *word_ptr_ &= ~mask_;
            }
            return *this;
        }
        
        BitReference& operator=(const BitReference& other) {
            return *this = static_cast<bool>(other);
        }
        
        bool operator~() const {
            return !static_cast<bool>(*this);
        }
        
        BitReference& flip() {
            *word_ptr_ ^= mask_;
            return *this;
        }
    };
    
    BitReference operator[](size_t index) {
        check_index(index);
        size_t word_index = index / BITS_PER_WORD;
        size_t bit_index = index % BITS_PER_WORD;
        return BitReference(&data_[word_index], bit_index);
    }
    
    bool at(size_t index) const {
        return operator[](index);
    }
    
    BitReference at(size_t index) {
        return operator[](index);
    }
    
    // Размер
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    // Изменение размера
    void resize(size_t new_size, bool value = false) {
        if (new_size <= size_) {
            // Уменьшение размера
            size_ = new_size;
            
            // Обнуляем лишние биты в последнем слове
            size_t extra_bits = new_size % BITS_PER_WORD;
            if (extra_bits > 0) {
                data_[capacity_ - 1] &= (static_cast<uint64_t>(1) << extra_bits) - 1;
            }
        } else {
            // Увеличение размера
            size_t new_capacity = words_for_bits(new_size);
            if (new_capacity > capacity_) {
                reallocate(new_capacity);
            }
            
            // Устанавливаем новые биты
            if (value) {
                // Устанавливаем старые биты за size_ в 1
                for (size_t i = size_; i < new_size; ++i) {
                    set(i);
                }
            } else {
                // Устанавливаем старые биты за size_ в 0
                // (уже 0 из-за reallocate)
            }
            
            size_ = new_size;
        }
    }
    
    // Операции с битами
    void set(size_t index, bool value = true) {
        check_index(index);
        size_t word_index = index / BITS_PER_WORD;
        size_t bit_index = index % BITS_PER_WORD;
        
        if (value) {
            data_[word_index] |= (static_cast<uint64_t>(1) << bit_index);
        } else {
            data_[word_index] &= ~(static_cast<uint64_t>(1) << bit_index);
        }
    }
    
    void reset(size_t index) {
        set(index, false);
    }
    
    void flip(size_t index) {
        check_index(index);
        size_t word_index = index / BITS_PER_WORD;
        size_t bit_index = index % BITS_PER_WORD;
        data_[word_index] ^= (static_cast<uint64_t>(1) << bit_index);
    }
    
    void set_all(bool value = true) {
        uint64_t pattern = value ? ~static_cast<uint64_t>(0) : 0;
        for (size_t i = 0; i < capacity_; ++i) {
            data_[i] = pattern;
        }
        
        // Корректируем последнее слово
        size_t extra_bits = size_ % BITS_PER_WORD;
        if (extra_bits > 0) {
            if (value) {
                data_[capacity_ - 1] &= (static_cast<uint64_t>(1) << extra_bits) - 1;
            } else {
                data_[capacity_ - 1] = 0;
            }
        }
    }
    
    void reset_all() {
        set_all(false);
    }
    
    void flip_all() {
        for (size_t i = 0; i < capacity_; ++i) {
            data_[i] = ~data_[i];
        }
        
        // Корректируем последнее слово
        size_t extra_bits = size_ % BITS_PER_WORD;
        if (extra_bits > 0) {
            data_[capacity_ - 1] &= (static_cast<uint64_t>(1) << extra_bits) - 1;
        }
    }
    
    // Битовая арифметика
    BitVector& operator&=(const BitVector& other) {
        if (size_ != other.size_) {
            throw std::invalid_argument("BitVector sizes must match for &=");
        }
        
        size_t min_capacity = std::min(capacity_, other.capacity_);
        for (size_t i = 0; i < min_capacity; ++i) {
            data_[i] &= other.data_[i];
        }
        
        // Обнуляем остальные слова
        for (size_t i = min_capacity; i < capacity_; ++i) {
            data_[i] = 0;
        }
        
        return *this;
    }
    
    BitVector& operator|=(const BitVector& other) {
        if (size_ != other.size_) {
            throw std::invalid_argument("BitVector sizes must match for |=");
        }
        
        size_t min_capacity = std::min(capacity_, other.capacity_);
        for (size_t i = 0; i < min_capacity; ++i) {
            data_[i] |= other.data_[i];
        }
        
        return *this;
    }
    
    BitVector& operator^=(const BitVector& other) {
        if (size_ != other.size_) {
            throw std::invalid_argument("BitVector sizes must match for ^=");
        }
        
        size_t min_capacity = std::min(capacity_, other.capacity_);
        for (size_t i = 0; i < min_capacity; ++i) {
            data_[i] ^= other.data_[i];
        }
        
        return *this;
    }
    
    BitVector operator~() const {
        BitVector result(*this);
        result.flip_all();
        return result;
    }
    
    // Подсчет битов
    size_t count() const {
        size_t result = 0;
        for (size_t i = 0; i < capacity_; ++i) {
            result += popcount(data_[i]);
        }
        return result;
    }
    
    bool any() const {
        for (size_t i = 0; i < capacity_; ++i) {
            if (data_[i] != 0) return true;
        }
        return false;
    }
    
    bool none() const {
        return !any();
    }
    
    bool all() const {
        if (size_ == 0) return true;
        
        // Проверяем полные слова
        size_t full_words = size_ / BITS_PER_WORD;
        for (size_t i = 0; i < full_words; ++i) {
            if (data_[i] != ~static_cast<uint64_t>(0)) return false;
        }
        
        // Проверяем последнее слово
        size_t extra_bits = size_ % BITS_PER_WORD;
        if (extra_bits > 0) {
            uint64_t mask = (static_cast<uint64_t>(1) << extra_bits) - 1;
            return (data_[capacity_ - 1] & mask) == mask;
        }
        
        return true;
    }
    
    // Строковое представление
    std::string to_string() const {
        std::string result;
        result.reserve(size_);
        for (size_t i = 0; i < size_; ++i) {
            result.push_back((*this)[i] ? '1' : '0');
        }
        return result;
    }
    
    // Поиск первого установленного бита
    size_t find_first() const {
        for (size_t i = 0; i < capacity_; ++i) {
            if (data_[i] != 0) {
                uint64_t word = data_[i];
                return i * BITS_PER_WORD + ctz(word);
            }
        }
        return size_; // не найдено
    }
    
    // Поиск следующего установленного бита после pos
    size_t find_next(size_t pos) const {
        if (pos >= size_) return size_;
        
        size_t word_index = pos / BITS_PER_WORD;
        size_t bit_index = pos % BITS_PER_WORD;
        
        // Проверяем текущее слово
        uint64_t word = data_[word_index] & (~static_cast<uint64_t>(0) << bit_index);
        if (word != 0) {
            return word_index * BITS_PER_WORD + ctz(word);
        }
        
        // Ищем в следующих словах
        for (size_t i = word_index + 1; i < capacity_; ++i) {
            if (data_[i] != 0) {
                return i * BITS_PER_WORD + ctz(data_[i]);
            }
        }
        
        return size_; // не найдено
    }

private:
    // Подсчет установленных битов в 64-битном слове
    static size_t popcount(uint64_t x) {
        // Используем оптимизированный алгоритм
        x = (x & 0x5555555555555555) + ((x >> 1) & 0x5555555555555555);
        x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
        x = (x & 0x0F0F0F0F0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F0F0F0F0F);
        x = (x & 0x00FF00FF00FF00FF) + ((x >> 8) & 0x00FF00FF00FF00FF);
        x = (x & 0x0000FFFF0000FFFF) + ((x >> 16) & 0x0000FFFF0000FFFF);
        x = (x & 0x00000000FFFFFFFF) + ((x >> 32) & 0x00000000FFFFFFFF);
        return static_cast<size_t>(x);
    }
    
    // Подсчет нулей справа (count trailing zeros)
    static size_t ctz(uint64_t x) {
        if (x == 0) return 64;
        
        size_t count = 0;
        if ((x & 0xFFFFFFFF) == 0) { x >>= 32; count += 32; }
        if ((x & 0xFFFF) == 0) { x >>= 16; count += 16; }
        if ((x & 0xFF) == 0) { x >>= 8; count += 8; }
        if ((x & 0xF) == 0) { x >>= 4; count += 4; }
        if ((x & 0x3) == 0) { x >>= 2; count += 2; }
        if ((x & 0x1) == 0) { count += 1; }
        return count;
    }
};

// Бинарные операторы
inline BitVector operator&(const BitVector& lhs, const BitVector& rhs) {
    BitVector result(lhs);
    result &= rhs;
    return result;
}

inline BitVector operator|(const BitVector& lhs, const BitVector& rhs) {
    BitVector result(lhs);
    result |= rhs;
    return result;
}

inline BitVector operator^(const BitVector& lhs, const BitVector& rhs) {
    BitVector result(lhs);
    result ^= rhs;
    return result;
}

} // namespace ds

#endif // BIT_VECTOR_H