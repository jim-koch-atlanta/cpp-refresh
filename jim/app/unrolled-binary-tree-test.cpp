#include <format>

#include "../containers/unrolled-binary-tree.h"

using jim::containers::UnrolledBinaryTree;
using jim::containers::UnrolledBinaryTreeNode;

namespace jim {
    namespace app {
        void testInts() {
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
        }

        void testIterator() {
            {
                std::cout << "Case 1:" << std::endl;
                UnrolledBinaryTree<int, 10> tree;

                // Insert values 1 to 30.
                for (int i = 1; i <= 30; i++) {
                    tree.insert(i);
                }

                // Print the values in the tree.
                for (auto it = tree.begin(); it != tree.end(); it++) {
                    std::cout << *it << std::endl;
                }
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

        int main(int argc, char** argv) {
            testInts();
            testUserDefinedTypes();
            testIterator();
            
            std::cout << "All tests passed.\n";
            return 0;
        }
    }
}

int main(int argc, char** argv) {
    return jim::app::main(argc, argv);
}