1. [DONE] Write the actual BST & "node" implementation.
2. [DONE] Make sure it works with built-in types (int, double, etc).
3. [DONE] Once I have that, make it work with defined types. This will force me to figure out concepts for ordering.
  * Define my own concept, rather than using a built-in concept type. This is for my own learning.
4. [DONE] Make sure it works with some defined type (a struct, or a class).
5. [DONE] In-order iterator.
6. Implement my own **allocator**.
  * Do a ring buffer... that could be interesting and fun.
7. Update my BST to use the ring buffer allocator.
8. Performance testing with ring buffer allocator vs with default allocator. Number of allocs / deallocs. Maybe some sort of actual performance metrics (Wall clock? CPU clock?)
  * Benchmark harness -- `<chrono> steady_clock` for wall time, and reuse TrackingAllocator to count allocs/deallocs.
  * Compare my allocator vs. `std::pmr::monotonic_buffer_resource`.
9. Rule of Five. Define deep-copy and move for the whole tree.
  * (Why are we making the move noexcept???)
10. operator<=> / operator==
11. operator<< as a hidden friend
  * [DONE] Also make the member variables of tree and node private.
12. Deduction guide + `initializer_list` ctor
  * Be able to do `UnrolledBinaryTree<int, 10>{1, 2, 3, 4, 5};`
  * Then "build from any range" edges into `ranges::to` territory.
13. Pluggable comparators, predicates, invocables.
14. `find()` returning std::optional and invariant checks via `assert`