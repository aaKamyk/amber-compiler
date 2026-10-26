// clang-format off
/**
 * @mainpage cpp_result
 * @brief Modern Rust-like Result<T, E> for C++17.
 *
 * cpp_result is a header-only, ergonomic Result type for C++17 inspired by
 * Rust, with combinators, helpers, and void specialization.
 *
 * - See the API reference for details.
 * - Quickstart:
 *   @code
 *   #include <result.hpp>
 *
 *   struct Error {
 *      std::string message;
 *   };
 *
 *   using Result = cpp_result::Result<int, Error>;
 *
 *   Result divide(int a, int b) {
 *     if (b == 0)
 *       return Result::Err({"Division by zero"});
 *     return Result::Ok(a / b);
 *   }
 *
 *   int main() {
 *     auto r = divide(10, 2);
 *     if (r.is_ok())
 *       std::cout << r.unwrap() << '\n';
 *     else
 *       std::cout << r.unwrap_err().message << '\n';
 *   }
 *   @endcode
 *
 * @section features Feature Control
 *
 * cpp_result provides fine-grained feature toggling via preprocessor macros.
 * This allows you to reduce binary size or limit API surface by disabling
 * unused features at compile time.
 *
 * - The main switch is `CPP_RESULT_FEATURE_ALL` (default: enabled).
 *   If set to 1, all features are enabled unless individually overridden.
 *   If set to 0, all features are disabled unless individually enabled.
 *
 * - Individual features:
 *   - `CPP_RESULT_FEATURE_UNWRAP`   : Unwrap/expect helpers (unwrap,
 * unwrap_err, expect, etc.)
 *   - `CPP_RESULT_FEATURE_MAP`      : Map/map_err/map_or/map_or_else
 *   - `CPP_RESULT_FEATURE_ANDOR`    : and_, and_then, or_, or_else
 *   - `CPP_RESULT_FEATURE_INSPECT`  : inspect, inspect_err
 *   - `CPP_RESULT_FEATURE_CONTAINS` : contains, contains_err
 *   - `CPP_RESULT_FEATURE_FLATTEN`  : flatten
 *   - `CPP_RESULT_FEATURE_OPTIONAL` : ok(), err() as std::optional
 *
 * You can set these macros via your build system or before including
 * result.hpp:
 * @code
 * #define CPP_RESULT_FEATURE_ALL 0
 * #define CPP_RESULT_FEATURE_UNWRAP 1
 * #include <result.hpp>
 * @endcode
 *
 * If you use CMake or Meson, options are provided to control these features.
 *
 * @see README.md for more details.
 *
 * @section macros Macros: TRY and TRY_ASSIGN
 *
 * cpp_result provides two macros to simplify error propagation in functions
 * returning Result types:
 *
 * - `TRY(expr)`: Evaluates `expr` (which must return a Result). If it is an
 * error, returns the error from the current function. Otherwise, yields the
 * value.
 * - `TRY_ASSIGN(name, expr)`: Like TRY, but binds the unwrapped value to the variable
 * `name`.
 *
 * These macros mimic Rust's `?` operator for early returns on error.
 *
 * Example usage:
 * @code
 * using cpp_result::Result;
 * using cpp_result::Ok;
 *
 * Result<int, std::string> parse_and_add(const std::string& a, const std::string& b) {
 *   int x = TRY(parse_int(a));
 *   int y = TRY(parse_int(b));
 *   return Ok<int, std::string>(x + y);
 * }
 *
 * // With TRY_ASSIGN:
 * Result<int, std::string> parse_and_add(const std::string& a, const std::string& b) {
 *   TRY_ASSIGN(x, parse_int(a));
 *   TRY_ASSIGN(y, parse_int(b));
 *   return Ok<int, std::string>(x + y);
 * }
 * @endcode
 *
 * @note These macros require the function to return a compatible Result type.
 * 
 * @section limitations Known Limitations
 * 
 * - The `TRY` macro requires your compiler to support statement expressions.
 * - `T` and `E` must not be reference types.
 * - `T` and `E` must not be same types. 
 */
