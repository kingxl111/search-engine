#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <utility>
#include <functional>
#include <stdexcept>
#include <algorithm>

namespace ds {

template<typename K, typename V, typename Compare = std::less<K>>
class AVLTree {
private:
    struct Node {
        K key;
        V value;
        Node* left;
        Node* right;
        int height;
        
        Node(const K& k, const V& v) : 
            key(k), value(v), left(nullptr), right(nullptr), height(1) {}
        
        Node(K&& k, V&& v) : 
            key(std::move(k)), value(std::move(v)), left(nullptr), right(nullptr), height(1) {}
    };
    
    Node* root_;
    size_t size_;
    Compare comp_;
    
    // Вспомогательные методы
    int height(Node* node) const {
        return node ? node->height : 0;
    }
    
    int balance_factor(Node* node) const {
        return node ? height(node->left) - height(node->right) : 0;
    }
    
    void update_height(Node* node) {
        if (node) {
            node->height = 1 + std::max(height(node->left), height(node->right));
        }
    }
    
    // Повороты
    Node* rotate_right(Node* y) {
        Node* x = y->left;
        Node* T2 = x->right;
        
        x->right = y;
        y->left = T2;
        
        update_height(y);
        update_height(x);
        
        return x;
    }
    
    Node* rotate_left(Node* x) {
        Node* y = x->right;
        Node* T2 = y->left;
        
        y->left = x;
        x->right = T2;
        
        update_height(x);
        update_height(y);
        
        return y;
    }
    
    // Балансировка
    Node* balance(Node* node) {
        if (!node) return nullptr;
        
        update_height(node);
        int bf = balance_factor(node);
        
        // Левый-левый случай
        if (bf > 1 && balance_factor(node->left) >= 0) {
            return rotate_right(node);
        }
        
        // Левый-правый случай
        if (bf > 1 && balance_factor(node->left) < 0) {
            node->left = rotate_left(node->left);
            return rotate_right(node);
        }
        
        // Правый-правый случай
        if (bf < -1 && balance_factor(node->right) <= 0) {
            return rotate_left(node);
        }
        
        // Правый-левый случай
        if (bf < -1 && balance_factor(node->right) > 0) {
            node->right = rotate_right(node->right);
            return rotate_left(node);
        }
        
        return node;
    }
    
    // Вставка
    Node* insert(Node* node, const K& key, const V& value) {
        if (!node) {
            ++size_;
            return new Node(key, value);
        }
        
        if (comp_(key, node->key)) {
            node->left = insert(node->left, key, value);
        } else if (comp_(node->key, key)) {
            node->right = insert(node->right, key, value);
        } else {
            // Ключ уже существует, обновляем значение
            node->value = value;
            return node;
        }
        
        return balance(node);
    }
    
    Node* insert(Node* node, K&& key, V&& value) {
        if (!node) {
            ++size_;
            return new Node(std::move(key), std::move(value));
        }
        
        if (comp_(key, node->key)) {
            node->left = insert(node->left, std::move(key), std::move(value));
        } else if (comp_(node->key, key)) {
            node->right = insert(node->right, std::move(key), std::move(value));
        } else {
            node->value = std::move(value);
            return node;
        }
        
        return balance(node);
    }
    
    // Поиск минимального узла
    Node* find_min(Node* node) const {
        while (node && node->left) {
            node = node->left;
        }
        return node;
    }
    
    // Удаление минимального узла
    Node* remove_min(Node* node) {
        if (!node->left) {
            return node->right;
        }
        node->left = remove_min(node->left);
        return balance(node);
    }
    
    // Удаление
    Node* remove(Node* node, const K& key) {
        if (!node) return nullptr;
        
        if (comp_(key, node->key)) {
            node->left = remove(node->left, key);
        } else if (comp_(node->key, key)) {
            node->right = remove(node->right, key);
        } else {
            // Ключ найден
            Node* left = node->left;
            Node* right = node->right;
            
            delete node;
            --size_;
            
            if (!right) return left;
            
            Node* min = find_min(right);
            min->right = remove_min(right);
            min->left = left;
            
            return balance(min);
        }
        
        return balance(node);
    }
    
    // Поиск
    Node* find(Node* node, const K& key) const {
        while (node) {
            if (comp_(key, node->key)) {
                node = node->left;
            } else if (comp_(node->key, key)) {
                node = node->right;
            } else {
                return node;
            }
        }
        return nullptr;
    }
    
