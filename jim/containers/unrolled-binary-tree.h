#include <iostream>
#include <iterator>

#include "../concepts/ordered.h"

namespace jim {
    namespace containers {
        template <typename T, int U>
        requires jim::concepts::Ordered<T>
        class UnrolledBinaryTreeNode {

        private:
            T elements[U];
            int element_count;
            UnrolledBinaryTreeNode<T, U>* left;
            UnrolledBinaryTreeNode<T, U>* right;
            UnrolledBinaryTreeNode<T, U>* parent; // if a given node becomes completely empty, it is removed

        public:
            UnrolledBinaryTreeNode(UnrolledBinaryTreeNode<T, U>* parent)
            : elements{ { } }
            , element_count{ 0 }
            , left{ nullptr }
            , right{ nullptr }
            , parent{ parent }
             { }

            UnrolledBinaryTreeNode()
            : elements{ { } }
            , element_count{ 0 }
            , left{ nullptr }
            , right{ nullptr }
            , parent{ nullptr }
             { }

        public:
            ~UnrolledBinaryTreeNode() {
                // Post-order recursive cleanup
                delete left;
                delete right;
            }

            // Nothing should be directly copying nodes.
            UnrolledBinaryTreeNode(const UnrolledBinaryTreeNode&) = delete;

            // Nothing should be directly copying nodes.
            UnrolledBinaryTreeNode& operator=(const UnrolledBinaryTreeNode&) = delete;

            UnrolledBinaryTreeNode* clone(UnrolledBinaryTreeNode* parent = nullptr) const {
                auto* newNode = new UnrolledBinaryTreeNode();

                newNode->element_count = element_count;
                for (int i = 0; i < element_count; ++i) {
                    newNode->elements[i] = elements[i];  // reads privates fine
                }

                newNode->parent = parent;

                if (left) {
                    newNode->left = left->clone(newNode);
                } else {
                    newNode->left = nullptr;
                }

                if (right) {
                    newNode->right = right->clone(newNode);
                } else {
                    newNode->right = nullptr;
                }

                return newNode;
            }

            bool contains(T value) {
                if (this->element_count == 0) {
                    return false;
                }

                if (value < this->elements[0]) {
                    if (this->left == nullptr) {
                        return false;
                    }
                    return this->left->contains(value);
                }

                if (value > this->elements[this->element_count - 1]) {
                    if (this->right == nullptr) {
                        return false;
                    }
                    return this->right->contains(value);
                }

                // TODO: We could possibly make this a binary search, since it's in-order.
                for (int index = 0; index < element_count; index++) {
                    if (elements[index] == value) {
                        return true;
                    }
                }

                return false;
            }

            void insert(T value) {
                if (this->element_count == U) {
                    if (value < elements[0]) {
                        if (this->left == nullptr) {
                            // Create a new node. This is its parent.
                            this->left = new UnrolledBinaryTreeNode<T, U>(this);
                        }
                        this->left->insert(value);
                    } else if (value > elements[this->element_count - 1]) {
                        if (this->right == nullptr) {
                            // Create a new node. This is its parent.
                            this->right = new UnrolledBinaryTreeNode<T, U>(this);
                        }
                        this->right->insert(value);
                    } else {
                        // TODO: Choose left or right to be more balanced.
                        if (this->left == nullptr) {
                            // Create a new node. This is its parent.
                            this->left = new UnrolledBinaryTreeNode<T, U>(this);
                        }
                        this->left->insert(elements[0]);
                        // TODO: Switch this from a for-loop to maybe using iterator.
                        int index;
                        for (index = 0; index < element_count - 1; index++) {
                            // Let's say we previously had:
                            //   1 2 3 4 6
                            // and we're attempting to insert 5.
                            // We already copy the value 1 to left.
                            if (this->elements[index + 1] < value) {
                                // 1. We copy the value 2 to 1. (index = 0)
                                // 2. We copy the value 3 to 2. (index = 1)
                                // 3. We copy the value 4 to 3. (index = 2)
                                this->elements[index] = this->elements[index + 1];
                            } else {
                                break;
                            }
                        }
                        // Now index = 3, and we need to set elements[index] to value.
                        this->elements[index] = value;
                    }
                } else {
                    // This node is not full. Just insert in the right place.
                    int index = element_count;
                    for (; index > 0; index--) {
                        if (this->elements[index - 1] > value) {
                            this->elements[index] = this->elements[index - 1];
                        } else {
                            break;
                        }
                    }
                    this->elements[index] = value;
                    element_count++;
                }
            }

            bool remove(T value) {
                // TODO: Get rid of this node if its element_count drops to 0.
                if (element_count > 0) {
                    if (value < elements[0]) {
                        if (left != nullptr) {
                            return left->remove(value);
                        }
                        return false;
                    } else if (value > elements[element_count - 1]) {
                        if (right != nullptr) {
                            return right->remove(value);
                        }
                        return false;
                    }
                    for (int index = 0; index < element_count; index++) {
                        if (elements[index] == value) {
                            // We found the value. Delete it by shifting anything to its right over.
                            for (int index2 = index; index2 < element_count - 1; index2++) {
                                elements[index2] = elements[index2 + 1];
                            }
                            element_count--;
                            break;
                        }
                    }

                    return false;
                }

                return ((left != nullptr && left->remove(value)) ||
                        (right != nullptr && right->remove(value)));
            }

