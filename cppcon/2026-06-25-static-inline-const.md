# CppCon 2025 — Back to Basics: static, inline, const, constexpr (Andreas Fertig)

---

## The Big Theme: keyword reuse

Adding a brand-new keyword to C++ risks breaking existing code that already uses
that word as an identifier (variable/function name). The committee checks large
code corpora before introducing one. To avoid breakage, they **reuse existing
keywords in new contexts** instead.

That's *why* `static`, `inline`, and `const` each mean several unrelated things
depending on where they appear. The keyword is the same; the meaning is not.

---

## `static`

`static` has multiple, unrelated meanings depending on context:

### 1. Free function with internal linkage
```cpp
static void Fun() { ... }
```
- `static` here means **internal linkage**: the name is visible only inside *this*
  translation unit (TU).
- Each TU that includes the header gets its **own private copy** of the function;
  copies in other TUs are invisible and unrelated.

### 2. Static local ("block") variable
```cpp
void Fun() {
  static int val{};   // one instance, shared across all calls
}
```
- **Constructed once**, on the first call that reaches the declaration.
- **Lives until program exit**; destroyed in reverse order of construction.
- Caveat: because construction order depends on runtime call order, the
  destruction order can be hard to reason about. Use with care.

### 3. Static data member (one shared instance)
```cpp
// header
class Emotions {
    static int mSmileDuration;   // declaration only
};

// .cpp  — out-of-class definition required (pre-C++17)
int Emotions::mSmileDuration = 0;
```
- A static data member is **not** per-object — there is **one shared instance**
  for the whole program. Mentally it's a global variable namespaced under the class.
- Pre-C++17 you **must** provide an out-of-class definition in exactly one `.cpp`.
  Put that definition in a header included by multiple TUs → ODR violation.
- (C++17 fixes this with `inline static` — see below.)

### 4. Static member function
```cpp
class Emotions {
public:
    static void Smile();   // no `this`, no object needed
};
```
- Like a free function namespaced under the class. **No `this` pointer**, so it
  **cannot access non-static members**.
- Called as `Emotions::Smile()` — no instance required.
- Can be defined inline (in-class) or out-of-line.

---

## `inline`

**The key realization:** modern `inline` is *not* about the optimization "inlining."

- **Old meaning (a hint):** "copy-paste the body into the call site." Compilers now
  decide inlining on their own and largely **ignore** the keyword for that purpose.
- **Real meaning today (ODR keyword):** "this entity may be **defined in multiple
  translation units**, and every definition must be **identical**. The linker
  merges them into one."

### One Definition Rule (ODR)
A function/variable may be **defined** only once across the whole program. `inline`
relaxes that: it permits one identical definition per TU, and the linker folds them
into a single entity. That's why you can put an `inline` function/variable in a
header and `#include` it everywhere.

> ⚠️ If the definitions are *not* identical across TUs (you "trick" it), it's
> undefined behavior. The linker silently picks one and uses it everywhere.

### Static local variable inside an inline function — safe
`inline` guarantees there's still exactly **one** instance of the static local
shared across all TUs. No duplication problem.

### `inline static` data member (C++17) — the fix for `static` case #3
```cpp
class Emotions {
    inline static int mSmileDuration{4};   // define + init in the header, done
};
```
- No separate `.cpp` definition needed. One shared instance, ODR-safe even when the
  header is included everywhere. This is the modern replacement for the old
  declare-in-header / define-in-cpp dance.

### Member functions defined in-class are *implicitly* inline
```cpp
class Emotions {
public:
    void Cheer() { puts("Go, go, go!!!"); }   // implicitly inline
};
```
- A function **defined inside the class body** is automatically `inline`. That's why
  headers full of in-class definitions don't blow up ODR. No need to sprinkle the
  `inline` keyword yourself.

---

## `const`

`const` also means different things depending on position. Read pointer
declarations **right-to-left**.

### const on values and pointers
```cpp
const char  jim;             // jim itself is read-only
const char* mike;            // pointer to const char
char* const kathleen;        // const pointer to (mutable) char
const char* const isabella;  // const pointer to const char
```

| Declaration | Can change what it points to? | Can change the pointed-to value? |
|---|---|---|
| `const char* mike`        | ✅ yes | ❌ no |
| `char* const kathleen`    | ❌ no  | ✅ yes |
| `const char* const isabella` | ❌ no | ❌ no |

> 🔧 **Correction from my first draft:** I had `mike` and `kathleen` swapped.
> - `const char* mike` = *pointer to const data*: you **can** repoint `mike`, but
>   **cannot** change the char through it.
> - `char* const kathleen` = *const pointer*: you **cannot** repoint `kathleen`, but
>   you **can** change the char through it.
>
> (`isabella` can in theory be `const_cast`-ed away, but mutating a genuinely
> const object via the cast is UB.)

### Fertig's renaming trick: const has two distinct jobs
He replaces `const` with descriptive pseudo-keywords to show *which* job it's doing:

