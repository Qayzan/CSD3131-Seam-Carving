// ============================================================================
// check.hpp  --  tiny test helpers for the shared-foundation tests (FF)
//
// Each tests/test_*.cpp is its own program registered with CTest. CHECK works
// in every build type -- unlike assert(), which Release builds (NDEBUG, as in
// CI) silently remove. A test program returns 1 if any check failed.
// ============================================================================
#ifndef SEAM_TEST_CHECK_HPP
#define SEAM_TEST_CHECK_HPP

#include <cmath>
#include <exception>
#include <iostream>
#include <string>

namespace check {

inline int& failures() {
    static int count = 0;
    return count;
}
inline int& passes() {
    static int count = 0;
    return count;
}

inline void record(bool ok, const char* expression, const char* file, int line) {
    if (ok) {
        ++passes();
    } else {
        ++failures();
        std::cout << "  FAIL  " << file << ":" << line << "  " << expression << "\n";
    }
}

// Prints one line per test function and turns an unexpected exception into a failure.
template <typename Fn>
void run(const char* name, Fn fn) {
    const int before = failures();
    try {
        fn();
    } catch (const std::exception& e) {
        ++failures();
        std::cout << "  FAIL  " << name << ": unexpected exception: " << e.what() << "\n";
    }
    std::cout << (failures() == before ? "PASS  " : "FAIL  ") << name << "\n";
}

inline int summary(const char* program) {
    std::cout << "\n" << program << ": " << passes() << " checks passed, " << failures()
              << " failed\n";
    return failures() == 0 ? 0 : 1;
}

}  // namespace check

#define CHECK(expr) ::check::record(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) \
    ::check::record(std::fabs((a) - (b)) <= (tol), #a " ~= " #b, __FILE__, __LINE__)
// Passes only if `statement` throws an exception of type `type`.
#define CHECK_THROWS(type, statement)                                          \
    do {                                                                       \
        bool thrown_ = false;                                                  \
        try {                                                                  \
            statement;                                                         \
        } catch (const type&) {                                                \
            thrown_ = true;                                                    \
        } catch (...) {                                                        \
        }                                                                      \
        ::check::record(thrown_, "throws " #type ": " #statement, __FILE__, __LINE__); \
    } while (0)

#endif  // SEAM_TEST_CHECK_HPP
