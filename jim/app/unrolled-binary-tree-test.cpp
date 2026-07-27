#include <algorithm>
#include <cassert>
#include <format>
#include <ranges>
#include <vector>

#include "../containers/unrolled-binary-tree.h"
#include "../allocators/pool-allocator.h"

using jim::containers::UnrolledBinaryTree;
using jim::containers::UnrolledBinaryTreeNode;

namespace jim {
    namespace app {
        void testInts() {
            std::cout << "=== testInts ===" << std::endl;
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
                    assert(true == tree.remove(i));
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

                assert(true == tree.remove(10));

                // Print the values in the tree.
                tree.print();
            }
        }

        void testIterator() {
            std::cout << "=== testIterator ===" << std::endl;
            {
                std::cout << "Case 1:" << std::endl;
                UnrolledBinaryTree<int, 10> tree;

                // Insert values 1 to 30.
                for (int i = 1; i <= 30; i++) {
                    tree.insert(i);
                }

                // Print the values in the tree.
                for (const auto& x : tree) {
                    std::cout << x << '\n';
                }

                // Use a range too.
                std::ranges::for_each(tree, [](const auto& x) { std::cout << x << ' '; });
                std::cout << '\n';
            }
        }

        void testRuleOfFive() {
            std::cout << "=== testRuleOfFive ===" << std::endl;
            {
                std::cout << "Case 1: Copy constructor" << std::endl;
                UnrolledBinaryTree<int, 10> tree1;
                UnrolledBinaryTree<int, 10> tree2(tree1);
            }

            {
                std::cout << "Case 2: Copy operator" << std::endl;
                UnrolledBinaryTree<int, 10> tree1;
                UnrolledBinaryTree<int, 10> tree2;
                tree2 = tree1;
            }

            {
                std::cout << "Case 3: Move constructor" << std::endl;
                UnrolledBinaryTree<int, 10> tree1;
                UnrolledBinaryTree<int, 10> tree2(std::move(tree1));
            }

            {
                std::cout << "Case 4: Move operator" << std::endl;
                UnrolledBinaryTree<int, 10> tree1;
                UnrolledBinaryTree<int, 10> tree2;
                tree2 = std::move(tree1);
            }
        }

        class Programmer {
        public:
            std::string firstName;
            std::string lastName;
            int codingAbility;
            int leadership;

            // friend so these can live in the class body, but are still non-member functions
            friend bool operator<(const Programmer& x, const Programmer& y) {
                return (x.codingAbility != y.codingAbility) ? (x.codingAbility < y.codingAbility) :
                       (x.leadership    != y.leadership)    ? (x.leadership    < y.leadership)    :
                       (x.lastName      != y.lastName)      ? (x.lastName      < y.lastName)      :
                                                              (x.firstName     < y.firstName);
            }

            friend bool operator>(const Programmer& x, const Programmer& y) {
                return (x.codingAbility != y.codingAbility) ? (x.codingAbility > y.codingAbility) :
                       (x.leadership    != y.leadership)    ? (x.leadership    > y.leadership)    :
                       (x.lastName      != y.lastName)      ? (x.lastName      > y.lastName)      :
                                                              (x.firstName     > y.firstName);
            }

            friend bool operator==(const Programmer& x, const Programmer& y) {
                return ((x.codingAbility == y.codingAbility) &&
                        (x.leadership == y.leadership) &&
                        (x.lastName == y.lastName) &&
                        (x.firstName == y.firstName));
            }

            friend bool operator<=(const Programmer& x, const Programmer& y) {
                return !(x > y);
            }

            friend bool operator>=(const Programmer& x, const Programmer& y) {
                return !(x < y);
            }

            // I could have just used the defaults like:
            // auto operator<=>(const Programmer&) const = default;
            
            friend std::ostream& operator<<(std::ostream& os, const Programmer& obj) {
                os << "Name: " << obj.firstName << " " << obj.lastName << std::endl;
                os << "Coding Ability: " << obj.codingAbility << std::endl;
                os << "Leadership: " << obj.leadership << std::endl;
                // 1. Write obj data to the stream (os)
                // 2. Return the stream to allow chaining
                return os; 
            }
        };

        void testUserDefinedTypes() {
            std::cout << "=== testUserDefinedTypes ===" << std::endl;
            {
                std::cout << "Case 1:" << std::endl;
                UnrolledBinaryTree<Programmer, 10> tree;

                for (int i = 1; i <= 10; i++) {
                    Programmer p{
                        std::format("FirstName #{}", i),
                        std::format("LastName #{}", i),
                        i,
                        i
                    };
                    tree.insert(p);
                }

                tree.print();
            }
        }

        // Exercises removing a node that has become empty and has two children.
        void testEmptyNodeRemoval() {
            std::cout << "=== testEmptyNodeRemoval ===" << std::endl;

            UnrolledBinaryTree<int, 2> tree;
            for (int v : {50, 51, 10, 90, 11, 5, 15}) {
                tree.insert(v);
            }

            auto inorder = [&] {
                std::vector<int> out;
                for (int x : tree) out.push_back(x);   // relies on the in-order iterator
                return out;
            };

            std::vector<int> before = inorder();
            std::cout << "before: ";
            for (int x : before) std::cout << x << ' ';
            std::cout << '\n';
            assert((before == std::vector<int>{5, 10, 11, 15, 50, 51, 90}));

            // Empty node C ([10, 11]) -> triggers the two-children merge.
            assert(true == tree.remove(10));
            assert(true == tree.remove(11));

            std::vector<int> after = inorder();
            std::cout << "after:  ";
            for (int x : after) std::cout << x << ' ';
            std::cout << '\n';

            // C is gone; its children [5] and [15] survived and are still in order.
            assert((after == std::vector<int>{5, 15, 50, 51, 90}));
        }

        // Exercise the custom allocator.
        void testCustomAllocator() {
            std::cout << "=== testCustomAllocator ===" << std::endl;

            UnrolledBinaryTree<int, 2, jim::allocators::PoolAllocator<int>> tree;
            for (int v : {50, 51, 10, 90, 11, 5, 15}) {
                tree.insert(v);
            }

            auto inorder = [&] {
                std::vector<int> out;
                for (int x : tree) out.push_back(x);   // relies on the in-order iterator
                return out;
            };

            std::vector<int> before = inorder();
            std::cout << "before: ";
            for (int x : before) std::cout << x << ' ';
            std::cout << '\n';
            assert((before == std::vector<int>{5, 10, 11, 15, 50, 51, 90}));

            // Empty node C ([10, 11]) -> triggers the two-children merge.
            assert(true == tree.remove(10));
            assert(true == tree.remove(11));

            std::vector<int> after = inorder();
            std::cout << "after:  ";
            for (int x : after) std::cout << x << ' ';
            std::cout << '\n';

            // C is gone; its children [5] and [15] survived and are still in order.
            assert((after == std::vector<int>{5, 15, 50, 51, 90}));
        }

        int main(int argc, char** argv) {
            testInts();
            testUserDefinedTypes();
            testIterator();
            testRuleOfFive();
            testEmptyNodeRemoval();
            testCustomAllocator();

            std::cout << "All tests passed.\n";
            return 0;
        }
    }
}

int main(int argc, char** argv) {
    return jim::app::main(argc, argv);
}