    // Копирование дерева
    Node* copy_tree(Node* node) {
        if (!node) return nullptr;
        
        Node* new_node = new Node(node->key, node->value);
        new_node->height = node->height;
        new_node->left = copy_tree(node->left);
        new_node->right = copy_tree(node->right);
        
        return new_node;
    }
    
    // Очистка дерева
    void clear_tree(Node* node) {
        if (node) {
            clear_tree(node->left);
            clear_tree(node->right);
            delete node;
        }
    }
    
    // Обходы
    template<typename Func>
    void inorder(Node* node, Func func) const {
        if (node) {
            inorder(node->left, func);
            func(node->key, node->value);
            inorder(node->right, func);
        }
    }
    
    template<typename Func>
    void preorder(Node* node, Func func) const {
        if (node) {
            func(node->key, node->value);
            preorder(node->left, func);
            preorder(node->right, func);
        }
    }
    
    template<typename Func>
    void postorder(Node* node, Func func) const {
        if (node) {
            postorder(node->left, func);
            postorder(node->right, func);
            func(node->key, node->value);
        }
    }

public:
    // Конструкторы
    AVLTree() : root_(nullptr), size_(0) {}
    
    AVLTree(const Compare& comp) : root_(nullptr), size_(0), comp_(comp) {}
    
    AVLTree(const AVLTree& other) : size_(other.size_), comp_(other.comp_) {
        root_ = copy_tree(other.root_);
    }
    
    AVLTree(AVLTree&& other) noexcept : 
        root_(other.root_), size_(other.size_), comp_(std::move(other.comp_)) {
        other.root_ = nullptr;
        other.size_ = 0;
    }
    
    // Деструктор
    ~AVLTree() {
        clear_tree(root_);
    }
    
    // Операторы присваивания
    AVLTree& operator=(const AVLTree& other) {
        if (this != &other) {
            clear_tree(root_);
            comp_ = other.comp_;
            size_ = other.size_;
            root_ = copy_tree(other.root_);
        }
        return *this;
    }
    
    AVLTree& operator=(AVLTree&& other) noexcept {
        if (this != &other) {
            clear_tree(root_);
            root_ = other.root_;
            size_ = other.size_;
            comp_ = std::move(other.comp_);
            other.root_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
    
    // Вставка
    void insert(const K& key, const V& value) {
        root_ = insert(root_, key, value);
    }
    
    void insert(K&& key, V&& value) {
        root_ = insert(root_, std::move(key), std::move(value));
    }
    
    // Поиск
    V* find(const K& key) {
        Node* node = find(root_, key);
        return node ? &node->value : nullptr;
    }
    
    const V* find(const K& key) const {
        Node* node = find(root_, key);
        return node ? &node->value : nullptr;
    }
    
    bool contains(const K& key) const {
        return find(root_, key) != nullptr;
    }
    
    // Удаление
    bool erase(const K& key) {
        size_t old_size = size_;
        root_ = remove(root_, key);
        return size_ < old_size;
    }
    
    // Доступ по ключу
    V& operator[](const K& key) {
        Node* node = find(root_, key);
        if (node) {
            return node->value;
        }
        
        // Вставляем значение по умолчанию
        insert(key, V());
        node = find(root_, key);
        return node->value;
    }
    
    // Размер
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    // Очистка
    void clear() {
        clear_tree(root_);
        root_ = nullptr;
        size_ = 0;
    }
    
    // Обходы
    template<typename Func>
    void inorder(Func func) const {
        inorder(root_, func);
    }
    
    template<typename Func>
    void preorder(Func func) const {
        preorder(root_, func);
    }
    
    template<typename Func>
    void postorder(Func func) const {
        postorder(root_, func);
    }
    
    // Получение минимального и максимального ключей
    K min_key() const {
        if (!root_) {
            throw std::runtime_error("Tree is empty");
        }
        return find_min(root_)->key;
    }
    
    K max_key() const {
        if (!root_) {
            throw std::runtime_error("Tree is empty");
        }
        Node* node = root_;
        while (node->right) {
            node = node->right;
        }
        return node->key;
    }
    
    // Проверка балансировки (для отладки)
    bool is_balanced() const {
        return is_balanced(root_);
    }
    
private:
    bool is_balanced(Node* node) const {
        if (!node) return true;
        
        int bf = balance_factor(node);
        if (bf < -1 || bf > 1) return false;
        
        return is_balanced(node->left) && is_balanced(node->right);
    }
};

} // namespace ds

#endif // AVL_TREE_H