# CppCon 2025 — Back to Basics: Refactoring (Amir Kirsh)

---

Historically, the code was written by juniors, and the seniors would refactor the code. In this age of AI, we now need everyone to have the ability to refactor.

## Code Review

Consider:

```cpp
if ( !players[i] || !players[i].isAlive ) {
    // ...
}
```

* It could probably use ranges, instead of index.
* The negation is confusing, and generally prefer `&&` over `||`.
  * The technique here is **De Morgan's law**: `!a || !b` ≡ `!(a && b)`. So `!players[i] || !players[i].isAlive` becomes `!(players[i] && players[i].isAlive)` -- "NOT (player exists AND is alive)". Pulling the single `!` to the outside is what makes it readable.
* We should probably hide the data member `isAlive`.
* If we are regularly checking those two conditions together, extract a **positively-named** predicate: `player.isActive()` = "exists AND alive", then the call site is just `if (!player.isActive())`.
  * (My note said `isNotActiveOrDead()` -- but that name bakes the negation *into* the name, which is exactly the confusion we're trying to remove. Prefer a positive predicate and negate once at the call site.)

## Refactoring

"Refactoring is a disciplined technique for restructuring an existing body of code, altering its internal structure without changing its external behavior." -- Martin Fowler

**How can we tell** that we didn't change its external behavior? **Regression testing.**

## Refactoring - Why, When and How

"Refactoring is not another word for cleaning the code. It's aimed for improving the health of a code-base." -- Martin Fowler

### Why

1. **Support for new features and future code changes**. Adapt code structure to better handle the new requirements and upcoming code changes.
2. **Improve readability and maintainability**. Make the code easier to understand, modify, and extend by others and future self.
3. **Performance improvements**.
4. **Make the code less bug-prone and prevent future bugs**. Identify areas where code invites bugs.

Ultimately, **reduce technical debt**.

### When

1. Refactor on change, not on whim. Don't polish stable code.
2. Refactor based on actual benefits. Don't refactor based on subjective preferences.

## Back to our code

Refactoring might be motivated by updating to the latest C++ version. If we were in C++23:

```cpp
for (auto&& [index, player] :
    players | std::views::enumerate
    | std::views::filter([](auto&& index_player) {
        auto&& [_, the_player] = index_player;
        return the_player.isAlive();
    }))
{

    ActionRequest action = player->getAction();
    if (action == ActionRequest::GetInfo) {
        handleInfoRequest(index);
    }
        // ...
}
```

And if that loop were common, maybe we would template it:

```cpp
template <typename Action>
void for_each_live_player(Action action) {
    for (auto&& [index, player] :
        players | std::views::enumerate
        | std::views::filter([](auto&& index_player) {
            auto&& [_, the_player] = index_player;
            return the_player.isActive();
        }))
    {
        action(index, player);
    }
}
```

So now the complexity is hidden.

(Note: the slides drift between `isAlive()` and `isActive()` across these two snippets, and `the_player` is used with `.` in the filter but `player->` in the loop body -- the container is really holding pointer-like elements. Don't read too much into those inconsistencies; the *point* is the extraction, not the exact predicate name.)

But maybe even simpler, we could follow the **command pattern** -- turn each `ActionRequest` into a command object rather than a growing `if`/`switch` on the action type (this is the "replace conditional with polymorphism" smell fix from the catalog below).

## Catalog of Code Smells and Refactoring Options

**Why a catalog?**
* Provide a shared language for discussing code quality.
* Connects each smell to established refactoring options.
* Makes problem detection systematic and repeatable.
* Provides actionable remedies, instead of reinventing the wheel.
* Supports education, onboarding, and code reviews.

## Common Code Smells

He described the appropriate refactoring for these common code smells:

1. Long Methods
2. Long Parameter Lists
3. Large Class
4. Comments
  * Extract sub-methods. Well-named methods can reduce the need for comments.
  * Consider replacing with assertions.
5. Switch statements and complex conditions
  * Replace conditional with polymorphism
  * Replace type code with state/strategy (state pattern)
  * Introduce null object

You don't need to memorize the code smells. Use AI to identify the code smells.

## Summary

1. **Continuous refactoring** is required to improve code readability, maintainability, and reliability.
2. **Do not refactor code you are not otherwise touching.** (i.e. only refactor code you're already changing for another reason -- matches the "Refactor on change, not on whim / don't polish stable code" rule above.)
3. **Refactoring becomes even more important in the AI era.** AI doesn't necessarily improve code quality.
4. **Refactor for a clear reason**.
  * Code is hard to understand and maintain.
  * You identify the code smells that need to be fixed.
5. **Refactor by small, incremental and testable changes.**
6. Be cautious using AI to refactor. As of September 2025, code quality after an AI refactor was no better than before the refactor.
