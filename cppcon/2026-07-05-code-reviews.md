# CppCon 2025 — Back to Basics: Code Reviews (Chandranath Bhattacharyya & Kathleen Baker)

---

Many of the recommendations come from:

* C++ Core Guidelines
* Chromium C++ style guide
* Chromium C++ Do's and Don'ts

## Automate the mechanical stuff (the underlying point)

They touched on this initially, but it's the real lesson tying the whole catalog together: **almost everything below is *mechanical*** -- the exact same comment a reviewer types over and over. Don't repeat mechanical feedback by hand; **push it into tooling** so it's caught *before* a human ever looks, and reserve human review for what tools **can't** check (design, correctness, naming, tests, "does this actually solve the problem?").

The stack:
* **clang-format** -- formatting / whitespace / brace style. Kills every "nit: spacing" comment. Run on save + in CI.
* **clang-tidy** -- literally the checks scattered through these notes:
  * `performance-unnecessary-value-param` -- pass read-only by `const&`
  * `modernize-use-emplace`, `modernize-loop-convert` (range-for), `modernize-use-nullptr`, `modernize-use-override`, ...
  * `bugprone-*` -- common mistakes
* **Compiler warnings** -- `-Wswitch` (missing enum cases), `-Wpessimizing-move` (bad `std::move`), plus `-Wall -Wextra`. Add `-Werror` so they can't be ignored.
* **CI / pre-commit hooks** -- run all of the above on every change automatically, so violations never reach a reviewer.

Payoff: the reviewer's attention isn't burned on `const&`-vs-value or `std::move` placement (the linter already flagged those) -- it goes to the stuff that actually needs a brain. **A rule you keep repeating in reviews is a rule that should be a lint check.**

## Scope

With C++17, we can restrict the scope of a variable used in an if/else block:

```cpp
if (const SomeClass s = Foo(); s.HasPropOne()) {
    ...
}
```

This improves readability and allows the variable to be used elsewhere in the function.

## Enums

1. Don't ever use `enum`. Use `enum class`.
2. Don't ever use `default` in a `switch` for an `enum class`. The `-Wswitch` flag can be used to check that there are no missed enum values in a switch.

Also, the Edge codebase includes a `NOTREACHED()` macro. If it's ever hit, it'll provide appropriate logging to catch this situation.

## Iteration

Don't use `for` loops with an index to iterate over containers. Instead, use range-based for loops.

### Use `const auto&` or `auto&&` in range-based loops

This will prevent cloning objects in a `for` loop:
```
for (const auto& val : vec) {
    ...
}

// or

for (auto&& val : vec) {
    ...
}
```

**When to use each:**
* **`const auto&`** -- read-only, no copy. Default when you're only *reading* elements.
* **`auto&`** -- when you need to *modify* elements in place; but it only binds to real lvalue references.
* **`auto&&`** -- a **forwarding reference**: binds to *anything* (lvalue/rvalue, const/non-const). Use it when you need to modify, **or** when the range yields **proxy/temporary** elements -- e.g. `std::vector<bool>` (yields a proxy, not `bool&`) or range views like `views::transform` (yield prvalues). `auto&` fails to compile on those; `auto&&` always binds and never copies.

Rule of thumb: **`const auto&` to read, `auto&&` to be safe/generic (especially over views), `auto&` only for lvalue-only mutable access.**

## Passing parameters

```cpp
void Print(std::string s) {
    std::cout << s << '\n';
}
```

Pass non-trivial read-only objects as const references to prevent unnecessary copies.

The clang-tidy check `--checks=performance-unnecessary-value-param` will flag this.

### Don't use `const &` for structs

`void PrintPoint(const Point pt)` vs `void PrintPoint(Point pt)` is the same.

**Why:** this is the **top-level-`const`-is-ignored** rule from Tour Ch. 6. A by-value parameter is a *copy* the function owns; whether that copy is `const` is the callee's private business, invisible to the caller. So the two are the **same signature** -- you can't even overload on the difference. The `const` only affects whether the body may reassign `pt`; it changes nothing at the interface, so don't write it in the declaration.

### Don't use `const &` for simple types

Passing trivial objects by value is preferred, because passing by reference can prevent optimizations.

### Setting a member variable

