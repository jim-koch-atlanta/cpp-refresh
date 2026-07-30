#include <algorithm>
#include <cassert>
#include <chrono>
#include <format>
#include <memory_resource>
#include <optional>
#include <ranges>
#include <vector>

#include "../containers/unrolled-binary-tree.h"
#include "../allocators/pool-allocator.h"

using jim::containers::UnrolledBinaryTree;
using jim::containers::UnrolledBinaryTreeNode;

namespace jim {
    namespace app {
        void testIntBulkInsertions(int numberOfInsertions, int numberOfPerfRuns = 1) {
            std::cout << "===== testIntBulkInsertions =====" << std::endl;
            std::cout << "number of insertions: " << numberOfInsertions << std::endl;
            {
                std::cout << "=== std::allocator ===" << std::endl;

                // Run the workload numberOfPerfRuns times and keep the fastest -- the min
                // filters out OS scheduling noise better than a single sample or a mean.
                std::optional<std::chrono::duration<double, std::milli>> fastestRun;
                for (int perfRun : std::views::iota(0, numberOfPerfRuns)) {
                    // Fresh, identically-primed tree each run so every run measures the
                    // SAME workload from the SAME starting state (priming is untimed).
                    std::srand(1);
                    UnrolledBinaryTree<int, 1> tree;
                    for (int i : std::views::iota(1, 101)) {
                        tree.insert(std::rand());
                    }

                    // Timed section.
                    auto start = std::chrono::steady_clock::now();
                    for (int i : std::views::iota(1, numberOfInsertions + 1)) {
                        tree.insert(std::rand());
                    }
                    auto end = std::chrono::steady_clock::now();

                    std::chrono::duration<double, std::milli> elapsed = end - start;
                    if (!fastestRun || elapsed < *fastestRun) {
                        fastestRun = elapsed;
                    }
                }
                std::cout << "Fastest of " << numberOfPerfRuns << " run(s): "
                          << fastestRun.value().count() << " ms\n";
            }

            {
                std::cout << "=== PoolAllocator ===" << std::endl;

                std::optional<std::chrono::duration<double, std::milli>> fastestRun;
                for (int perfRun : std::views::iota(0, numberOfPerfRuns)) {
                    std::srand(1);
                    UnrolledBinaryTree<int, 1, jim::allocators::PoolAllocator<int, 10>> tree;
                    for (int i : std::views::iota(1, 101)) {
                        tree.insert(std::rand());
                    }

                    auto start = std::chrono::steady_clock::now();
                    for (int i : std::views::iota(1, numberOfInsertions + 1)) {
                        tree.insert(std::rand());
                    }
                    auto end = std::chrono::steady_clock::now();

                    std::chrono::duration<double, std::milli> elapsed = end - start;
                    if (!fastestRun || elapsed < *fastestRun) {
                        fastestRun = elapsed;
                    }
                }
                std::cout << "Fastest of " << numberOfPerfRuns << " run(s): "
                          << fastestRun.value().count() << " ms\n";
            }

            {
                std::cout << "=== pmr ===" << std::endl;

                // Run the workload numberOfPerfRuns times and keep the fastest -- the min
                // filters out OS scheduling noise better than a single sample or a mean.
                std::optional<std::chrono::duration<double, std::milli>> fastestRun;
                for (int perfRun : std::views::iota(0, numberOfPerfRuns)) {
                    // Fresh, identically-primed tree each run so every run measures the
                    // SAME workload from the SAME starting state (priming is untimed).
                    std::srand(1);
                    std::pmr::monotonic_buffer_resource buffer;
                    UnrolledBinaryTree<int, 1, std::pmr::polymorphic_allocator<int>> tree{ &buffer };
                    for (int i : std::views::iota(1, 101)) {
                        tree.insert(std::rand());
                    }

                    // Timed section.
                    auto start = std::chrono::steady_clock::now();
                    for (int i : std::views::iota(1, numberOfInsertions + 1)) {
                        tree.insert(std::rand());
                    }
                    auto end = std::chrono::steady_clock::now();

                    std::chrono::duration<double, std::milli> elapsed = end - start;
                    if (!fastestRun || elapsed < *fastestRun) {
                        fastestRun = elapsed;
                    }
                }
                std::cout << "Fastest of " << numberOfPerfRuns << " run(s): "
                          << fastestRun.value().count() << " ms\n";
            }
        }

        struct Operation {
            int value;
            int op; // 0 = insertion, 1 = delete
        };