```cpp
const_by_user_request char jim;                            // = const char jim
read_only_data         char* mike;                         // = const char* mike
char*  const_by_user_request kathleen;                     // = char* const kathleen
read_only_data char* const_by_user_request isabella;       // = const char* const isabella
```

- **`read_only_data`** (low-level const, *left* of the `*`): the pointed-to data is
  read-only. The pointer can still move.
- **`const_by_user_request`** (top-level const, on the object/pointer itself): this
  variable can't be reassigned after init. Says nothing about the data it points to.

That single distinction — **low-level** (read-only data) vs **top-level** (the
variable itself is fixed) — explains the parameter rules below.

### const on function parameters
```cpp
void Fun(char);               // A.
void Fun(const char);         // B.  ← SAME as A (redefinition)
void Fun(const char*);        // C.
void Fun(char* const);        // D.  ← SAME as Fun(char*), NOT same as C
void Fun(const char* const);  // E.  ← SAME as C (redefinition)
```

**The rule: top-level `const` on a by-value parameter is ignored for the
signature.** A by-value parameter is the callee's private *copy*; whether that copy
is const is nobody else's business, so it doesn't change the function's identity.

- **B = A**: the `const` is top-level (on the local `char` copy) → ignored → B is a
  redefinition of A. You cannot overload on it.
- **E = C**: `const char*` is the meaningful part (read-only data); the *extra*
  trailing `const` (on the pointer copy) is top-level → ignored → E redefines C.
- **D ≠ C**: D's `const` is top-level (the pointer copy is fixed) → ignored →
  D behaves like `void Fun(char*)`. That's a *different* signature from C
  (`const char*`), so D and C coexist fine; D just isn't a duplicate of C.

Low-level const (read-only data) *does* participate in the signature; top-level
const does *not*.

---

## `constexpr`

> "The purpose of a `constexpr` function is to produce a constant at compile time."

- **`constexpr` function = *may* run at compile time.** If you call it in a context
  that requires a constant *and* all inputs are constant expressions, it's evaluated
  at compile time. Otherwise it runs at runtime, like a normal function. ("Maybe
  compile-time.")
- **`constexpr` variable** must be initialized by a constant expression; its value is
  known at compile time, and it is implicitly `const`.
- `constexpr` functions are **implicitly `inline`** — don't add `inline` yourself,
  it's redundant. Consequence: the **definition must be visible** wherever it's used
  (you can't split declaration and definition across a `.cpp`), because the compiler
  needs the body to evaluate it.

---

## `consteval`

- **C++20. Applies only to functions. "Immediate function."**
- **`consteval` function = *must* run at compile time, always.** Every call must
  produce a constant expression; if it can't be evaluated at compile time, it's a
  **compile error**.

> The clarification you were missing: **`consteval` is the certain one,
> `constexpr` is the "maybe" one.**
> - `constexpr` → compile-time *if possible*, else runtime.
> - `consteval` → compile-time *or it doesn't compile*. Never runs at runtime.

---

## `if constexpr`

```cpp
template <typename T>
void f(T t) {
    if constexpr (std::is_pointer_v<T>) {
        use(*t);     // compiled only when T is a pointer
    } else {
        use(t);      // compiled only otherwise
    }
}
```
- Compile-time branch selection. The condition must be a constant expression.
- **The branch not taken is discarded** — not just skipped at runtime, but *not
  compiled* for that instantiation. The dead branch doesn't even have to be valid
  for that `T`. This is what replaces a lot of old SFINAE machinery.

---

## `std::is_constant_evaluated()`

```cpp
constexpr void Log(std::string_view msg) {
    if (not std::is_constant_evaluated()) { puts(msg.data()); }
}
```
- Returns `true` when the current evaluation is happening in a **compile-time
  context**, `false` at runtime. Lets one `constexpr` function behave differently in
  each (e.g. skip I/O at compile time, where `puts` isn't allowed).

> ⚠️ **Never put it in an `if constexpr`:**
> `if constexpr (std::is_constant_evaluated())` is **always true**. Evaluating the
> condition of an `if constexpr` is itself a constant evaluation, so the function
> reports "compile-time" unconditionally and you always take that branch. Use a
> **plain `if`**. (C++23 adds `if consteval` to do this cleanly and avoid the trap.)

---

## `constinit`

- **C++20. Applies to variables with static/thread storage duration.**
- Guarantees the variable is **initialized at compile time** (constant
  initialization) — no runtime/dynamic initialization. This kills the **static
  initialization order fiasco** for that variable.
- **It does NOT make the variable const.** The value can still be mutated at runtime.
  It only constrains *how the initialization happens*.

> Easy to confuse with `constexpr` variables:
> - `constexpr` variable → compile-time init **+ const** (immutable afterwards).
> - `constinit` variable → compile-time init **only** (still mutable afterwards).
