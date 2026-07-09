# CppCon 2025 — Back to Basics: Custom Allocators Explained (Kevin Carpenter)

---

## Introduction

As C++ developers, we should rely on the standard library. 20% of the effort for 80% of the output.

### Importance of Memory Management

* Directly affects performance, stability, and efficiency.
* Fine-grained control over memory compared to other compiled languages.
* With great power comes great responsibility.

## Why: Problems with Default Memory Management

### **What's wrong with `new` and `delete`?**

**Problem #1**: The Speed Trap
* The `new` calls `malloc`
* `malloc` tries its own memory
* If that amount of memory is outside the app's memory, it does a context switch
* Kernel will use `brk()/sbrk()` and `mmap`
* Context switch again
* Allocation complete
* Object creation
* Memory with objects

**Problem #2**: Memory Fragmentation
* If it's in L1 cache, great.
* If it's not there, it goes to L2 cache, which is slower.
* If it's not there, it goes to main memory, which is even slower.

### The Opportunity

What if allocation could be nearly instantaneous?

## What: Understanding the std::allocator Model

We've all seen:
```cpp
std::vector<int> my_vec;
```

but it's really this:
```cpp
std::vector<int, std::allocator<int>> my_vec;
```

### How did we get here?

* Originally the std allocator solved the problem of `near` and `far` pointers, back in the days of DOS.
* Now `std::allocator` is C++ standard way of controlling memory that is flexible and extensible.

### Where is std::allocator used?

