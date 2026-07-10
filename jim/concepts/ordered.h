#include <concepts>

namespace jim {
    namespace concepts {

        // This could be done with std::totally_ordered, but I'm practicing.
        template <typename T>
        concept Ordered = requires(T x, T y) {
            x < y;      // Less than
            x > y;      // Greater than
            x <= y;     // Less than or equal
            x >= y;     // Greater than or equal
            x == y;     // Equality
        };
    }
}