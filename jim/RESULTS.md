# Analysis: Binary Search Tree Performance with Various Allocators

**Author**: Jim Koch
**Date**: 2026-07-27 (July 27th)

## Project Description

I set up this project as a way to reinforce the fundamentals of C++, especially in C++17 and C++20. It was an opportunity to use:
* Templating
* Concepts
* Iterators and ranges
* Allocators

[UnrolledBinaryTree](./containers/unrolled-binary-tree.h) is a binary tree where each node stores an **array** of elements. There are very few use cases where this provides performance benefits, but it was a good change from a typical binary search tree.

The tree is templated, and it uses the [Ordered](./concepts/ordered.h) concept / constraint for its elements (i.e. the elements must be comparable, supporting `<`, `>`, `<=`, `>=`, and `==`).

The tree also accepts an allocator type. By default it will use `std::allocator`, but it can also use any other allocator type. I compared it to my own [PoolAllocator](./allocators/pool-allocator.h) and to the PMR polymorphic allocator `std::pmr::polymorphic_allocator`.

## Performance Testing

Performance testing was added in [unrolled-binary-tree-perf-test.cpp](./app/unrolled-binary-tree-perf-test.cpp).

### Performance Test Setup

There are two different types of test runs:

1. **Bulk insertion**: The test measures the time to insert a specific number of elements into the tree.
2. **"Churn"**: The test includes a mixture of insertions and deletions.

For each test type, it runs multiple times and takes the **minimum** execution time. This eliminates the impact of OS overhead on test results.

### Early Issues from Performance Testing

Early churn numbers were measured on a structurally-broken tree. In situations when a node's array was fully emptied, it would not be properly destroyed. This was verified by using ASan during testing.

Addressing it required a full rewrite of the `remove()` function. See commit [69743ea4](https://github.com/jim-koch-atlanta/cpp-refresh/commit/69743ea435d62ab07671fccd142f36e7422dacad).

### Hypothesis

For the **bulk insertion test**, I assumed:
* PMR polymorphic allocator would be fastest. It's specifically made for extremely fast allocation, so it should have performance advantages.
* `std::allocator` would be the second fastest. It does not have the overhead of the many `new` and `delete` calls that the PoolAllocator experiences once its pool is full.
* PoolAllocator would be a distant third. It suffers from needing to perform many `new` calls once its pool is full.

For the **churn test**, I assumed:
* PMR polymorphic allocator would be fastest. It doesn't bother with **deallocation**, so this should give it a performance advantage.
* PoolAllocator would be second fastest. Because it wouldn't fill up its pool, it could reuse the allocated blocks. This would make alloc and dealloc extremely fast.
* `std::allocator` would be a close third. I assume it has some level of caching built-in, but it would still need to do many allocations / deallocations throughout the churn test.

### Results (N = 10,000)
```
┌─────────────┬────────────────┬──────────┬──────────┬────────┐
│  Workload   │ std::allocator │   Pool   │   pmr    │ Winner │
├─────────────┼────────────────┼──────────┼──────────┼────────┤
│ Bulk-insert │ 0.729 ms       │ 1.360 ms │ 0.599 ms │ pmr    │
├─────────────┼────────────────┼──────────┼──────────┼────────┤
│ Churn       │ 0.379 ms       │ 0.332 ms │ 0.355 ms │ Pool   │
└─────────────┴────────────────┴──────────┴──────────┴────────┘
```

Surprisingly, the PoolAllocator was the fastest for the churn test — so my hypothesis that PMR would win the churn test was wrong. The following is a *hypothesis* (suggested by Claude, not yet validated) for why the pool edged it out:

1. Static vs. runtime dispatch — my pool is a template allocator: direct, inlinable calls. PMR routes every allocate/deallocate through `memory_resource`'s virtual interface — a virtual call per op on the hot path. That's the "static (compile-time) vs. runtime (PMR) polymorphism" tradeoff from the Carpenter allocators talk, showing up in my own numbers.
2. The pool never hits its fallback here (PoolSize=1000 > live set) → pure free-list reuse. PMR's monotonic buffer never frees, so across a run it keeps bumping and occasionally `new`s a fresh upstream chunk.

This would need to be validated, but the idea makes sense.

**Note**: The pool allocator is notably slower in the bulk-insertion test. However, this is a symptom of how it was configured. It was configured with `PoolSize=10`. Essentially, it was able to store 10 of 10,000 elements within its pre-allocated pool, and then the remaining elements needed to be allocated on the heap.

If the pool allocator is defined with `PoolSize=10000`, it is nearly as fast as pmr:
```
===== testIntBulkInsertions =====
number of insertions: 10000
=== std::allocator ===
Fastest of 25 run(s): 0.809198 ms
=== PoolAllocator ===
Fastest of 25 run(s): 0.640929 ms
=== pmr ===
Fastest of 25 run(s): 0.610029 ms
```

This configuration may be acceptable if the tree will **consistently** have ~10,000 elements. Otherwise, it would be wasting memory.

## Conclusion

Performance testing demonstrated that the spreads in execution time were small across the three allocators (~14% on churn, ~18% pmr-vs-std on bulk). This is because the allocator is **not** the bottleneck. Traversal, comparisons, and element shifts within the BST dominate the execution time.

That said:

* `std::allocator` is a safe bet for most use cases.
* In specific scenarios, other allocators may be able to give some performance benefits.
* Use of allocators for performance tuning should always be validated with sufficient performance testing.