        void testIntChurn(int numberOfOperations, int numberOfPerfRuns = 1) {
            std::cout << "===== testIntChurn =====" << std::endl;
            std::cout << "number of operations: " << numberOfOperations << std::endl;

            // We need to build a pseudo-random collection of insertions and deletions to run with both allocators.
            srand(46);
            std::vector<int> elements;
            std::vector<Operation> operations;
            for (int i : std::views::iota(1, numberOfOperations + 1)) {
                int op = std::rand() % 2;
                if (elements.size() == 0 || (op == 0)) {
                    // Insert if we chose the insertion operation, or if we have no elements.
                    int value = std::rand();

                    elements.push_back(value);
                    operations.push_back({value, 0});
                }
                else {
                    int index = std::rand() % elements.size();
                    int value = elements[index];
                    elements.erase(elements.begin() + index);
                    operations.push_back({value, 1});
                }
            }

            {
                std::cout << "=== std::allocator ===" << std::endl;

                std::optional<std::chrono::duration<double, std::milli>> fastestRun;
                for (int perfRun : std::views::iota(0, numberOfPerfRuns)) {
                    // Fresh, identically-primed tree each run (untimed) -> the replay below
                    // measures the same operation stream from the same starting state.
                    std::srand(1);
                    UnrolledBinaryTree<int, 1> tree;
                    for (int i : std::views::iota(1, 101)) {
                        tree.insert(std::rand());
                    }

                    // Timed section: replay the pre-built operation stream.
                    auto start = std::chrono::steady_clock::now();
                    for (Operation op : operations) {
                        if (op.op == 0) {
                            tree.insert(op.value);
                        } else {
                            if (!tree.remove(op.value)) throw std::runtime_error("Failed removal for existing element.");
                        }
                    }
                    auto end = std::chrono::steady_clock::now();

                    std::chrono::duration<double, std::milli> elapsed = end - start;
                    if (!fastestRun || elapsed < *fastestRun) {
                        fastestRun = elapsed;
                    }
                }
                std::cout << "Fastest of " << numberOfPerfRuns << " run(s): "
                          << fastestRun.value().count() << " ms\n";
            }

            {
                std::cout << "=== PoolAllocator ===" << std::endl;

                std::optional<std::chrono::duration<double, std::milli>> fastestRun;
                for (int perfRun : std::views::iota(0, numberOfPerfRuns)) {
                    std::srand(1);
                    UnrolledBinaryTree<int, 1, jim::allocators::PoolAllocator<int, 1000>> tree;
                    for (int i : std::views::iota(1, 101)) {
                        tree.insert(std::rand());
                    }

                    auto start = std::chrono::steady_clock::now();
                    for (Operation op : operations) {
                        if (op.op == 0) {
                            tree.insert(op.value);
                        } else {
                            if (!tree.remove(op.value)) throw std::runtime_error("Failed removal for existing element.");
                        }
                    }
                    auto end = std::chrono::steady_clock::now();

                    std::chrono::duration<double, std::milli> elapsed = end - start;
                    if (!fastestRun || elapsed < *fastestRun) {
                        fastestRun = elapsed;
                    }
                }
                std::cout << "Fastest of " << numberOfPerfRuns << " run(s): "
                          << fastestRun.value().count() << " ms\n";
            }

            {
                std::cout << "=== pmr ===" << std::endl;

                std::optional<std::chrono::duration<double, std::milli>> fastestRun;
                for (int perfRun : std::views::iota(0, numberOfPerfRuns)) {
                    // Fresh, identically-primed tree each run (untimed) -> the replay below
                    // measures the same operation stream from the same starting state.
                    std::srand(1);
                    std::pmr::monotonic_buffer_resource buffer;
                    UnrolledBinaryTree<int, 1, std::pmr::polymorphic_allocator<int>> tree{ &buffer };
                    for (int i : std::views::iota(1, 101)) {
                        tree.insert(std::rand());
                    }

                    // Timed section: replay the pre-built operation stream.
                    auto start = std::chrono::steady_clock::now();
                    for (Operation op : operations) {
                        if (op.op == 0) {
                            tree.insert(op.value);
                        } else {
                            if (!tree.remove(op.value)) throw std::runtime_error("Failed removal for existing element.");
                        }
                    }
                    auto end = std::chrono::steady_clock::now();

                    std::chrono::duration<double, std::milli> elapsed = end - start;
                    if (!fastestRun || elapsed < *fastestRun) {
                        fastestRun = elapsed;
                    }
                }
                std::cout << "Fastest of " << numberOfPerfRuns << " run(s): "
                          << fastestRun.value().count() << " ms\n";
            }
        }

        int main(int argc, char** argv) {
            (void)argc; (void)argv;

            constexpr int kRuns = 25;   // take the fastest of this many runs

            testIntBulkInsertions(100, kRuns);
            std::cout << std::endl;

            testIntBulkInsertions(10000, kRuns);
            std::cout << std::endl << std::endl;

            testIntChurn(100, kRuns);
            std::cout << std::endl;

            testIntChurn(10000, kRuns);
            std::cout << std::endl << std::endl;

            return 0;
        }
    }
}

int main(int argc, char** argv) {
    return jim::app::main(argc, argv);
}