// result.hpp - Rust-like Result<T, E> for C++17
// SPDX-License-Identifier: MIT
//
// This header provides a modern, ergonomic Result type for C++17 inspired by
// Rust. It supports combinators, helpers, and void specialization.
//
// All API is in the cpp_result namespace.
//
// --- API OVERVIEW ---
//
// Construction:
//   Result<T, E>::Ok(T)
//   Result<T, E>::Err(E)
//   Result<void, E>::Ok()
//   Result<void, E>::Err(E)
//   cpp_result::Ok<T, E>(T)
//   cpp_result::Err<T, E>(E)
//   cpp_result::Ok<E>()
//   cpp_result::Err<E>(E)
//
// Query:
//   is_ok(), is_err()
//
// Unwrap:
//   unwrap(), unwrap_err(), unwrap_or(value), unwrap_or_else(fn)
//   expect(msg), expect_err(msg)
//
// Combinators:
//   map(fn), map_err(fn), and_then(fn)
//   inspect(fn), inspect_err(fn)
//
// See the documentation below each function for usage examples.
// clang-format on

#pragma once

#ifndef CPP_RESULT_HAS_STATEMENT_EXPR
#if defined(__GNUC__) || defined(__clang__)
#define CPP_RESULT_HAS_STATEMENT_EXPR 1
#else
#define CPP_RESULT_HAS_STATEMENT_EXPR 0
#endif
#endif

#ifndef CPP_RESULT_FEATURE_ALL
#define CPP_RESULT_FEATURE_ALL 1
#endif
#ifndef CPP_RESULT_FEATURE_UNWRAP
#define CPP_RESULT_FEATURE_UNWRAP CPP_RESULT_FEATURE_ALL
#endif
#ifndef CPP_RESULT_FEATURE_MAP
#define CPP_RESULT_FEATURE_MAP CPP_RESULT_FEATURE_ALL
#endif
#ifndef CPP_RESULT_FEATURE_ANDOR
#define CPP_RESULT_FEATURE_ANDOR CPP_RESULT_FEATURE_ALL
#endif
#ifndef CPP_RESULT_FEATURE_INSPECT
#define CPP_RESULT_FEATURE_INSPECT CPP_RESULT_FEATURE_ALL
#endif
#ifndef CPP_RESULT_FEATURE_CONTAINS
#define CPP_RESULT_FEATURE_CONTAINS CPP_RESULT_FEATURE_ALL
#endif
#ifndef CPP_RESULT_FEATURE_FLATTEN
#define CPP_RESULT_FEATURE_FLATTEN CPP_RESULT_FEATURE_ALL
#endif
#ifndef CPP_RESULT_FEATURE_OPTIONAL
#define CPP_RESULT_FEATURE_OPTIONAL CPP_RESULT_FEATURE_ALL
#endif

// clang-format off
/**
 * @def TRY(expr)
 * @brief Propagate errors like Rust's `?` operator.
 *
 * Evaluates `expr` (which must return a Result). If it is an error, returns the
 * error from the current function. Otherwise, yields the value.
 *
 * @note This implementation needs your compiler support statement expressions.
 * @see Source code for the macro definition.
 *
 * Example:
 * @code
 * Result<int, std::string> parse_and_add(const std::string& a, const std::string& b) {
 *   int x = TRY(parse_int(a));
 *   int y = TRY(parse_int(b));
 *   return Ok<int, std::string>(x + y);
 * }
 * @endcode
 */
// clang-format on
#if CPP_RESULT_HAS_STATEMENT_EXPR
#define TRY(expr)                                                       \
    ({                                                                  \
        auto &&__res = (expr);                                          \
        using __res_type = std::decay_t<decltype(__res)>;               \
        if (__res.is_err()) return __res_type::Err(__res.unwrap_err()); \
        std::move(__res.unwrap());                                      \
    })
#endif

// clang-format off
/**
 * @def TRY_ASSIGN(name, expr)
 * @brief Propagate errors and bind the value to a variable.
 *
 * Evaluates `expr` (which must return a Result). If it is an error, returns the
 * error from the current function. Otherwise, binds the unwrapped value to the
 * variable `name`.
 *
 * Example:
 * @code
 * Result<int, std::string> parse_and_add(const std::string& a, const std::string& b) {
 *   TRY_ASSIGN(x, parse_int(a));
 *   TRY_ASSIGN(y, parse_int(b));
 *   return Ok<int, std::string>(x + y);
 * }
 * @endcode
 */
