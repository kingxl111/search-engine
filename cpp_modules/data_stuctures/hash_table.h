#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "vector.h"
#include <functional>
#include <utility>
#include <string>
#include <stdexcept>

namespace ds {

// Константы для хеш-таблицы
const size_t DEFAULT_CAPACITY = 16;
const double DEFAULT_LOAD_FACTOR = 0.75;

// Структура для хранения элемента хеш-таблицы
template<typename K, typename V>
struct HashNode {
    K key;
    V value;
    bool occupied;
    bool deleted;
    
    HashNode() : occupied(false), deleted(false) {}
    HashNode(const K& k, const V& v) : key(k), value(v), occupied(true), deleted(false) {}
    HashNode(K&& k, V&& v) : key(std::move(k)), value(std::move(v)), occupied(true), deleted(false) {}
};

template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEqual = std::equal_to<K>>
class HashTable {
private:
    using Node = HashNode<K, V>;
    
    Vector<Node> table_;
    size_t size_;
    size_t capacity_;
    double load_factor_;
    Hash hash_function_;
    KeyEqual key_equal_;
    
    // Простое число для увеличения размера
    static const size_t PRIMES[];
    static const size_t PRIMES_COUNT;
    
    // Хеш-функция для индекса
    size_t hash_index(const K& key) const {
        return hash_function_(key) % capacity_;
    }
    
    // Поиск индекса для ключа
    size_t find_index(const K& key) const {
        size_t index = hash_index(key);
        size_t start_index = index;
        size_t i = 1;
        
        while (true) {
            const Node& node = table_[index];
            
            if (!node.occupied && !node.deleted) {
                // Пустая ячейка, ключ не найден
                return capacity_; // возвращаем недопустимый индекс
            }
            
            if (node.occupied && !node.deleted && key_equal_(node.key, key)) {
                // Ключ найден
                return index;
            }
            
            // Квадратичное пробирование
            index = (start_index + i * i) % capacity_;
            ++i;
            
            if (index == start_index) {
                // Прошли весь массив
                return capacity_;
            }
        }
    }
    
    // Поиск индекса для вставки
    size_t find_insert_index(const K& key) {
        size_t index = hash_index(key);
        size_t start_index = index;
        size_t i = 1;
        size_t first_deleted = capacity_;
        
        while (true) {
            Node& node = table_[index];
            
            if (!node.occupied) {
                // Пустая ячейка
                if (first_deleted != capacity_) {
                    return first_deleted;
                }
                return index;
            }
            
            if (node.deleted) {
                // Удаленная ячейка
                if (first_deleted == capacity_) {
                    first_deleted = index;
                }
            } else if (key_equal_(node.key, key)) {
                // Ключ уже существует
                return index;
            }
            
            // Квадратичное пробирование
            index = (start_index + i * i) % capacity_;
            ++i;
            
            if (index == start_index) {
                // Прошли весь массив
                return first_deleted != capacity_ ? first_deleted : capacity_;
            }
        }
    }
    
    // Перехеширование
    void rehash() {
        Vector<Node> old_table = std::move(table_);
        size_t old_capacity = capacity_;
        
        // Увеличиваем емкость
        size_t new_capacity = old_capacity * 2;
        for (size_t i = 0; i < PRIMES_COUNT; ++i) {
            if (PRIMES[i] > new_capacity) {
                new_capacity = PRIMES[i];
                break;
            }
        }
        
        capacity_ = new_capacity;
        table_ = Vector<Node>(capacity_);
        size_ = 0;
        
        // Перемещаем элементы
        for (size_t i = 0; i < old_capacity; ++i) {
            if (old_table[i].occupied && !old_table[i].deleted) {
                insert(std::move(old_table[i].key), std::move(old_table[i].value));
            }
        }
    }

public:
    // Конструкторы
    HashTable() : 
        table_(DEFAULT_CAPACITY), 
        size_(0), 
        capacity_(DEFAULT_CAPACITY), 
        load_factor_(DEFAULT_LOAD_FACTOR) {}
    
    HashTable(size_t capacity) : 
        capacity_(capacity), 
        size_(0), 
        load_factor_(DEFAULT_LOAD_FACTOR) {
        // Находим ближайшее простое число
        for (size_t i = 0; i < PRIMES_COUNT; ++i) {
            if (PRIMES[i] >= capacity) {
                capacity_ = PRIMES[i];
                break;
            }
        }
        table_ = Vector<Node>(capacity_);
    }
    
    HashTable(const HashTable& other) : 
        table_(other.table_), 
        size_(other.size_), 
        capacity_(other.capacity_), 
        load_factor_(other.load_factor_),
        hash_function_(other.hash_function_),
        key_equal_(other.key_equal_) {}
    
    HashTable(HashTable&& other) noexcept : 
        table_(std::move(other.table_)), 
        size_(other.size_), 
        capacity_(other.capacity_), 
        load_factor_(other.load_factor_),
        hash_function_(std::move(other.hash_function_)),
        key_equal_(std::move(other.key_equal_)) {
        other.size_ = 0;
        other.capacity_ = 0;
    }
    
