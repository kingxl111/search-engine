#ifndef DS_VECTOR_H
#define DS_VECTOR_H

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <initializer_list>
#include <algorithm>

namespace ds {

template <typename T>
class Vector {
private:
    T* data_;
    size_t size_;
    size_t capacity_;

    // Приватные вспомогательные методы
    void reallocate(size_t new_capacity) {
        if (new_capacity <= capacity_) return;
        
        T* new_data = new T[new_capacity];
        
        // Копируем существующие элементы
        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move(data_[i]);
        }
        
        // Удаляем старый массив
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }

public:
    // Конструкторы
    Vector() : data_(nullptr), size_(0), capacity_(0) {}
    
    explicit Vector(size_t size) : size_(size), capacity_(size) {
        data_ = new T[capacity_];
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = T();
        }
    }
    
    Vector(size_t size, const T& value) : size_(size), capacity_(size) {
        data_ = new T[capacity_];
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = value;
        }
    }
    
    Vector(std::initializer_list<T> init) : 
        size_(init.size()), capacity_(init.size()) {
        data_ = new T[capacity_];
        size_t i = 0;
        for (const auto& item : init) {
            data_[i++] = item;
        }
    }
    
    // Конструктор копирования
    Vector(const Vector& other) : 
        size_(other.size_), capacity_(other.capacity_) {
        data_ = new T[capacity_];
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }
    
    // Конструктор перемещения
    Vector(Vector&& other) noexcept : 
        data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    
    // Деструктор
    ~Vector() {
        delete[] data_;
    }
    
    // Оператор присваивания копированием
    Vector& operator=(const Vector& other) {
        if (this != &other) {
            delete[] data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            data_ = new T[capacity_];
            for (size_t i = 0; i < size_; ++i) {
                data_[i] = other.data_[i];
            }
        }
        return *this;
    }
    
    // Оператор присваивания перемещением
    Vector& operator=(Vector&& other) noexcept {
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
    
    // Доступ к элементам
    T& operator[](size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Vector index out of range");
        }
        return data_[index];
    }
    
    const T& operator[](size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Vector index out of range");
        }
        return data_[index];
    }
    
    T& at(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Vector index out of range");
        }
        return data_[index];
    }
    
    const T& at(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Vector index out of range");
        }
        return data_[index];
    }
    
    T& front() {
        if (size_ == 0) {
            throw std::out_of_range("Vector is empty");
        }
        return data_[0];
    }
    
    const T& front() const {
        if (size_ == 0) {
            throw std::out_of_range("Vector is empty");
        }
        return data_[0];
    }
    
    T& back() {
        if (size_ == 0) {
            throw std::out_of_range("Vector is empty");
        }
        return data_[size_ - 1];
    }
    
    const T& back() const {
        if (size_ == 0) {
            throw std::out_of_range("Vector is empty");
        }
        return data_[size_ - 1];
    }
    
    T* data() { return data_; }
    const T* data() const { return data_; }
    
    // Итераторы
    T* begin() { return data_; }
    const T* begin() const { return data_; }
    const T* cbegin() const { return data_; }
    
    T* end() { return data_ + size_; }
    const T* end() const { return data_ + size_; }
    const T* cend() const { return data_ + size_; }
    
    // Емкость
    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    
    void reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            reallocate(new_capacity);
        }
    }
    
    void shrink_to_fit() {
        if (capacity_ > size_) {
            reallocate(size_);
        }
    }
    
    // Модификаторы
    void clear() {
        size_ = 0;
    }
    
    void push_back(const T& value) {
        if (size_ == capacity_) {
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        data_[size_++] = value;
    }
    
    void push_back(T&& value) {
        if (size_ == capacity_) {
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        data_[size_++] = std::move(value);
    }
    
    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        data_[size_++] = T(std::forward<Args>(args)...);
    }
    
    void pop_back() {
        if (size_ > 0) {
            --size_;
        }
    }
    
    void resize(size_t new_size) {
        if (new_size > capacity_) {
            reallocate(new_size);
        }
        size_ = new_size;
    }
    
    void resize(size_t new_size, const T& value) {
        size_t old_size = size_;
        resize(new_size);
        if (new_size > old_size) {
            for (size_t i = old_size; i < new_size; ++i) {
                data_[i] = value;
            }
        }
    }
    
    void swap(Vector& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }
    
    // Удаление и вставка
    void erase(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Vector index out of range");
        }
        
        for (size_t i = index; i < size_ - 1; ++i) {
            data_[i] = std::move(data_[i + 1]);
        }
        --size_;
    }
    
    void insert(size_t index, const T& value) {
        if (index > size_) {
            throw std::out_of_range("Vector index out of range");
        }
        
        if (size_ == capacity_) {
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        
        // Сдвигаем элементы вправо
        for (size_t i = size_; i > index; --i) {
            data_[i] = std::move(data_[i - 1]);
        }
        
        data_[index] = value;
        ++size_;
    }
};

} // namespace ds

#endif // DS_VECTOR_H