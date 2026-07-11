#pragma once

#include <iostream>
#include <iterator>
#include <memory>
#include <utility>

#include "../concepts/ordered.h"

namespace jim {
    namespace containers {

        // The node stays "pure structure": it knows nothing about allocation.
        // Any method that needs to create/destroy nodes takes a `Factory&`
        // (see NodeFactory below) -- threaded in, never stored, so nodes stay small.
        template <typename T, int U>
        requires jim::concepts::Ordered<T>
        class UnrolledBinaryTreeNode {

            // The factory owns the allocator and needs to read left/right during
            // teardown, so every NodeFactory specialization is a friend.
            template <typename, int, typename> friend class NodeFactory;

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

            // Nothing should be directly copying nodes.
            UnrolledBinaryTreeNode(const UnrolledBinaryTreeNode&) = delete;
            UnrolledBinaryTreeNode& operator=(const UnrolledBinaryTreeNode&) = delete;

            // Deep-copy this subtree. Allocates via the factory, so it is threaded in.
            template <typename Factory>
            UnrolledBinaryTreeNode* clone(Factory& factory, UnrolledBinaryTreeNode* parent = nullptr) const {
                auto* newNode = factory.make_node();

                newNode->element_count = element_count;
                for (int i = 0; i < element_count; ++i) {
                    newNode->elements[i] = elements[i];  // reads privates fine
                }

                newNode->parent = parent;

                if (left) {
                    newNode->left = left->clone(factory, newNode);
                } else {
                    newNode->left = nullptr;
                }

                if (right) {
                    newNode->right = right->clone(factory, newNode);
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

            template <typename Factory>
            void removeChildNode(UnrolledBinaryTreeNode* childNode, Factory& factory) {
                // If it's a leaf node, just delete it.
                if (childNode->left == nullptr && childNode->right == nullptr) {
                    if (childNode == left) {
                        left = nullptr;
                    }
                    if (childNode == right) {
                        right = nullptr;
                    }
                    factory.destroy_node(childNode);
                    return;
                }

                if (childNode == left) {
                    if (childNode->left == nullptr) {
                        left = childNode->right;
                        childNode->right->parent = this;
                        childNode->right = nullptr;
                        factory.destroy_node(childNode);
                        return;
                    } else if (childNode->right == nullptr) {
                        left = childNode->left;
                        childNode->left->parent = this;
                        childNode->left = nullptr;
                        factory.destroy_node(childNode);
                        return;
                    } else { // the child node has two children.
                        // Merge childNode's subtrees: everything in its left subtree (L)
                        // is < everything in its right subtree (R), so hang R off the
                        // rightmost (max) node of L, then promote L into this slot.
                        left = childNode->left;
                        childNode->left->parent = this;

                        auto maxOfLeft = childNode->left;
                        while (maxOfLeft->right != nullptr) {
                            maxOfLeft = maxOfLeft->right;
                        }
                        maxOfLeft->right = childNode->right;
                        childNode->right->parent = maxOfLeft;

                        childNode->left = nullptr;
                        childNode->right = nullptr;
                        factory.destroy_node(childNode);
                    }
                }
                else if (childNode == right) {
                    if (childNode->left == nullptr) {
                        right = childNode->right;
                        childNode->right->parent = this;
                        childNode->right = nullptr;
                        factory.destroy_node(childNode);
                        return;
                    } else if (childNode->right == nullptr) {
                        right = childNode->left;
                        childNode->left->parent = this;
                        childNode->left = nullptr;
                        factory.destroy_node(childNode);
                        return;
                    } else { // the child node has two children.
                        // Same merge as the left branch, but promoting into `right`.
                        right = childNode->left;
                        childNode->left->parent = this;

                        auto maxOfLeft = childNode->left;
                        while (maxOfLeft->right != nullptr) {
                            maxOfLeft = maxOfLeft->right;
                        }
                        maxOfLeft->right = childNode->right;
                        childNode->right->parent = maxOfLeft;

                        childNode->left = nullptr;
                        childNode->right = nullptr;
                        factory.destroy_node(childNode);
                    }
                }
                else {
                    // Should never happen.
                }
            }

            template <typename Factory>
            void insert(T value, Factory& factory) {
                if (this->element_count == U) {
                    if (value < elements[0]) {
                        if (this->left == nullptr) {
                            // Create a new node. This is its parent.
                            this->left = factory.make_node(this);
                        }
                        this->left->insert(value, factory);
                    } else if (value > elements[this->element_count - 1]) {
                        if (this->right == nullptr) {
                            // Create a new node. This is its parent.
                            this->right = factory.make_node(this);
                        }
                        this->right->insert(value, factory);
                    } else {
                        // TODO: Choose left or right to be more balanced.
                        if (this->left == nullptr) {
                            // Create a new node. This is its parent.
                            this->left = factory.make_node(this);
                        }
                        this->left->insert(elements[0], factory);
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

            template <typename Factory>
            bool remove(T value, Factory& factory) {
                if (element_count > 0) {
                    if (value < elements[0]) {
                        if (left != nullptr) {
                            auto result = left->remove(value, factory);
                            if (left->element_count == 0) {
                                removeChildNode(left, factory);
                            }
                            return result;
                        }
                        return false;
                    } else if (value > elements[element_count - 1]) {
                        if (right != nullptr) {
                            auto result = right->remove(value, factory);
                            if (right->element_count == 0) {
                                removeChildNode(right, factory);
                            }
                            return result;
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

                return ((left != nullptr && left->remove(value, factory)) ||
                        (right != nullptr && right->remove(value, factory)));
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

        // Owns the (rebound) allocator and is the single place that creates and
        // destroys nodes. The tree holds one of these and threads it into the
        // node methods that allocate.
        template <typename T, int U, typename Alloc = std::allocator<T>>
        class NodeFactory {
            using Node       = UnrolledBinaryTreeNode<T, U>;
            using NodeAlloc  = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
            using NodeTraits = std::allocator_traits<NodeAlloc>;

            NodeAlloc alloc_;

        public:
            NodeFactory() = default;
            explicit NodeFactory(const Alloc& a) : alloc_(a) {}

            // allocate raw memory for one node, then construct the node in it
            template <typename... Args>
            Node* make_node(Args&&... args) {
                Node* p = NodeTraits::allocate(alloc_, 1);
                NodeTraits::construct(alloc_, p, std::forward<Args>(args)...);
                return p;
            }

            // destroy one node (run its destructor), then free its memory
            void destroy_node(Node* p) {
                NodeTraits::destroy(alloc_, p);
                NodeTraits::deallocate(alloc_, p, 1);
            }

            // post-order teardown of an entire subtree
            void destroy_subtree(Node* p) {
                if (p != nullptr) {
                    destroy_subtree(p->left);
                    destroy_subtree(p->right);
                    destroy_node(p);
                }
            }
        };

        template <typename T, int U, typename Alloc = std::allocator<T>>
        requires jim::concepts::Ordered<T>
        class UnrolledBinaryTree {

            using Node    = UnrolledBinaryTreeNode<T, U>;
            using Factory = NodeFactory<T, U, Alloc>;

        private:
            Node* root;
            Factory factory;   // owns the allocator; single source of node lifecycle

        public:
            UnrolledBinaryTree() {
                root = factory.make_node();
            }

            // 1. Destructor.
            ~UnrolledBinaryTree() {
                // DFS teardown via the factory, since we support custom allocators.
                factory.destroy_subtree(root);
            }

            // 2. Copy Constructor (Deep Copy)
            UnrolledBinaryTree(const UnrolledBinaryTree& other) {
                root = other.root->clone(factory);
                std::cout << "Copy constructed\n";
            }

            friend void swap(UnrolledBinaryTree& first, UnrolledBinaryTree& second) noexcept {
                std::swap(first.root, second.root);
                // NOTE: stateless std::allocator -> memory is interchangeable, so we don't
                // swap `factory`. A *stateful* allocator (e.g. the ring buffer) would need
                // allocator-propagation handling here.
            }

            // 3. Copy Operator (via copy-and-swap idiom)
            UnrolledBinaryTree& operator=(const UnrolledBinaryTree& other) {
                UnrolledBinaryTree tmp(other);   // deep copy (via copy ctor / clone)
                swap(*this, tmp);                // swap the roots
                std::cout << "Copy operated\n";
                return *this;
            }                                    // tmp destructs -> frees *this's OLD tree

            // 4. Move Constructor
            UnrolledBinaryTree(UnrolledBinaryTree&& other) noexcept {
                root = other.root;
                other.root = nullptr;
                std::cout << "Move constructed\n";
            }

            // 5. Move Operator
            UnrolledBinaryTree& operator=(UnrolledBinaryTree&& other) noexcept {
                if (this != &other) {
                    factory.destroy_subtree(root);   // free our whole existing tree
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
                root->insert(value, factory);
            }

            bool remove(T value) {
                return root->remove(value, factory);
            }

            void print() {
                std::cout << "-----" << std::endl;
                root->print();
                std::cout << "-----" << std::endl;
            }

            // Re-export of UnrolledBinaryTreeNode iterator.
            using Iterator = typename Node::Iterator;

            Iterator begin() { return root->begin(); }
            Iterator end()   { return root->end(); }
        };
   }
}