            void print() {
                if (left != nullptr) {
                    left->print();
                }

                for (int index = 0; index < element_count; index++) {
                    std::cout << elements[index] << std::endl;
                }

                if (right != nullptr) {
                    right->print();
                }
            }

            // --- The Iterator Class ---
            class Iterator {
            public:
                // Required type aliases for STL compatibility
                using iterator_category = std::forward_iterator_tag;
                using value_type        = T;
                using difference_type   = std::ptrdiff_t;
                using pointer           = T*;
                using reference         = T&;

                // Default constructor, needed for algorithms.
                Iterator() : node(nullptr), index(0) {}

                // Constructor
                explicit Iterator(UnrolledBinaryTreeNode<T, U>* node, int index)
                    : node(node)
                    , index(index) {}

                // 1. Dereference operators
                reference operator*() const { return node->elements[index]; }
                pointer operator->() { return &(node->elements[index]); }

                // 2. Prefix increment: ++it
                Iterator& operator++() {
                    index++;
                    if (index == node->element_count) {
                        index = 0;

                        if (node->right != nullptr) {
                            node = node->right;
                            while (node->left != nullptr) {
                                node = node->left;
                            }
                        } else {
                            // Keep traversing up the tree while this is the "right" child.
                            // Then traverse up once to the left.
                            while ((node->parent != nullptr) && (node->parent->right == node)) {
                                node = node->parent;
                            }

                            // This should be the "left" child now. Go up one more level.
                            node = node->parent;
                        }
                    }

                    return *this;
                }

                // 3. Postfix increment: it++
                Iterator operator++(int) {
                    Iterator tmp = Iterator(node, index);

                    index++;
                    if (index == node->element_count) {
                        index = 0;

                        if (node->right != nullptr) {
                            node = node->right;
                            while (node->left != nullptr) {
                                node = node->left;
                            }
                        } else {
                            // Keep traversing up the tree while this is the "right" child.
                            // Then traverse up once to the left.
                            while ((node->parent != nullptr) && (node->parent->right == node)) {
                                node = node->parent;
                            }

                            // This should be the "left" child now. Go up one more level.
                            node = node->parent;
                        }
                    }

                    return tmp;
                }

                // 4. Comparison operators
                friend bool operator==(const Iterator& a, const Iterator& b) { return (a.node == b.node) && (a.index == b.index); }
                friend bool operator!=(const Iterator& a, const Iterator& b) { return (a.node != b.node) || (a.index != b.index); }

            private:
                UnrolledBinaryTreeNode<T, U>* node;
                int index;
            };

            // --- Container Methods ---
            // Returns an iterator to the first element
            Iterator begin() {
                if (this->left != nullptr) {
                    return this->left->begin();
                }

                return Iterator(this, 0);
            }
            
            // Returns an iterator to the position past the last element
            Iterator end()   {
                if (this->right != nullptr) {
                    return this->right->end();
                }

                return Iterator(nullptr, 0);
            }
        };

        template <typename T, int U>
        requires jim::concepts::Ordered<T>
        class UnrolledBinaryTree {

        private:
            UnrolledBinaryTreeNode<T, U>* root;

        public:
            UnrolledBinaryTree() {
                root = new UnrolledBinaryTreeNode<T, U>();
            };

            // 1. Destructor
            ~UnrolledBinaryTree() {
                delete root;
            }

            // 2. Copy Constructor (Deep Copy)
            UnrolledBinaryTree(const UnrolledBinaryTree& other) {
                root = other.root->clone();
                std::cout << "Copy constructed\n";
            }

            friend void swap(UnrolledBinaryTree& first, UnrolledBinaryTree& second) noexcept {
                std::swap(first.root, second.root);
            }

            // 3. Copy Operator (via copy-and-swap idiom)
            UnrolledBinaryTree& operator=(const UnrolledBinaryTree& other) {
                UnrolledBinaryTree tmp(other);   // deep copy (via your copy ctor / clone)
                swap(*this, tmp);                // swap the pointers
                return *this;
            }                                    // tmp destructs → frees *this's OLD tree

            // 4. Move Constructor
            UnrolledBinaryTree(UnrolledBinaryTree&& other) noexcept {
                root = other.root;
                other.root = nullptr;
                std::cout << "Move constructed\n";
            }

            // 5. Move Operator
            UnrolledBinaryTree& operator=(UnrolledBinaryTree&& other) noexcept {
                if (this != &other) {
                    delete root;
                    root = other.root;
                    other.root = nullptr;
                }

                std::cout << "Move operated\n";

                return *this;
        }

            bool contains(T value) {
                return root->contains(value);
            }

            void insert(T value) {
                return root->insert(value);
            }

            bool remove(T value) {
                return root->remove(value);
            }

            void print() {
                std::cout << "-----" << std::endl;
                root->print();
                std::cout << "-----" << std::endl;
            }

            // Re-export of UnrolledBinaryTreeNode iterator.
            using Iterator = typename UnrolledBinaryTreeNode<T, U>::Iterator;

            Iterator begin() { return root->begin(); }
            Iterator end()   { return root->end(); }
        };
   }
}