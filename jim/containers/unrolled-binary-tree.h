#include <iostream>

namespace jim {
    namespace containers {
        template <typename T, int U>
        class UnrolledBinaryTreeNode {
        public:
            T elements[U];
            int element_count;
            UnrolledBinaryTreeNode<T, U>* left;
            UnrolledBinaryTreeNode<T, U>* right;
            UnrolledBinaryTreeNode<T, U>* parent; // if a given node becomes completely empty, it is removed

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
        };

        template <typename T, int U>
        class UnrolledBinaryTree {
        public:
            UnrolledBinaryTreeNode<T, U> root;

            UnrolledBinaryTree() {};

            bool contains(T value) {
                return root.contains(value);
            }

            void insert(T value) {
                return root.insert(value);
            }

            bool remove(T value) {
                return root.remove(value);
            }

            void print() {
                std::cout << "-----" << std::endl;
                root.print();
                std::cout << "-----" << std::endl;
            }
        };
   }
}