**For "will-move-from" parameters, pass by `X&&` and `std::move` the parameter.**

```cpp
class Widget {
    std::string name_;
public:
    // "sink" the argument: caller passes an rvalue, we steal it into the member
    void set_name(std::string&& name) {
        name_ = std::move(name);   // move the rvalue in, no copy
    }
};

Widget w;
w.set_name("hello");          // temporary is an rvalue → binds to std::string&&
std::string n = "hi";
w.set_name(std::move(n));     // must std::move an lvalue to call it
```

The inner `std::move` is required: `name` is an rvalue *reference*, but as a *named* variable it's an lvalue, so without `std::move` you'd copy, not move.

(Alternative idiom -- **pass-by-value-then-move** -- handles both lvalues and rvalues with one overload: `void set_name(std::string name) { name_ = std::move(name); }`. The talk preferred the `&&` form; both are common.)

### string_view over `const std::string &`

`std::string_view` can handle `std::string`, `const char*`, and `{const char*, len}` as arguments. It does not do any heap allocation.

### `std::span`

**Why fewer allocations:** `std::span<int>` is a non-owning **view** -- just a `{pointer, length}` pair, like `string_view` but for any contiguous sequence. It **allocates nothing**. If a function instead takes `const std::vector<int>&` and you only have a `std::array` or C array, you'd have to **build a `vector` from it first** (heap allocation + copy) just to make the call. With `span`, you pass the array/vector/etc. directly -- zero allocation.

**Why multiple types:** a `span<int>` can view a `std::vector<int>`, `std::array<int,N>`, a C array `int[N]`, or any contiguous range of `int`. A `const std::vector<int>&` parameter accepts **only** a `std::vector<int>`. So `span` decouples the function from the *specific container* -- it's the container-level analog of `string_view`. (Same non-owning **dangling** caveat: the span must not outlive the data it points into.)

## Use std::optional

If a function returns a value on success, but doesn't return a value on failure, use `std::optional`.

## Use `std::expected`

`std::expected<T, E>` (**C++23**) holds **either** a value of type `T` (success) **or** an error of type `E` (failure). It's like `std::optional`, but the failure case carries *why* it failed instead of just "nothing":
* `std::optional<T>` = value **or** nothing.
* `std::expected<T, E>` = value **or** an error value.

Use it for functions that can fail, when you want to return result-or-error **without exceptions**. Check `.has_value()`; read `.value()` or `.error()`. (This is the value-or-error return type from the Ch. 4 error-handling discussion.)

## NRVO

`-Wpessimizing-move` (a compiler / clang-tidy warning) tells us when we're using `std::move` inefficiently -- e.g. `std::move` on a return value that would otherwise be elided.

**Copy elision vs. NRVO -- related, not the same.** Copy elision is the *umbrella*: the compiler omitting a copy/move constructor call. It has two named forms:
* **RVO** (Return Value Optimization) -- eliding the copy when returning a **temporary/prvalue**: `return Foo();`. **Mandatory** since C++17.
* **NRVO** (Named RVO) -- eliding the copy when returning a **named local**: `Foo f; ...; return f;`. **Allowed but NOT guaranteed**, even in C++17.

So NRVO is *one specific kind* of copy elision (the named-variable case), and it's the optional one. (This is why `return std::move(f);` is a pessimization -- it turns the named local into an expression the compiler can't elide, forcing a move where NRVO would've been free. Same lesson as the Tour Ch. 6 notes.)

## Don't return const value from function

A `const` return value can defeat `std::move` operations, including NRVO. Returning `const Foo` (by value) means the result can't be moved from (a move needs a *non-const* rvalue), so you get a **copy** instead of a move, and elision is inhibited. → **Return by plain value `Foo`, not `const Foo`.**

**"Is there ever a time we *should* return a `const` value?" -- basically never (for class types).** The rule is nearly absolute:
* **Why it ever existed:** pre-C++11, Scott Meyers recommended `const` returns from operators to catch typos -- with a non-const return, `if (a * b = c)` (meant `==`) silently *assigns* to the temporary; a `const` return makes it a **compile error**. Move semantics (C++11) killed this trade-off: `const` blocks moves, and that cost now outweighs the typo-safety.
* **Modern replacement for that safety:** ref-qualify the assignment operator so you can't assign to a temporary -- *without* the move penalty:
  ```cpp
  Rational& operator=(const Rational&) & = default;  // trailing & = lvalues only
  // → (a * b) = c;  is now a compile error, and moves still work
  ```
