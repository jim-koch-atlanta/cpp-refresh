#pragma once

#include <cstddef>
#include <cstdlib>
#include <set>

namespace jim {

    namespace allocators {

        template <typename T, std::size_t PoolSize = 10>
        class PoolAllocator {
            union FreeNode {
                FreeNode* next;                          // free-list link when slot is free
                alignas(T) std::byte storage[sizeof(T)]; // object storage when slot is live
            };

        public:
            using value_type = T;

            template <typename U>
            struct rebind { using other = PoolAllocator<U, PoolSize>; };

            PoolAllocator() {
                allocator_id = next_allocator_id;
                next_allocator_id++;
            }

            // owns a fixed buffer and hands out internal pointers -> non-copyable
            PoolAllocator(const PoolAllocator&)            = delete;
            PoolAllocator& operator=(const PoolAllocator&) = delete;

            T* allocate(std::size_t n) {                    // pop the head of the free list
                if (!initialized_) {
                    // Create a free list on first allocation. Slow, but this is a
                    // learning exercise on allocators.
                    for (std::size_t i = 0; i < PoolSize; ++i) {
                        auto* node = reinterpret_cast<FreeNode*>(&pool_[i * sizeof(FreeNode)]);
                        node->next = head;
                        head = node;
                    }
                    initialized_ = true;
                }

                if ((head == nullptr) || (n > 1)) {
                    // We've run out of pool nodes, or more multiple contiguous nodes
                    // were requested. Create on the heap.
                    T* node = (T*) std::malloc(sizeof(T) * n);
                    heap_nodes.insert(node);
                    return node;
                }

                FreeNode* node = head;
                head = node->next;
                ++allocated_count_;
                return reinterpret_cast<T*>(node);
            }

            void deallocate(T* p, std::size_t n) noexcept {   // push back onto the head
                if (!p) return;

                if (heap_nodes.find(p) != heap_nodes.end()) {
                    heap_nodes.erase(p);
                    std::free(p);
                    return;
                }

                auto* node = reinterpret_cast<FreeNode*>(p);
                node->next = head;
                head = node;
                --allocated_count_;
            }

            std::size_t allocated() const noexcept { return allocated_count_; }

            // Required equality operator
            bool operator==(const PoolAllocator& other) const {
                return allocator_id == other.allocator_id;
            }

            // Required inequality operator
            bool operator!=(const PoolAllocator& other) const {
                return !(*this == other);
            }

        private:
            alignas(T) std::byte pool_[PoolSize * sizeof(FreeNode)];
            FreeNode*   head = nullptr;

            // Live pool count.
            std::size_t allocated_count_ = 0;

            // Initialization happens on first allocation.
            bool initialized_ = false;

            // A list of any nodes that were created outside of the pool.
            std::set<T*> heap_nodes;
            
            // This allocator's ID.
            int allocator_id;

            static inline int next_allocator_id = 0;
        };
    }
}