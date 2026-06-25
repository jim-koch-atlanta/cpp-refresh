# CppCon 2025 — Back to Basics: Move Semantics (Ben Saks)

---
 
## The Four Special Member Functions (Rule of Five)
 
| | Copy | Move |
|---|---|---|
| Constructor | `T(const T& other)` | `T(T&& other)` |
| Assignment | `T& operator=(const T& other)` | `T& operator=(T&& other)` |
 
Plus the destructor. If you define any one of the five, define all five — or define none (Rule of Zero).
 
---
 
## `const T&` Can Bind to Temporaries
 
```cpp
const double &rd = 1;  // legal — compiler creates a temporary, extends its lifetime
```
 
- Requires `const` — you can't mutate a temporary through a reference
- Lifetime of the temporary matches the lifetime of the reference
- Main use case: `const T&` function parameters that accept both lvalues and literals/temporaries without overloads
---
 
## Rvalue References (`&&`)
 
```cpp
int &&ri = 10;   // binds to a temporary; ri is mutable
```
 
- `&&` does NOT mean "two references" — it means *rvalue reference*
- Can only bind to rvalues (temporaries, literals, things about to expire)
- Gives you a **mutable** handle to a temporary — unlike `const&`
- Foundation of move semantics: `&&` signals "I know this is expiring, I can steal from it"
---
 
## `std::move()` Is Just a Cast
 
```cpp
my_string s2 {std::move(s1)};  // casts s1 to my_string&&, invoking move constructor
```
 
- `std::move()` doesn't move anything — it casts to `&&`
- The actual move happens in the constructor/operator that receives the `&&`
- After a move, the source is in a **valid but unspecified state** (not destroyed)
---
 
## The Wrong Way to Write a Move Constructor (Ben's Key Example)
 
```cpp
// BAD — lvalue ref parameter
my_string(my_string &other) {
    data = other.data;
    other.data = nullptr;   // silently guts the source!
}
```
 
```cpp
my_string s2 {s1};   // looks like a copy, silently destroys s1
```
 
The danger: move-like behavior triggered by innocent-looking copy syntax, with no compiler warning.
 
**Fix:** use `&&` — forces the caller to write `std::move(s1)`, making the gutting explicit and visible in the code.
 
---
 
## Automatic Generation Rules
 
The compiler auto-generates all five if you define none. If you define **any one**, generation of the others may be suppressed — especially move operations. Custom destructor = you're managing a resource = default copy/move is probably wrong anyway.