    // Деструктор
    ~HashTable() = default;
    
    // Операторы присваивания
    HashTable& operator=(const HashTable& other) {
        if (this != &other) {
            table_ = other.table_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            load_factor_ = other.load_factor_;
            hash_function_ = other.hash_function_;
            key_equal_ = other.key_equal_;
        }
        return *this;
    }
    
    HashTable& operator=(HashTable&& other) noexcept {
        if (this != &other) {
            table_ = std::move(other.table_);
            size_ = other.size_;
            capacity_ = other.capacity_;
            load_factor_ = other.load_factor_;
            hash_function_ = std::move(other.hash_function_);
            key_equal_ = std::move(other.key_equal_);
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }
    
    // Вставка
    bool insert(const K& key, const V& value) {
        // Проверяем необходимость перехеширования
        if (static_cast<double>(size_ + 1) / capacity_ > load_factor_) {
            rehash();
        }
        
        size_t index = find_insert_index(key);
        if (index == capacity_) {
            return false; // Таблица полна
        }
        
        Node& node = table_[index];
        if (!node.occupied) {
            // Новая ячейка
            node.key = key;
            node.value = value;
            node.occupied = true;
            node.deleted = false;
            ++size_;
            return true;
        } else if (node.deleted) {
            // Заполняем удаленную ячейку
            node.key = key;
            node.value = value;
            node.occupied = true;
            node.deleted = false;
            ++size_;
            return true;
        } else {
            // Обновляем существующее значение
            node.value = value;
            return true;
        }
    }
    
    bool insert(K&& key, V&& value) {
        if (static_cast<double>(size_ + 1) / capacity_ > load_factor_) {
            rehash();
        }
        
        size_t index = find_insert_index(key);
        if (index == capacity_) {
            return false;
        }
        
        Node& node = table_[index];
        if (!node.occupied) {
            node.key = std::move(key);
            node.value = std::move(value);
            node.occupied = true;
            node.deleted = false;
            ++size_;
            return true;
        } else if (node.deleted) {
            node.key = std::move(key);
            node.value = std::move(value);
            node.occupied = true;
            node.deleted = false;
            ++size_;
            return true;
        } else {
            node.value = std::move(value);
            return true;
        }
    }
    
    // Поиск
    V* find(const K& key) {
        size_t index = find_index(key);
        if (index < capacity_) {
            return &table_[index].value;
        }
        return nullptr;
    }
    
    const V* find(const K& key) const {
        size_t index = find_index(key);
        if (index < capacity_) {
            return &table_[index].value;
        }
        return nullptr;
    }
    
    bool contains(const K& key) const {
        return find_index(key) < capacity_;
    }
    
    // Удаление
    bool erase(const K& key) {
        size_t index = find_index(key);
        if (index < capacity_) {
            table_[index].deleted = true;
            table_[index].occupied = false;
            --size_;
            return true;
        }
        return false;
    }
    
    // Доступ по ключу
    V& operator[](const K& key) {
        size_t index = find_index(key);
        if (index < capacity_) {
            return table_[index].value;
        }
        
        // Ключ не найден, вставляем со значением по умолчанию
        insert(key, V());
        index = find_index(key);
        return table_[index].value;
    }
    
    // Размер и емкость
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }
    
    // Очистка
    void clear() {
        for (size_t i = 0; i < capacity_; ++i) {
            table_[i].occupied = false;
            table_[i].deleted = false;
        }
        size_ = 0;
    }
    
    // Получение всех ключей и значений
    Vector<K> keys() const {
        Vector<K> result;
        for (size_t i = 0; i < capacity_; ++i) {
            if (table_[i].occupied && !table_[i].deleted) {
                result.push_back(table_[i].key);
            }
        }
        return result;
    }
    
    Vector<V> values() const {
        Vector<V> result;
        for (size_t i = 0; i < capacity_; ++i) {
            if (table_[i].occupied && !table_[i].deleted) {
                result.push_back(table_[i].value);
            }
        }
        return result;
    }
};

// Простые числа для размера таблицы
template<typename K, typename V, typename Hash, typename KeyEqual>
const size_t HashTable<K, V, Hash, KeyEqual>::PRIMES[] = {
    17, 31, 67, 127, 257, 509, 1021, 2053, 4099, 8191, 
    16381, 32771, 65537, 131071, 262147, 524287, 1048573
};

template<typename K, typename V, typename Hash, typename KeyEqual>
const size_t HashTable<K, V, Hash, KeyEqual>::PRIMES_COUNT = 
    sizeof(PRIMES) / sizeof(PRIMES[0]);

// Специализация для строк
template<>
struct std::hash<std::string> {
    size_t operator()(const std::string& str) const {
        size_t hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
        }
        return hash;
    }
};

} // namespace ds

#endif // HASH_TABLE_H