All standard containers use `std::allocator` **by default** (it's the default template argument).

The allocator contract is not that big:

```cpp
template<typename T>
struct MyAllocator {
    using value_type = T;

    MyAllocator() = default;

    T* allocate(size_t n);

    void deallocate(T* p, size_t n);
};
```

**Is "Allocator" an actual interface or a concept?** *Neither, exactly* -- it's a **named requirement** (the standard calls it *Cpp17Allocator*): a documented, **duck-typed contract**, not a base class you inherit and not a formal `concept` you `requires`. Your type just has to *structurally* provide:
* `value_type`
* `T* allocate(size_t n)` and `void deallocate(T*, size_t n)`
* be copyable and equality-comparable (`==` / `!=`)
* a converting / "rebind" constructor (see the tracking allocator below)

Everything else is filled in by **`std::allocator_traits`** (defaults). And crucially this is **compile-time (static) polymorphism**: the allocator is a **template parameter** of the container (`vector<int, MyAlloc>`), so `vector<int, A>` and `vector<int, B>` are *different types*. Zero runtime overhead, but the allocator type "infects" the container type. (The `pmr` model at the end flips this to runtime.)

When you use a custom allocator, you have the pattern of:
1. Allocate the memory.
2. Construct / destruct the objects **in** that allocated memory.

For example:

```cpp
std::allocator<int> allocator;
int *array = allocator.allocate(5);

for (int i = 0; i < 5; ++i) {
    std::construct_at(&array[i], i * 10); // Initializes array[i] to i * 10
}

for (int i = 0; i < 5; ++i) { std::cout << array[i] << " "; }

for (int i = 0; i < 5; ++i) { std::destroy_at(&array[i]); }

allocator.deallocate(array, 5);
```

**Why `construct_at`, not `array[i] = i * 10`?** Because `allocate(5)` returns **raw, uninitialized memory** -- correctly sized and aligned, but **no `int` objects exist there yet**. The allocator model splits two things that `new` normally fuses:
1. **allocate** = get raw bytes.
2. **construct** = create an object *in* those bytes (run its constructor).

`array[i] = i * 10` is an **assignment**, which assumes there's *already* a live object at `array[i]`. For a trivial `int` that happens to work (no constructor/invariants, so writing to raw memory is harmless). But for a **non-trivial** type -- say `std::string` -- `array[i] = "hi"` would run `string::operator=` on a **non-existent object**, treating garbage bytes as a valid string → UB/crash. `std::construct_at(&array[i], val)` (placement-new under the hood) *builds* the object. Symmetrically, `destroy_at` runs the destructor (needed for non-trivial types to free their resources) before `deallocate`.

So the lifecycle is always **allocate → construct → use → destroy → deallocate.** `new`/`delete` bundle allocate+construct / destroy+deallocate together; a custom allocator forces you to see the seam. (Same reason `emplace_back` constructs in place -- Ch. 12.)

### The Modern Helper: `std::allocator_traits`

Instead of `my_alloc.construct(p, val)`:

```cpp
std::allocator_traits<MyAlloc>::construct(my_alloc, p, val);
```

**What `allocator_traits` is for:** it's an **indirection layer** between containers and allocators that supplies **defaults**, so an allocator can stay *minimal*. Your allocator only *must* provide `value_type`/`allocate`/`deallocate`; `allocator_traits<A>` synthesizes everything else -- `construct`, `destroy`, `max_size`, `rebind`, pointer types, copy/move/swap-propagation rules -- using the allocator's own version *if it defines one*, else a sensible default.

So **containers never call the allocator directly** -- they always go through `std::allocator_traits<A>::allocate(a, n)`, `::construct(a, p, args...)`, etc. That's why the "modern helper" replaces `my_alloc.construct(p, val)` (which your allocator might not even define) with `allocator_traits<MyAlloc>::construct(...)` (which *always* works -- it falls back to `construct_at`/placement-new). In fact C++20 **removed** `construct`/`destroy` from `std::allocator` itself, so going through `allocator_traits` is now the required path.

## How: Popular Custom Allocator Designs

We're only focusing on two: the pool allocator and the stack allocator.

### The Pool Allocator

```cpp
template <typename T, std::size_t PoolSize>
class SimplePoolAllocator {
    struct FreeNode { FreeNode* next; };
    static_assert(sizeof(T)  >= sizeof(FreeNode),  "slot too small for the free-list pointer");
    static_assert(alignof(T) >= alignof(FreeNode), "slot alignment too weak for the free-list pointer");

public:
    SimplePoolAllocator() {
        // thread every slot onto the free list up front
        for (std::size_t i = 0; i < PoolSize; ++i) {
            auto* node = reinterpret_cast<FreeNode*>(&pool_[i * sizeof(T)]);
            node->next = free_head_;
            free_head_ = node;
        }
    }

    // owns a fixed buffer and hands out interior pointers -> non-copyable
    SimplePoolAllocator(const SimplePoolAllocator&)            = delete;
    SimplePoolAllocator& operator=(const SimplePoolAllocator&) = delete;

    T* allocate() {                    // pop the head of the free list
        if (free_head_ == nullptr) throw std::bad_alloc{};
        FreeNode* node = free_head_;
        free_head_ = node->next;
        ++allocated_count_;
        return reinterpret_cast<T*>(node);
    }

    void deallocate(T* p) noexcept {   // push back onto the head
        if (!p) return;
        auto* node = reinterpret_cast<FreeNode*>(p);
        node->next = free_head_;
        free_head_ = node;
        --allocated_count_;
    }

    std::size_t allocated() const noexcept { return allocated_count_; }

private:
    alignas(T) std::byte pool_[PoolSize * sizeof(T)];
    FreeNode*   free_head_       = nullptr;
    std::size_t allocated_count_ = 0;
};
```

> Note on the code above: moving `allocate()` *into* the class is correct (it's a member). But as written it's under `private:` -- `allocate`/`deallocate` must be **public** so the container (via `allocator_traits`) can call them. Keep the *data members* private, put the *operations* public.

Here we're doing a linked list. We can allocate all of the memory **up front**, so we don't need to get it when it's needed. As memory is needed, it's iterating in-order through the memory. On `deallocate()`, we'll push the memory back onto the front of the list.

The neat trick: the free list stores its `next` pointers **inside the free slots themselves** (`reinterpret_cast<T*>(free_head_)`). A free slot isn't holding a `T` right now, so its bytes are reused to hold a `FreeNode*` -- zero extra bookkeeping memory. Allocation = "pop the head of the free list"; deallocation = "push onto the head." Both are O(1) pointer swaps, which is why it's so fast.

**The Verdict**:
* Extremely fast and eliminates memory fragmentation for object types.
* **But** only works for a single fixed size, and reserves potentially a large block of memory up front.

So it would work well for a system that frequently creates and destroys many objects of the same size.

### The Stack Allocator

```cpp
class StackAllocator {
public:
    explicit StackAllocator(std::size_t size)
        : memory_{std::make_unique<std::byte[]>(size)}, total_size_{size} {}
    // Rule of Zero: unique_ptr gives correct move, non-copyable, and auto cleanup.

    void* allocate(std::size_t bytes,
                   std::size_t alignment = alignof(std::max_align_t)) {
        // bump the offset up to the requested (power-of-two) alignment
        std::size_t aligned = (current_offset_ + alignment - 1) & ~(alignment - 1);
        if (aligned + bytes > total_size_) throw std::bad_alloc{};
        void* p = memory_.get() + aligned;
        current_offset_ = aligned + bytes;
        return p;
    }

    void reset() noexcept { current_offset_ = 0; }   // free everything at once

    std::size_t used()     const noexcept { return current_offset_; }
    std::size_t capacity() const noexcept { return total_size_; }

private:
    std::unique_ptr<std::byte[]> memory_;
    std::size_t total_size_;
    std::size_t current_offset_ = 0;
};
```

Yes, it's the constructor. **`explicit`** (from Tour Ch. 6) means the constructor **won't be used for implicit conversions**: you can write `StackAllocator a(35);` or `a{35}`, but **not** `StackAllocator a = 35;`, and a bare `35` won't silently turn into a `StackAllocator` when passed to a function. It's the "use `explicit` for single-argument constructors unless you *want* implicit conversion" rule -- here it stops an integer from accidentally becoming a whole allocator.

(Also: this is a **stateful** allocator -- it owns its buffer and offset. If you adapted it into a *std-conforming* allocator, its `operator==` couldn't just return `true` like a stateless one's -- two stack allocators are only interchangeable if they're literally the *same* object.)

```cpp
StackAllocator sa{1024};
void* a = sa.allocate(100);                  // 100 bytes
void* b = sa.allocate(200, alignof(double)); // aligned for double
sa.reset();                                  // reclaim everything at once
```

**The Verdict**:
* Fastest possible allocation, zero fragmentation, move single pointer to free.
* Cannot deallocate individual objects out of order, difficult for unpredictable lifetimes.

Best for:
* Scratchpad memory, game engines, like allocating memory to handle a single frame, short-lived data.

> Both of the above are standalone **strategy demos** -- they show the *allocation logic*, not the full `std::allocator` interface. To drop one into a std container you'd wrap the strategy behind the Cpp17Allocator contract (`value_type`, `allocate(n)`, rebind ctor, `operator==`) -- which is exactly what the TrackingAllocator below does.

## Practical: Building One

Let's create a tracking allocator, so we can see how allocation actually works!

```cpp
#include <iostream>

template <typename T>
struct TrackingAllocator {
    using value_type = T;

    TrackingAllocator() = default;

    template <typename U>
    constexpr TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

    bool operator==(const TrackingAllocator<T>&) const { return true; }
    bool operator!=(const TrackingAllocator<T>&) const { return false; }

    T* allocate(size_t n) {
        std::cout << "ALLOCATING: " << n << " object(s) of size " << sizeof(T) << " bytes.\n";
        
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t n) {
        std::cout << "DEALLOCATING: " << n << " object(s) of size " << sizeof(T) << " bytes.\n";
        return (::operator delete(p));
    }
};
```

Decoding the "contract" pieces (this is what the *Cpp17Allocator* requirement asks for):
* **`template<typename U> TrackingAllocator(const TrackingAllocator<U>&)`** -- the **rebind / converting constructor**. A container often needs to allocate something *other* than `T`: e.g. `std::list<T>` allocates *nodes*, `std::map` allocates tree-nodes -- not bare `T`s. This lets the library turn a `TrackingAllocator<T>` into a `TrackingAllocator<Node>`. `noexcept` because it does no work.
* **`operator==` / `operator!=`** -- required so the library can ask *"can memory allocated by A be freed by B?"* Two **stateless** allocators of the same kind are interchangeable → always `==`. (A **stateful** one like the stack/pool allocators must compare their actual state -- see the `StackAllocator` note above.)
* **`allocate` uses `::operator new`** (raw bytes), not `new T` -- because allocation must *not* construct. Construction happens later via `construct_at`/`allocator_traits`.

## How this ties into `pmr` (the runtime-polymorphism version)

Everything above is the **classic, compile-time** allocator model: the allocator is a **template parameter**, so `vector<int, PoolAlloc>` and `vector<int, StackAlloc>` are *different, incompatible types*. Fast (static dispatch, zero overhead), but the allocator type infects the container type -- you can't write one function that takes "a vector using any allocator" without templating it.

`std::pmr` fixes that with **runtime polymorphism**:
* `pmr` containers use **one fixed allocator type**, `std::pmr::polymorphic_allocator<T>`, which just holds a **pointer to a `std::pmr::memory_resource`** (an abstract base with virtual `do_allocate` / `do_deallocate` / `do_is_equal`).
* So `std::pmr::vector<int>` *is* `std::vector<int, std::pmr::polymorphic_allocator<int>>` -- and **all** `pmr::vector<int>` are the *same type* no matter which resource they use. You pick the strategy at **runtime** by passing a different `memory_resource*` at construction; allocation dispatches through a **virtual call**.

| | classic allocator (this talk) | `pmr` |
|---|---|---|
| allocator lives in | template parameter `vector<T, A>` | runtime pointer inside `polymorphic_allocator<T>` |
| dispatch | compile-time (static) | virtual (runtime) |
| type identity | allocator is part of the container type | all `pmr::vector<T>` are one type |
| swap strategy | need a different container *type* | pass a different `memory_resource*` |
| cost | zero overhead | small per-alloc virtual-call cost |

**Punchline: the allocator designs in this talk are exactly the standard `pmr` resources** (from the Ch. 12 Allocators notes):
* the **stack / scratchpad** allocator ≈ `std::pmr::monotonic_buffer_resource` (bump a pointer; free everything at once on destruction).
* the **pool** allocator ≈ `std::pmr::synchronized_pool_resource` / `unsynchronized_pool_resource` (fixed-size-block pools).
* the default ≈ `std::pmr::new_delete_resource()`.

So this talk taught me to **hand-build** what `pmr` gives me **off the shelf** -- and `pmr` is how you deploy those same strategies *without* baking the allocator into the container's type.