// clang-format on
#define TRY_ASSIGN(name, expr)                                                           \
    auto &&__res_##name = (expr);                                                        \
    using __res_type_##name = std::decay_t<decltype(__res_##name)>;                      \
    if (__res_##name.is_err()) return __res_type_##name::Err(__res_##name.unwrap_err()); \
    auto &name = __res_##name.unwrap()

#include <cstdio>
#include <cstdlib>
#if CPP_RESULT_FEATURE_OPTIONAL
#include <optional>
#endif  // CPP_RESULT_FEATURE_OPTIONAL
#include <type_traits>
#include <utility>

#define EXPECT_OR_ABORT(cond, msg)    \
    do {                              \
        if (!(cond)) {                \
            std::fputs(msg, stderr);  \
            std::fputc('\n', stderr); \
            std::abort();             \
        }                             \
    } while (0)

namespace cpp_result {

/**
 * @brief Result<T, E> - Holds either a value (Ok) or an error (Err).
 *
 * @tparam T Value type
 * @tparam E Error type
 *
 * Example:
 * @code
 * auto r = Ok<int, std::string>(42);
 * if (r.is_ok())
 *  std::cout << r.unwrap();
 * @endcode
 */
template <typename T, typename E>
class [[nodiscard]] Result {
    static_assert(
        !std::is_reference_v<T> && !std::is_reference_v<E>,
        "Result<T,E> does not support reference types"
    );

public:
    /**
     * @brief Construct an Ok result.
     * @param val Value to store
     * @return Ok result
     * @code
     * auto r = cpp_result::Ok<int, std::string>(42);
     * // or
     * using Result = cpp_result::Result<int, std::string>;
     * auto r2 = Result::Ok(42);
     * @endcode
     */
    static inline Result Ok(T val) noexcept { return Result(std::move(val)); }

    /**
     * @brief Construct an Err result.
     * @param err Error to store
     * @return Err result
     * @code
     * using Result = cpp_result::Result<int, std::string>;
     * auto r = Result::Err("fail");
     * // or
     * auto r2 = cpp_result::Err<int, std::string>("fail");
     * @endcode
     */
    static inline Result Err(E err) noexcept { return Result(std::move(err)); }

    /**
     * @brief Returns true if the result is Ok.
     * @code
     * auto r = Ok<int, std::string>(1);
     * if (r.is_ok()) {  ...  }
     * @endcode
     */
    constexpr bool is_ok() const noexcept { return is_ok_; }

    /**
     * @brief Returns true if the result is Err.
     * @code
     * auto r = Err<int, std::string>("fail");
     * if (r.is_err()) {  ...  }
     * @endcode
     */
    constexpr bool is_err() const noexcept { return !is_ok_; }

#if CPP_RESULT_FEATURE_UNWRAP
    /**
     * @brief Unwraps the value. Aborts if Err.
     * @return T&
     * @code
     * auto r = Ok<int, std::string>(42);
     * assert(r.unwrap() == 42);
     * @endcode
     */
    inline T &unwrap() noexcept {
        EXPECT_OR_ABORT(is_ok_, "unwrap called on Result::Err()");
        return data_.value;
    }
    inline const T &unwrap() const noexcept {
        EXPECT_OR_ABORT(is_ok_, "unwrap called on Result::Err()");
        return data_.value;
    }

    /**
     * @brief Unwraps the error. Aborts if Ok.
     * @return E&
     * @code
     * auto r = Err<int, std::string>("fail");
     * assert(r.unwrap_err() == "fail");
     * @endcode
     */
    inline E &unwrap_err() noexcept {
        EXPECT_OR_ABORT(!is_ok_, "unwrap_err called on Result::Ok()");
        return data_.error;
    }
    inline const E &unwrap_err() const noexcept {
        EXPECT_OR_ABORT(!is_ok_, "unwrap_err called on Result::Ok()");
        return data_.error;
    }

    /**
     * @brief Returns value if Ok, else returns default_value.
     * @code
     * auto r = Err<int, std::string>("fail");
     * int v = r.unwrap_or(123);
     * assert(v == 123);
     * @endcode
     */
    inline T unwrap_or(T default_value) const noexcept {
        return is_ok_ ? data_.value : default_value;
    }

    /**
     * @brief Returns value if Ok, else calls func().
     * @code
     * auto r = Err<int, std::string>("fail");
     * int v = r.unwrap_or_else([]{ return 123; });
     * assert(v == 123);
     * @endcode
     */
    template <typename F>
    inline T unwrap_or_else(F &&func) const noexcept(noexcept(func())) {
        return is_ok_ ? data_.value : std::forward<F>(func)();
    }

    /**
     * @brief Returns the value if Ok, else returns a default-constructed value.
     *        Requires T to be default-constructible.
     * @code
     * auto r1 = Ok<int, std::string>(42);
     * assert(r1.unwrap_or_default() == 42);
     * auto r2 = Err<int, std::string>("fail");
     * assert(r2.unwrap_or_default() == 0); // int default
     * @endcode
     */
    T unwrap_or_default() const noexcept(std::is_nothrow_default_constructible_v<T>) {
        if (is_ok_) return data_.value;
        return T{};
    }

    /**
     * @brief Unwraps the value or aborts with a custom message if Err.
     * @code
     * auto r = Ok<int, std::string>(42);
     * assert(r.expect("should not fail") == 42);
     * @endcode
     */
    T &expect(const char *msg) noexcept {
        EXPECT_OR_ABORT(is_ok_, msg);
        return data_.value;
    }
    const T &expect(const char *msg) const noexcept {
        EXPECT_OR_ABORT(is_ok_, msg);
        return data_.value;
    }

    /**
     * @brief Unwraps the error or aborts with a custom message if Ok.
     * @code
     * auto r = Err<int, std::string>("fail");
     * assert(r.expect_err("should not fail") == "fail");
     * @endcode
     */
    E &expect_err(const char *msg) noexcept {
        EXPECT_OR_ABORT(!is_ok_, msg);
        return data_.error;
    }
    const E &expect_err(const char *msg) const noexcept {
        EXPECT_OR_ABORT(!is_ok_, msg);
        return data_.error;
    }

    /**
     * @brief Returns true if the result is Ok and the predicate returns true for
     * the value.
     * @code
     * auto r = Ok<int, std::string>(42);
     * bool b = r.is_ok_and([](int v){ return v > 0; });
     * assert(b);
     * @endcode
     */
    template <typename Pred>
    bool is_ok_and(Pred &&pred) const noexcept(noexcept(pred(std::declval<T>()))) {
        return is_ok_ && pred(data_.value);
    }

    /**
     * @brief Returns true if the result is Err and the predicate returns true for
     * the error.
     * @code
     * auto r = Err<int std::string>("fail");
     * bool b = r.is_err_and([](const std::string& e){ return e == "fail"; });
     * assert(b);
     * @endcode
     */
    template <typename Pred>
    bool is_err_and(Pred &&pred) const noexcept(noexcept(pred(std::declval<E>()))) {
        return !is_ok_ && pred(data_.error);
    }
#endif  // CPP_RESULT_FEATURE_UNWRAP

#if CPP_RESULT_FEATURE_MAP
    /**
     * @brief Maps the value if Ok, else propagates Err.
     * @code
     * auto r = Ok<int, std::string>(21);
     * auto mapped = r.map([](int v){ return v*2; });
     * assert(mapped.unwrap() == 42);
     * @endcode
     */
    template <typename F, typename U = std::invoke_result_t<F, T>>
    Result<U, E> map(F &&func) const noexcept(noexcept(func(std::declval<T>()))) {
        if (is_ok_) return Result<U, E>::Ok(func(data_.value));
        return Result<U, E>::Err(data_.error);
    }

    /**
     * @brief Maps the error if Err, else propagates Ok.
     * @code
     * auto r = Err<int, std::string>("fail");
     * auto mapped = r.map_err([](const std::string& e){ return e+"!"; });
     * assert(mapped.unwrap_err() == "fail!");
     * @endcode
     */
    template <typename F, typename E2 = std::invoke_result_t<F, E>>
    Result<T, E2> map_err(F &&func) const noexcept(noexcept(func(std::declval<E>()))) {
        if (is_ok_) return Result<T, E2>::Ok(data_.value);
        return Result<T, E2>::Err(func(data_.error));
    }
    /**
     * @brief Applies a function to the value if Ok, else returns default_value.
     * @code
     * auto r = Ok<int, std::string>(21);
     * int v = r.map_or(0, [](int v){ return v*2; });
     * assert(v == 42);
     * @endcode
     */
    template <typename U, typename F>
    U map_or(U default_value, F &&func) const {
        return is_ok_ ? func(data_.value) : default_value;
    }

    /**
     * @brief Applies a function to the value if Ok, else computes a default with
     * another function.
     * @code
     * auto r = Err<int, std::string>("fail");
     * int v = r.map_or_else([]{ return 0; }, [](int v){ return v*2; });
     * assert(v == 0);
     * @endcode
     */
    template <typename D, typename F>
    auto map_or_else(D &&default_fn, F &&func) const {
        return is_ok_ ? func(data_.value) : default_fn();
    }
#endif  // CPP_RESULT_FEATURE_MAP

#if CPP_RESULT_FEATURE_ANDOR
    /**
     * @brief Chains another result-producing function if Ok, else propagates Err.
     * @code
     * auto r = Ok<int, std::string>(1);
     * auto chained = r.and_then([](int v){
     *    return Ok<int, std::string>(v+1);
     * });
     * assert(chained.unwrap() == 2);
     * @endcode
     */
    template <typename F, typename R = typename std::invoke_result_t<F, T>>
    R and_then(F &&func) const noexcept(noexcept(func(std::declval<T>()))) {
        if (is_ok_) return func(data_.value);
        return R::Err(data_.error);
    }
    /**
     * @brief Returns res if the result is Ok, otherwise returns self.
     * @code
     * auto r1 = Ok<int, std::string>(1);
     * auto r2 = Ok<int, std::string>(2);
     * auto out = r1.and_(r2);
     * assert(out.unwrap() == 2);
     * @endcode
     */
    template <typename R2>
    auto and_(R2 &&res) const {
        if (is_ok_) return std::forward<R2>(res);
        return Result<T, E>::Err(data_.error);
    }
    /**
     * @brief Returns res if the result is Err, otherwise returns self.
     * @code
     * auto r1 = Err<int, std::string>("fail");
     * auto r2 = Ok<int, std::string>(2);
     * auto out = r1.or_(r2);
     * assert(out.unwrap() == 2);
     * @endcode
     */
    template <typename R2>
    auto or_(R2 &&res) const {
        if (!is_ok_) return std::forward<R2>(res);
        return *this;
    }
    /**
     * @brief Calls op if the result is Err, otherwise returns self.
     * @code
     * auto r = Err<int, std::string>("fail");
     * auto out = r.or_else([]{ return Ok<int, std::string>(42); });
     * assert(out.unwrap() == 42);
     * @endcode
     */
    template <typename F>
    auto or_else(F &&op) const {
        if (!is_ok_) return op();
        return *this;
    }
#endif  // CPP_RESULT_FEATURE_ANDOR

#if CPP_RESULT_FEATURE_INSPECT
    /**
     * @brief Calls func(value) if Ok, returns self.
     * @code
     * auto r = Ok<int, std::string>(42);
     * r.inspect([](int v){ std::cout << v; });
     * @endcode
     */
    template <typename F>
    const Result &inspect(F &&func) const noexcept(
        noexcept(func(std::declval<const T &>()))
    ) {
        if (is_ok_) func(data_.value);
        return *this;
    }

    /**
     * @brief Calls func(error) if Err, returns self.
     * @code
     * auto r = Err<int, std::string>("fail");
     * r.inspect_err([](const std::string& e){ std::cout << e; });
     * @endcode
     */
    template <typename F>
    const Result &inspect_err(F &&func) const noexcept(
        noexcept(func(std::declval<const E &>()))
    ) {
        if (!is_ok_) func(data_.error);
        return *this;
    }
#endif  // CPP_RESULT_FEATURE_INSPECT

#if CPP_RESULT_FEATURE_CONTAINS
    /**
     * @brief Returns true if the result is Ok and contains the given value.
     * @code
     * auto r = Ok<int, std::string>(42);
     * assert(r.contains(42));
     * @endcode
     */
    bool contains(const T &value) const { return is_ok_ && data_.value == value; }

    /**
     * @brief Returns true if the result is Err and contains the given error.
     * @code
     * auto r = Err<int, std::string>("fail");
     * assert(r.contains_err("fail"));
     * @endcode
     */
    bool contains_err(const E &error) const { return !is_ok_ && data_.error == error; }
#endif  // CPP_RESULT_FEATURE_CONTAINS

#if CPP_RESULT_FEATURE_FLATTEN
    /**
     * @brief Flattens a Result<Result<U, E>, E> into Result<U, E>.
     * @code
     * auto inner = Ok<int, std::string>(42);
     * auto outer = Ok<Result<int, std::string>, std::string>(inner);
     * assert(outer.flatten().unwrap() == 42);
     * @endcode
     */
    auto flatten() const {
        using Inner = decltype(data_.value);
        if (is_ok_) return data_.value;
        return Inner::Err(data_.error);
    }
#endif  // CPP_RESULT_FEATURE_FLATTEN

#if CPP_RESULT_FEATURE_OPTIONAL
    /**
     * @brief Returns the value as std::optional if Ok, otherwise std::nullopt.
     * @code
     * Result<int, std::string> r = Result<int, std::string>::Ok(42);
     * std::optional<int> opt = r.ok();
     * assert(opt.has_value() && *opt == 42);
     * @endcode
     */
    std::optional<T> ok() const {
        return is_ok_ ? std::optional<T>(data_.value) : std::nullopt;
    }

    /**
     * @brief Returns the error as std::optional if Err, otherwise std::nullopt.
     * @code
     * Result<int, std::string> r = Result<int, std::string>::Err("fail");
     * std::optional<std::string> opt = r.err();
     * assert(opt.has_value() && *opt == "fail");
     * @endcode
     */
    std::optional<E> err() const {
        return !is_ok_ ? std::optional<E>(data_.error) : std::nullopt;
    }
#endif

    ~Result() { destroy(); }

    // Move semantics
    Result(Result &&other) noexcept(
        std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<E>
    )
        : is_ok_(other.is_ok_) {
        if (is_ok_)
            new (&data_.value) T(std::move(other.data_.value));
        else
            new (&data_.error) E(std::move(other.data_.error));
    }

    Result &operator=(Result &&other) noexcept(
        std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_assignable_v<E>
    ) {
        if (this != &other) {
            destroy();
            is_ok_ = other.is_ok_;
            if (is_ok_)
                new (&data_.value) T(std::move(other.data_.value));
            else
                new (&data_.error) E(std::move(other.data_.error));
        }
        return *this;
    }

    // Conditional copy semantics
    Result(const Result &other) noexcept(
        std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_constructible_v<E>
    )
        : is_ok_(other.is_ok_) {
        if (is_ok_)
            new (&data_.value) T(other.data_.value);
        else
            new (&data_.error) E(other.data_.error);
    }

    Result &operator=(const Result &other) noexcept(
        std::is_nothrow_copy_assignable_v<T> && std::is_nothrow_copy_assignable_v<E>
    ) {
        if (this != &other) {
            destroy();
            is_ok_ = other.is_ok_;
            if (is_ok_)
                new (&data_.value) T(other.data_.value);
            else
                new (&data_.error) E(other.data_.error);
        }
        return *this;
    }

private:
    union Data {
        T value;
        E error;
        Data() {}
        ~Data() {}
    } data_;
    bool is_ok_;

    explicit Result(T val) noexcept
        : is_ok_(true) {
        new (&data_.value) T(std::move(val));
    }

    explicit Result(E err) noexcept
        : is_ok_(false) {
        new (&data_.error) E(std::move(err));
    }

    void destroy() noexcept { is_ok_ ? data_.value.~T() : data_.error.~E(); }
};

/**
 * @brief Result<void, E> - Specialization for operations that return no
 * value, only success or error.
 *
 * @tparam E Error type
 *
 * Example:
 * @code
 * auto r = Ok<std::string>();
 * if (r.is_ok())
 *   std::cout << "All good!";
 * else
 *   std::cout << r.unwrap_err();
 * @endcode
 */
template <typename E>
class [[nodiscard]] Result<void, E> {
    static_assert(
        !std::is_reference_v<E>, "Result<void,E> does not support reference types"
    );

public:
    /**
     * @brief Construct an Ok result.
     * @return Ok result
     * @code
     * auto r = Ok<std::string>();
     * @endcode
     */
    static inline Result Ok() noexcept { return Result(); }

    /**
     * @brief Construct an Err result.
     * @param err Error to store
     * @return Err result
     * @code
     * auto r = Err<std::string>("fail");
     * @endcode
     */
    static inline Result Err(E err) noexcept { return Result(std::move(err)); }

    /**
     * @brief Returns true if the result is Ok.
     * @code
     * auto r = Ok<std::string>();
     * if (r.is_ok()) {  ...  }
     * @endcode
     */
    inline bool is_ok() const noexcept { return is_ok_; }

    /**
     * @brief Returns true if the result is Err.
     * @code
     * auto r = Err<std::string>("fail");
     * if (r.is_err()) {  ...  }
     * @endcode
     */
    inline bool is_err() const noexcept { return !is_ok_; }

    /**
     * @brief Unwraps the value. Aborts if Err.
     * @code
     * auto r = Ok<std::string>();
     * r.unwrap(); // does nothing if Ok
     * @endcode
     */
    inline void unwrap() noexcept {
        EXPECT_OR_ABORT(is_ok_, "unwrap called on Result::Err()");
    }

    /**
     * @brief Unwraps the error. Aborts if Ok.
     * @return E&
     * @code
     * auto r = Err<std::string>("fail");
     * assert(r.unwrap_err() == "fail");
     * @endcode
     */
    inline E &unwrap_err() noexcept {
        EXPECT_OR_ABORT(!is_ok_, "unwrap_err called on Result::Ok()");
        return error_;
    }
    inline const E &unwrap_err() const noexcept {
        EXPECT_OR_ABORT(!is_ok_, "unwrap_err called on Result::Ok()");
        return error_;
    }

    /**
     * @brief Returns unit type if Ok, else returns error.
     * @code
     * auto r = Ok<std::string>();
     * auto mapped = r.map([]{ return 42; });
     * assert(mapped.unwrap() == 42);
     * @endcode
     */
    template <typename F, typename U = std::invoke_result_t<F>>
    Result<U, E> map(F &&func) const noexcept(noexcept(func())) {
        if (is_ok_) return Result<U, E>::Ok(func());
        return Result<U, E>::Err(error_);
    }

    /**
     * @brief Maps the error if Err, else propagates Ok.
     * @code
     * auto r = Err<std::string>("fail");
     * auto mapped = r.map_err([](const std::string& e){ return e+"!"; });
     * assert(mapped.unwrap_err() == "fail!");
     * @endcode
     */
    template <typename F, typename E2 = std::invoke_result_t<F, E>>
    Result<void, E2> map_err(F &&func) const noexcept(noexcept(func(std::declval<E>()))) {
        if (is_ok_) return Result<void, E2>::Ok();
        return Result<void, E2>::Err(func(error_));
    }

    /**
     * @brief Chains another result-producing function if Ok, else propagates
     * Err.
     * @code
     * auto r = Ok<std::string>();
     * auto chained = r.and_then([]{
     *    return Ok<int, std::string>(42);
     * });
     * assert(chained.unwrap() == 42);
     * @endcode
     */
    template <typename F, typename R = std::invoke_result_t<F>>
    R and_then(F &&func) const noexcept(noexcept(func())) {
        if (is_ok_) return func();
        return R::Err(error_);
    }

    /**
     * @brief Unwraps the value or aborts with a custom message if Err.
     * @code
     * auto r = Ok<st::string>();
     * r.expect("should not fail");
     * @endcode
     */
    void expect(const char msg[]) const noexcept { EXPECT_OR_ABORT(is_ok_, msg); }

    /**
     * @brief Unwraps the error or aborts with a custom message if Ok.
     * @code
     * auto r = Err<std::string>("fail");
     * assert(r.expect_err("should not fail") == "fail");
     * @endcode
     */
    E &expect_err(const char msg[]) noexcept {
        EXPECT_OR_ABORT(!is_ok_, msg);
        return error_;
    }

    const E &expect_err(const char msg[]) const noexcept {
        EXPECT_OR_ABORT(!is_ok_, msg);
        return error_;
    }

    /**
     * @brief Calls func() if Ok, returns self.
     * @code
     * auto r = Ok<std::string>();
     * r.inspect([]{ std::cout << "All good!"; });
     * @endcode
     */
    template <typename F>
    const Result &inspect(F &&func) const noexcept(noexcept(func())) {
        if (is_ok_) func();
        return *this;
    }

    /**
     * @brief Calls func(error) if Err, returns self.
     * @code
     * auto r = Err<std::string>("fail");
     * r.inspect_err([](const std::string& e){ std::cout << e; });
     * @endcode
     */
    template <typename F>
    const Result &inspect_err(F &&func) const noexcept(
        noexcept(func(std::declval<const E &>()))
    ) {
        if (!is_ok_) func(error_);
        return *this;
    }

    /**
     * @brief Returns true if the result is Err and the predicate returns true
     * for the error.
     * @code
     * auto r = Err<std::string>("fail");
     * bool b = r.is_err_and([](const std::string& e){ return e == "fail"; });
     * assert(b);
     * @endcode
     */
    template <typename Pred>
    bool is_err_and(Pred &&pred) const noexcept(noexcept(pred(std::declval<E>()))) {
        return !is_ok_ && pred(error_);
    }

    /**
     * @brief Converts the Result into a std::optional<E> (Err value or
     * std::nullopt).
     * @code
     * auto r = Err<std::string>("fail");
     * std::optional<std::string> opt = r.err();
     * assert(opt.has_value());
     * assert(opt.value() == "fail");
     * @endcode
     */
    std::optional<E> err() const {
        if (!is_ok_) return error_;
        return std::nullopt;
    }

    /**
     * @brief Returns res if the result is Ok, otherwise returns self.
     * @code
     * auto ok = Ok<std::string>();
     * auto err = Err<std::string>("fail");
     * auto out = ok.and_(err);
     * assert(out.is_err());
     * @endcode
     */
    template <typename R2>
    auto and_(R2 &&res) const {
        if (is_ok_) return std::forward<R2>(res);
        return Result<void, E>::Err(error_);
    }

    /**
     * @brief Returns res if the result is Err, otherwise returns self.
     * @code
     * auto ok = Ok<std::string>();
     * auto err = Err<std::string>("fail");
     * auto out = err.or_(ok);
     * assert(out.is_ok());
     * @endcode
     */
    template <typename R2>
    auto or_(R2 &&res) const {
        if (!is_ok_) return std::forward<R2>(res);
        return *this;
    }

    /**
     * @brief Applies a function if Ok, else returns default_value.
     * @code
     * auto ok = Ok<std::string>();
     * int v = ok.map_or(0, []{ return 42; });
     * assert(v == 42);
     * @endcode
     */
    template <typename U, typename F>
    U map_or(U default_value, F &&func) const {
        return is_ok_ ? func() : default_value;
    }

    /**
     * @brief Applies a function if Ok, else computes a default with another
     * function.
     * @code
     * auto err = Err<std::string>("fail");
     * int v = err.map_or_else([]{ return 0; }, []{ return 42; });
     * assert(v == 0);
     * @endcode
     */
    template <typename D, typename F>
    auto map_or_else(D &&default_fn, F &&func) const {
        return is_ok_ ? func() : default_fn();
    }

    /**
     * @brief Returns true if the result is Err and contains the given error.
     * @code
     * auto err = Err<std::string>("fail");
     * assert(err.contains_err("fail"));
     * @endcode
     */
    bool contains_err(const E &error) const { return !is_ok_ && error_ == error; }

    // Move semantics
    Result(Result &&other) noexcept(std::is_nothrow_move_constructible_v<E>)
        : is_ok_(other.is_ok_) {
        if (!is_ok_) new (&error_) E(std::move(other.error_));
    }

    Result &operator=(Result &&other) noexcept(std::is_nothrow_move_assignable_v<E>) {
        if (this != &other) {
            is_ok_ = other.is_ok_;
            if (!is_ok_) new (&error_) E(std::move(other.error_));
        }
        return *this;
    }

    // Conditional copy semantics
    Result(const Result &other) noexcept(std::is_nothrow_copy_constructible_v<E>)
        : is_ok_(other.is_ok_) {
        if (!is_ok_) new (&error_) E(other.error_);
    }

    Result &operator=(
        const Result &other
    ) noexcept(std::is_nothrow_copy_assignable_v<E>) {
        if (this != &other) {
            is_ok_ = other.is_ok_;
            if (!is_ok_) new (&error_) E(other.error_);
        }
        return *this;
    }

private:
    E error_;
    bool is_ok_;

    Result() noexcept
        : is_ok_(true) {}
    explicit Result(E err) noexcept
        : is_ok_(false) {
        new (&error_) E(std::move(err));
    }
};

template <typename T, typename E>
inline Result<T, E> Ok(T val) {
    return Result<T, E>::Ok(std::move(val));
}
template <typename T, typename E>
inline Result<T, E> Err(E err) {
    return Result<T, E>::Err(std::move(err));
}
template <typename E>
inline Result<void, E> Ok() {
    return Result<void, E>::Ok();
}
template <typename E>
inline Result<void, E> Err(E err) {
    return Result<void, E>::Err(std::move(err));
}

}  // namespace cpp_result