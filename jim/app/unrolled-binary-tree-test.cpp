#include "../containers/unrolled-binary-tree.h"

using jim::containers::UnrolledBinaryTree;
using jim::containers::UnrolledBinaryTreeNode;

namespace jim {
    namespace app {
        int main(int argc, char** argv) {
            {
                std::cout << "Case 1:" << std::endl;
                UnrolledBinaryTree<int, 10> tree;

                // Insert values 1 to 30.
                for (int i = 1; i <= 30; i++) {
                    tree.insert(i);
                }

                // Print the values in the tree.
                tree.print();
            }

            {
                std::cout << "Case 2:" << std::endl;
                UnrolledBinaryTree<int, 10> tree;

                // Insert values 11 to 20.
                for (int i = 11; i <= 20; i++) {
                    tree.insert(i);
                }

                // Insert values 1 to 10.
                for (int i = 1; i <= 10; i++) {
                    tree.insert(i);
                }

                // Insert values 21 to 30.
                for (int i = 21; i <= 30; i++) {
                    tree.insert(i);
                }

                // Print the values in the tree.
                tree.print();
            }

            {
                std::cout << "Case 3:" << std::endl;
                UnrolledBinaryTree<int, 10> tree;

                // Insert values 11 to 20.
                for (int i = 11; i <= 20; i++) {
                    tree.insert(i);
                }

                // Insert values 1 to 10.
                for (int i = 1; i <= 10; i++) {
                    tree.insert(i);
                }

                // **Delete** values 11 to 20.
                for (int i = 11; i <= 20; i++) {
                    tree.remove(i);
                }

                // Insert values 21 to 30.
                for (int i = 21; i <= 30; i++) {
                    tree.insert(i);
                }

                // Insert values 11 to 20 **again**.
                for (int i = 11; i <= 20; i++) {
                    tree.insert(i);
                }

                // Print the values in the tree.
                tree.print();
            }

            {
                std::cout << "Case 4:" << std::endl;
                UnrolledBinaryTree<int, 10> tree;

                // Insert values 1 to 10.
                for (int i = 1; i <= 10; i++) {
                    tree.insert(i);
                }

                tree.remove(10);

                // Print the values in the tree.
                tree.print();
            }

            std::cout << "All tests passed.\n";
            return 0;
        }
    }
}

int main(int argc, char** argv) {
    return jim::app::main(argc, argv);
}