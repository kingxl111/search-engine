#ifndef STRING_H
#define STRING_H

#include "vector.h"
#include <cstddef>
#include <cstdint>
#include <utility>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace ds {

class String {
private:
    Vector<char> data_;
    
    // Вспомогательные функции для UTF-8
    static bool is_ascii_char(char c) {
        return static_cast<unsigned char>(c) <= 127;
    }
    
    static bool is_continuation_byte(char c) {
        return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
    }
    
    static size_t utf8_char_length(char first_byte) {
        unsigned char c = static_cast<unsigned char>(first_byte);
        if (c < 0x80) return 1;
        if (c < 0xE0) return 2;
        if (c < 0xF0) return 3;
        if (c < 0xF8) return 4;
        return 1; // Некорректный UTF-8, обрабатываем как 1 байт
    }
    
public:
    // Конструкторы
    String() = default;
    
    String(const char* str) {
        if (str) {
            while (*str) {
                data_.push_back(*str);
                ++str;
            }
        }
    }
    
    String(const char* str, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            data_.push_back(str[i]);
        }
    }
    
    String(size_t count, char ch) {
        for (size_t i = 0; i < count; ++i) {
            data_.push_back(ch);
        }
    }
    
    String(std::initializer_list<char> init) : data_(init) {}
    
    // Конструкторы копирования/перемещения
    String(const String& other) : data_(other.data_) {}
    String(String&& other) noexcept : data_(std::move(other.data_)) {}
    
    // Деструктор
    ~String() = default;
    
    // Операторы присваивания
    String& operator=(const String& other) {
        if (this != &other) {
            data_ = other.data_;
        }
        return *this;
    }
    
    String& operator=(String&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
        }
        return *this;
    }
    
    String& operator=(const char* str) {
        data_.clear();
        if (str) {
            while (*str) {
                data_.push_back(*str);
                ++str;
            }
        }
        return *this;
    }
    
    // Доступ к элементам
    char& operator[](size_t index) {
        return data_[index];
    }
    
    const char& operator[](size_t index) const {
        return data_[index];
    }
    
    char& at(size_t index) {
        if (index >= data_.size()) {
            throw std::out_of_range("String index out of range");
        }
        return data_[index];
    }
    
    const char& at(size_t index) const {
        if (index >= data_.size()) {
            throw std::out_of_range("String index out of range");
        }
        return data_[index];
    }
    
    char& front() {
        if (data_.empty()) {
            throw std::out_of_range("String is empty");
        }
        return data_.front();
    }
    
    const char& front() const {
        if (data_.empty()) {
            throw std::out_of_range("String is empty");
        }
        return data_.front();
    }
    
    char& back() {
        if (data_.empty()) {
            throw std::out_of_range("String is empty");
        }
        return data_.back();
    }
    
    const char& back() const {
        if (data_.empty()) {
            throw std::out_of_range("String is empty");
        }
        return data_.back();
    }
    
    const char* c_str() const {
        // Добавляем нулевой терминатор
        if (data_.empty() || data_.back() != '\0') {
            const_cast<String*>(this)->data_.push_back('\0');
        }
        return data_.data();
    }
    
    const char* data() const {
        return data_.data();
    }
    
    // Итераторы
    char* begin() { return data_.begin(); }
    const char* begin() const { return data_.begin(); }
    const char* cbegin() const { return data_.begin(); }
    
    char* end() { return data_.end(); }
    const char* end() const { return data_.end(); }
    const char* cend() const { return data_.end(); }
    
    // Емкость
    bool empty() const { return data_.empty(); }
    size_t size() const { return data_.size(); }
    size_t length() const { return data_.size(); }
    size_t capacity() const { return data_.capacity(); }
    
    void reserve(size_t new_capacity) {
        data_.reserve(new_capacity);
    }
    
    void shrink_to_fit() {
        data_.shrink_to_fit();
    }
    
    // Модификаторы
    void clear() {
        data_.clear();
    }
    
    void push_back(char ch) {
        data_.push_back(ch);
    }
    
    void pop_back() {
        if (!data_.empty()) {
            data_.pop_back();
        }
    }
    
    void append(const String& str) {
        for (char c : str) {
            data_.push_back(c);
        }
    }
    
    void append(const char* str) {
        if (str) {
            while (*str) {
                data_.push_back(*str);
                ++str;
            }
        }
    }
    
    void append(size_t count, char ch) {
        for (size_t i = 0; i < count; ++i) {
            data_.push_back(ch);
        }
    }
    
    String& operator+=(const String& str) {
        append(str);
        return *this;
    }
    
    String& operator+=(const char* str) {
        append(str);
        return *this;
    }
    
    String& operator+=(char ch) {
        push_back(ch);
        return *this;
    }
    
    // Сравнение
    bool operator==(const String& other) const {
        if (data_.size() != other.data_.size()) return false;
        for (size_t i = 0; i < data_.size(); ++i) {
            if (data_[i] != other.data_[i]) return false;
        }
        return true;
    }
    
    bool operator!=(const String& other) const {
        return !(*this == other);
    }
    
    bool operator<(const String& other) const {
        size_t min_size = std::min(data_.size(), other.data_.size());
        for (size_t i = 0; i < min_size; ++i) {
            if (data_[i] != other.data_[i]) {
                return data_[i] < other.data_[i];
            }
        }
        return data_.size() < other.data_.size();
    }
    
    bool operator<=(const String& other) const {
        return !(other < *this);
    }
    
    bool operator>(const String& other) const {
        return other < *this;
    }
    
    bool operator>=(const String& other) const {
        return !(*this < other);
    }
    
    // Поиск
    size_t find(char ch, size_t pos = 0) const {
        for (size_t i = pos; i < data_.size(); ++i) {
            if (data_[i] == ch) {
                return i;
            }
        }
        return npos;
    }
    
    size_t find(const String& str, size_t pos = 0) const {
        if (str.empty() || pos + str.size() > data_.size()) {
            return npos;
        }
        
        for (size_t i = pos; i <= data_.size() - str.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < str.size(); ++j) {
                if (data_[i + j] != str[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return i;
            }
        }
        return npos;
    }
    
    size_t find(const char* str, size_t pos = 0) const {
        return find(String(str), pos);
    }
    
    // Подстрока
    String substr(size_t pos = 0, size_t count = npos) const {
        if (pos > data_.size()) {
            throw std::out_of_range("String::substr position out of range");
        }
        
        size_t actual_count = std::min(count, data_.size() - pos);
        String result;
        result.reserve(actual_count);
        
        for (size_t i = pos; i < pos + actual_count; ++i) {
            result.push_back(data_[i]);
        }
        
        return result;
    }
    
    // Замена
    void replace(size_t pos, size_t count, const String& str) {
        if (pos > data_.size()) {
            throw std::out_of_range("String::replace position out of range");
        }
        
        size_t actual_count = std::min(count, data_.size() - pos);
        
        // Создаем новую строку
        Vector<char> new_data;
        new_data.reserve(data_.size() - actual_count + str.size());
        
        // Копируем часть до позиции
        for (size_t i = 0; i < pos; ++i) {
            new_data.push_back(data_[i]);
        }
        
        // Копируем замену
        for (char c : str) {
            new_data.push_back(c);
        }
        
        // Копируем часть после замены
        for (size_t i = pos + actual_count; i < data_.size(); ++i) {
            new_data.push_back(data_[i]);
        }
        
        data_ = std::move(new_data);
    }
    
    // Удаление
    void erase(size_t pos = 0, size_t count = npos) {
        if (pos > data_.size()) {
            throw std::out_of_range("String::erase position out of range");
        }
        
        size_t actual_count = std::min(count, data_.size() - pos);
        
        // Сдвигаем элементы
        for (size_t i = pos; i < data_.size() - actual_count; ++i) {
            data_[i] = data_[i + actual_count];
        }
        
        // Удаляем лишние элементы
        data_.resize(data_.size() - actual_count);
    }
    
    // Вставка
    void insert(size_t pos, const String& str) {
        if (pos > data_.size()) {
            throw std::out_of_range("String::insert position out of range");
        }
        
        Vector<char> new_data;
        new_data.reserve(data_.size() + str.size());
        
        // Копируем часть до позиции
        for (size_t i = 0; i < pos; ++i) {
            new_data.push_back(data_[i]);
        }
        
        // Вставляем новую строку
        for (char c : str) {
            new_data.push_back(c);
        }
        
        // Копируем часть после позиции
        for (size_t i = pos; i < data_.size(); ++i) {
            new_data.push_back(data_[i]);
        }
        
        data_ = std::move(new_data);
    }
    
    // Преобразование регистра (только для ASCII)
    String to_lower() const {
        String result = *this;
        for (size_t i = 0; i < result.size(); ++i) {
            char c = result[i];
            if (c >= 'A' && c <= 'Z') {
                result[i] = c + ('a' - 'A');
            } else if (c >= 'А' && c <= 'Я') { // Русские заглавные в Windows-1251
                result[i] = c + ('а' - 'А');
            } else if (c == 'Ё') {
                result[i] = 'ё';
            }
        }
        return result;
    }
    
    String to_upper() const {
        String result = *this;
        for (size_t i = 0; i < result.size(); ++i) {
            char c = result[i];
            if (c >= 'a' && c <= 'z') {
                result[i] = c - ('a' - 'A');
            } else if (c >= 'а' && c <= 'я') { // Русские строчные в Windows-1251
                result[i] = c - ('а' - 'А');
            } else if (c == 'ё') {
                result[i] = 'Ё';
            }
        }
        return result;
    }
    
    // Проверка на число
    bool is_digit() const {
        if (empty()) return false;
        for (char c : *this) {
            if (c < '0' || c > '9') return false;
        }
        return true;
    }
    
    bool is_alpha() const {
        if (empty()) return false;
        for (char c : *this) {
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= 'а' && c <= 'я') || (c >= 'А' && c <= 'Я') ||
                  c == 'ё' || c == 'Ё')) {
                return false;
            }
        }
        return true;
    }
    
    bool is_alnum() const {
        if (empty()) return false;
        for (char c : *this) {
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') ||
                  (c >= 'а' && c <= 'я') || (c >= 'А' && c <= 'Я') ||
                  c == 'ё' || c == 'Ё')) {
                return false;
            }
        }
        return true;
    }
    
    // Разделение строки
    Vector<String> split(char delimiter) const {
        Vector<String> result;
        size_t start = 0;
        size_t end = find(delimiter);
        
        while (end != npos) {
            result.push_back(substr(start, end - start));
            start = end + 1;
            end = find(delimiter, start);
        }
        
        // Добавляем последнюю часть
        result.push_back(substr(start));
        
        return result;
    }
    
    Vector<String> split(const String& delimiter) const {
        Vector<String> result;
        size_t start = 0;
        size_t end = find(delimiter);
        
        while (end != npos) {
            result.push_back(substr(start, end - start));
            start = end + delimiter.size();
            end = find(delimiter, start);
        }
        
        result.push_back(substr(start));
        
        return result;
    }
    
    // Обрезка пробелов
    String trim() const {
        size_t start = 0;
        size_t end = data_.size();
        
        // Находим первый не пробельный символ
        while (start < end && std::isspace(static_cast<unsigned char>(data_[start]))) {
            ++start;
        }
        
        // Находим последний не пробельный символ
        while (end > start && std::isspace(static_cast<unsigned char>(data_[end - 1]))) {
            --end;
        }
        
        return substr(start, end - start);
    }
    
    String trim_left() const {
        size_t start = 0;
        while (start < data_.size() && std::isspace(static_cast<unsigned char>(data_[start]))) {
            ++start;
        }
        return substr(start);
    }
    
    String trim_right() const {
        size_t end = data_.size();
        while (end > 0 && std::isspace(static_cast<unsigned char>(data_[end - 1]))) {
            --end;
        }
        return substr(0, end);
    }
    
    // Проверка на начало/конец
    bool starts_with(const String& str) const {
        if (str.size() > data_.size()) return false;
        for (size_t i = 0; i < str.size(); ++i) {
            if (data_[i] != str[i]) return false;
        }
        return true;
    }
    
    bool ends_with(const String& str) const {
        if (str.size() > data_.size()) return false;
        size_t offset = data_.size() - str.size();
        for (size_t i = 0; i < str.size(); ++i) {
            if (data_[offset + i] != str[i]) return false;
        }
        return true;
    }
    
    // Статические константы
    static const size_t npos = static_cast<size_t>(-1);
    
    // Дружественные операторы
    friend std::ostream& operator<<(std::ostream& os, const String& str) {
        for (char c : str) {
            os << c;
        }
        return os;
    }
};

// Бинарные операторы
inline String operator+(const String& lhs, const String& rhs) {
    String result = lhs;
    result += rhs;
    return result;
}

inline String operator+(const String& lhs, const char* rhs) {
    String result = lhs;
    result += rhs;
    return result;
}

inline String operator+(const char* lhs, const String& rhs) {
    String result = lhs;
    result += rhs;
    return result;
}

inline String operator+(const String& lhs, char rhs) {
    String result = lhs;
    result += rhs;
    return result;
}

inline String operator+(char lhs, const String& rhs) {
    String result;
    result += lhs;
    result += rhs;
    return result;
}

// Хеш-функция для String (для использования в HashTable)
template<>
struct std::hash<ds::String> {
    size_t operator()(const ds::String& str) const {
        size_t hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
        }
        return hash;
    }
};

} // namespace ds

#endif // STRING_H