* **Harmless-but-pointless case:** for **scalars / trivially-copyable** types the top-level `const` is *ignored* -- `const int f()` ≡ `int f()`. No effect, so don't write it.
* **Not covered by this rule:** `const T&` (reference return -- fine, avoids a copy) and `const T*` (pointer to const -- fine). The rule is only about `const T` **by value** for class types.
* Tooling: clang-tidy `readability-const-return-type` flags it (Core Guidelines F.20).

## Use the member initialization list

If you use the member initialization list, it'll catch ordering problems. Without it, it'll compile when it shouldn't.

## Make member functions static as possible

If a member function doesn't use any member variables, it can probably be made `static`.

## Don't declare functions as `extern`

All function declarations have external linkage by default. A namespace-scope function already has external linkage unless you make it `static` or put it in an anonymous namespace (→ internal linkage). So `extern` on a *function* is redundant noise. (`extern` *is* meaningful for **variables** -- `extern int x;` declares a variable defined elsewhere. Functions just don't need it. Ties to the linkage discussion in the static/inline/const talk.)

## Make member functions const as possible

If a member function isn't modifying any members, make it `const`.

## Returning non-trival member object

Return as `const &` if possible.

**No conflict with "don't return const value" -- the difference is reference vs value:**
* "Don't return `const` **value**" = don't write `const Foo f()` (by value) -- it defeats move/NRVO.
* "Return non-trivial member as `const &`" = a **getter** returning `const Foo&` -- a *reference* to the existing member, so **no copy at all**, read-only.

`const Foo` (value) forces a copy; `const Foo&` (reference) avoids one. Different things. (Caveat: only safe when the object outlives the reference -- don't return a `const&` into a temporary; dangling.)

## Ensure appropriate special member functions

Essentially, follow the Rule Of 5.

## `emplace_back` vs `push_back`

`emplace_back(args...)` constructs the element **in place** from constructor arguments -- no temporary, no move. `push_back(X(args...))` builds a temporary `X` and then moves it in. So `emplace_back` can save a move.

Caveats (not a blanket "always emplace"): for **trivial types** there's no move to save anyway, and `emplace_back` will silently perform `explicit`/narrowing conversions that `push_back` would reject -- so `push_back` is sometimes the safer, clearer choice. Rough guide: `emplace_back` when constructing *from arguments*; `push_back` when you already have the object.

## STL Algorithms

Modernize call sites toward ranges / dedicated helpers:

`std::sort(v.begin(), v.end())` → `std::ranges::sort(v)`

**Erase-remove idiom → `std::erase_if`:**
```cpp
// old two-step erase-remove idiom
// (note: v.end() is the 2nd argument to erase, NOT an argument to remove_if):
v.erase(std::remove_if(v.begin(), v.end(),
                       [](int x){ return x % 2 != 0; }),
        v.end());

// C++20 one-liner:
std::erase_if(v, [](int x){ return x % 2 != 0; });
```

**Membership test on a set** -- `s.contains(x)` (C++20) instead of the `find != end` dance:
```cpp
std::find(s.begin(), s.end(), x) != s.end()   // generic algorithm
std::ranges::find(s, x) != s.end()             // ranges form  (note: ranges::find, not "range()::find")
s.contains(x)                                  // C++20 -- cleanest for associative containers
```

## Use `std::variant` to unify multiple type alternatives

Replace `union` with `std::variant`.

## `std::monostate`

`std::monostate` is an empty placeholder type you put as the **first alternative** of a `std::variant` to make the variant **default-constructible**. If a variant's first alternative isn't default-constructible, the variant isn't either:

```cpp
struct A { A(int); };                     // not default-constructible
std::variant<A, int> v1;                  // ❌ error: can't default-construct (first alt is A)
std::variant<std::monostate, A, int> v2;  // ✅ defaults to the empty monostate state
```

So `monostate` = "the variant is currently holding *nothing meaningful*" -- an explicit empty/null state. (Ties to the `variant` guidance above: it's the idiom for giving a variant a valid "empty" default.)

