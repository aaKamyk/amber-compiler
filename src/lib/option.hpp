#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace lib::option {

struct NoneType {};
struct UnitType {};
inline constexpr NoneType None{};
inline constexpr UnitType Unit{};

inline constexpr auto BAD_ACCESS_MESSAGE = "called 'Option::unwrap()' on a 'None' value";

struct BadOptionAccess : std::runtime_error {
    explicit BadOptionAccess(const std::string& msg)
        : runtime_error(msg) {}
    explicit BadOptionAccess(const char* msg)
        : runtime_error(msg) {}
};
template <typename>
class Option;

template <typename TOption, typename TExpected>
concept OptionType = requires(TOption option) {
    { option.take() } -> std::same_as<TExpected>;
    { option.get() } -> std::same_as<TExpected&>;
    { option.has_some() } -> std::same_as<bool>;
};

namespace detail {

template <typename>
struct is_option : std::false_type {};

template <typename T>
struct is_option<Option<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_option_v = is_option<T>::value;

template <typename T, typename Derived>
class OptionImpl {
public:
    [[nodiscard]] constexpr T expect(const char* message) &&
        requires OptionType<Derived, T>
    {
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { throw BadOptionAccess{message}; }

        return self.take();
    }

    [[nodiscard]] constexpr T unwrap() &&
        requires OptionType<Derived, T>
    {
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { throw BadOptionAccess{BAD_ACCESS_MESSAGE}; }

        return self.take();
    }

    [[nodiscard]] constexpr T unwrap_or(const T& value) &&
        requires OptionType<Derived, T>
    {
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return value; }

        return self.take();
    }

    [[nodiscard]] constexpr T unwrap_or(T&& value) &&
        requires OptionType<Derived, T>
    {
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return std::move(value); }

        return self.take();
    }

    [[nodiscard]] constexpr T unwrap_or_default()
        requires OptionType<Derived, T> && std::is_default_constructible_v<T>
    {
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return T{}; }

        return self.take();
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor> &&
                 std::same_as<std::invoke_result_t<TFunctor>, T>
                 [[nodiscard]] constexpr T unwrap_or_else(TFunctor&& functor)
                     requires OptionType<Derived, T>
    {
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return std::invoke(std::forward<TFunctor>(functor)); }

        return self.take();
    }

    [[nodiscard]] constexpr T unwrap_unchecked() &&
        requires OptionType<Derived, T>
    {
        auto& self = *static_cast<Derived*>(this);
        return self.take();
    }

    constexpr T flatten() const&
        requires is_option_v<T>
    {
        auto& self = *static_cast<const Derived*>(this);
        if (!self.has_some()) { return T{None}; }

        return self.get();
    }

    constexpr T flatten() &&
        requires is_option_v<T>
    {
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return T{None}; }

        return self.take();
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor, T&>
    [[nodiscard]] constexpr auto map(TFunctor&& functor) &
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, T&>;
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return Option<TResult>{None}; }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.get())};
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor, const T&>
    [[nodiscard]] constexpr auto map(TFunctor&& functor) const&
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, const T&>;
        auto& self = *static_cast<const Derived*>(this);
        if (!self.has_some()) { return Option<TResult>{None}; }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.get())};
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor, T>
    [[nodiscard]] constexpr auto map(TFunctor&& functor) &&
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, T>;
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return Option<TResult>{None}; }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.take())};
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor, T&>
    [[nodiscard]] constexpr auto map_or(
        TFunctor&& functor, const std::invoke_result_t<TFunctor, T&>& value
    ) &
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, T&>;
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return Option<TResult>{value}; }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.get())};
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor, T&>
    [[nodiscard]] constexpr auto map_or(
        TFunctor&& functor, std::invoke_result_t<TFunctor, T&>&& value
    ) &
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, T&>;
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return Option<TResult>{std::move(value)}; }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.get())};
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor, const T&>
    [[nodiscard]] constexpr auto map_or(
        TFunctor&& functor, const std::invoke_result_t<TFunctor, const T&>& value
    ) const&
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, const T&>;
        auto& self = *static_cast<const Derived*>(this);
        if (!self.has_some()) { return Option<TResult>{value}; }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.get())};
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor, const T&>
    [[nodiscard]] constexpr auto map_or(
        TFunctor&& functor, std::invoke_result_t<TFunctor, const T&>&& value
    ) const&
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, const T&>;
        auto& self = *static_cast<const Derived*>(this);
        if (!self.has_some()) { return Option<TResult>{std::move(value)}; }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.get())};
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor, T>
    [[nodiscard]] constexpr auto map_or(
        TFunctor&& functor, const std::invoke_result_t<TFunctor, T>& value
    ) &&
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, T>;
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return Option<TResult>{value}; }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.take())};
    }

    template <typename TFunctor>
        requires std::invocable<TFunctor, T>
    [[nodiscard]] constexpr auto map_or(
        TFunctor&& functor, std::invoke_result_t<TFunctor, T>&& value
    ) &&
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, T>;
        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) { return Option<TResult>{std::move(value)}; }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.take())};
    }

    template <typename TFunctor, typename TFallbackFunctor>
        requires std::invocable<TFunctor, T&>
    [[nodiscard]] constexpr auto map_or_else(
        TFunctor&& functor, TFallbackFunctor&& fallback
    ) &
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, T&>;
        static_assert(
            std::same_as<std::invoke_result_t<TFallbackFunctor>, TResult>,
            "Fallback type must exactly match with return type of mapping functor"
        );

        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) {
            return Option<TResult>{std::invoke(std::forward<TFallbackFunctor>(fallback))};
        }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.get())};
    }

    template <typename TFunctor, typename TFallbackFunctor>
        requires std::invocable<TFunctor, const T&>
    [[nodiscard]] constexpr auto map_or_else(
        TFunctor&& functor, TFallbackFunctor&& fallback
    ) const&
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, const T&>;
        static_assert(
            std::same_as<std::invoke_result_t<TFallbackFunctor>, TResult>,
            "Fallback type must exactly match with return type of mapping functor"
        );

        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) {
            return Option<TResult>{std::invoke(std::forward<TFallbackFunctor>(fallback))};
        }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.get())};
    }

    template <typename TFunctor, typename TFallbackFunctor>
        requires std::invocable<TFunctor, T>
    [[nodiscard]] constexpr auto map_or_else(
        TFunctor&& functor, TFallbackFunctor&& fallback
    ) &&
        requires OptionType<Derived, T>
    {
        using TResult = std::invoke_result_t<TFunctor, T>;
        static_assert(
            std::same_as<std::invoke_result_t<TFallbackFunctor>, TResult>,
            "Fallback type must exactly match with return type of mapping functor"
        );

        auto& self = *static_cast<Derived*>(this);
        if (!self.has_some()) {
            return Option<TResult>{std::invoke(std::forward<TFallbackFunctor>(functor))};
        }

        return Option<TResult>{std::invoke(std::forward<TFunctor>(functor), self.take())};
    }
};

template <typename T, typename Derived>
class OptionPointerImpl : public OptionImpl<T*, Derived> {
public:
    // Default and none constructors
    OptionPointerImpl() = default;
    constexpr OptionPointerImpl(NoneType)
        : OptionPointerImpl(nullptr) {}
    constexpr explicit OptionPointerImpl(T* value)
        : m_value(value) {}

    [[nodiscard]] constexpr bool has_some() const { return this->m_value != nullptr; }

    constexpr T* take() {
        T* val = this->m_value;
        this->m_value = nullptr;
        return val;
    }

    [[nodiscard]] constexpr T* get() & { return this->m_value; }

    [[nodiscard]] constexpr const T* get() const& { return this->m_value; }

    constexpr void reset() { this->m_value = nullptr; }

protected:
    T* m_value{nullptr};
};

template <typename>
class OptionEmptyImpl {
public:
    [[nodiscard]] constexpr bool has_some() const noexcept { return this->m_has_some; }

private:
    bool m_has_some{false};
};

}  // namespace detail

template <typename T>
class Option : public detail::OptionImpl<T, Option<T>> {
    static_assert(
        std::is_object_v<T> && !std::is_array_v<T>,
        "Option<T> requires T to be a non-array object type."
    );

public:
    // Default & None constructors
    constexpr Option() noexcept
        : m_has_some(false) {};
    constexpr Option(NoneType) noexcept
        : m_has_some(false) {};

    // Copy constructor
    Option(const Option&)
        requires std::is_trivially_copy_constructible_v<T>
    = default;

    constexpr Option(
        const Option& other
    ) noexcept(std::is_nothrow_copy_constructible_v<T>)
        requires std::is_copy_constructible_v<T>
        : m_has_some(other.m_has_some) {
        if (other.has_some()) {
            std::construct_at(std::addressof(this->m_value), other.get());
        }
    }

    // Move constructor
    Option(Option&&)
        requires std::is_trivially_move_constructible_v<T>
    = default;

    constexpr Option(Option&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires std::is_move_constructible_v<T>
        : m_has_some(other.m_has_some) {
        if (other.has_some()) {
            std::construct_at(std::addressof(this->m_value), other.take());
        }
    }

    // Value copy constructor
    constexpr explicit Option(
        const T& value
    ) noexcept(std::is_nothrow_copy_constructible_v<T>)
        requires std::is_copy_constructible_v<T>
        : m_has_some(true) {
        std::construct_at(std::addressof(this->m_value), value);
    }

    // Value move constructor
    constexpr explicit Option(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires std::is_move_constructible_v<T>
        : m_has_some(true) {
        std::construct_at(std::addressof(this->m_value), std::move(value));
    }

    // Value inplace constructor
    template <typename... Args>
        requires(
            std::is_constructible_v<T, Args...> &&
            !(sizeof...(Args) == 1 &&
              (detail::is_option_v<std::remove_cvref_t<Args>> || ...))
        )
    explicit constexpr Option(
        Args&&... args
    ) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : m_has_some(true) {
        std::construct_at(std::addressof(this->m_value), std::forward<Args>(args)...);
    }

    // Destructor
    ~Option()
        requires std::is_trivially_destructible_v<T>
    = default;

    constexpr ~Option() noexcept(std::is_nothrow_destructible_v<T>) { this->reset(); }

    // Copy assignment operator
    Option& operator=(const Option&)
        requires std::is_trivially_copy_assignable_v<T> &&
                     std::is_trivially_copy_constructible_v<T> &&
                     std::is_trivially_destructible_v<T>
    = default;

    constexpr Option& operator=(const Option& other) noexcept(
        std::is_nothrow_copy_assignable_v<T> && std::is_nothrow_copy_constructible_v<T>
    ) {
        if (std::addressof(other) == this) [[unlikely]] { return *this; }

        if (this->has_some() && other.has_some()) {
            this->m_value = other.get();
        } else if (this->has_some() && !other.has_some()) {
            this->reset();
        } else if (!this->has_some() && other.has_some()) {
            std::construct_at(std::addressof(this->m_value), other.get());
            this->m_has_some = true;
        }
        return *this;
    }

    // Move assignment operator
    Option& operator=(Option&&)
        requires std::is_trivially_move_assignable_v<T> &&
                     std::is_trivially_move_constructible_v<T> &&
                     std::is_trivially_destructible_v<T>
    = default;

    constexpr Option& operator=(Option&& other) noexcept(
        std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>
    ) {
        if (std::addressof(other) == this) [[unlikely]] { return *this; }

        if (this->has_some() && other.has_some()) {
            this->m_value = other.take();
        } else if (this->has_some() && !other.has_some()) {
            this->reset();
        } else if (!this->has_some() && other.has_some()) {
            std::construct_at(std::addressof(this->m_value), other.take());
            this->m_has_some = true;
        }
        return *this;
    }

    [[nodiscard]] constexpr bool has_some() const noexcept { return this->m_has_some; }

    [[nodiscard]] constexpr T take() noexcept(std::is_nothrow_destructible_v<T>) {
        if (!this->has_some()) { throw BadOptionAccess{BAD_ACCESS_MESSAGE}; }
        return this->take_unchecked();
    }

    [[nodiscard]] constexpr T& get() & {
        if (!this->has_some()) { throw BadOptionAccess{BAD_ACCESS_MESSAGE}; }
        return this->get_unchecked();
    }

    [[nodiscard]] constexpr const T& get() const& {
        if (!this->has_some()) { throw BadOptionAccess{BAD_ACCESS_MESSAGE}; }
        return this->get_unchecked();
    }

    [[nodiscard]] constexpr T& get_unchecked() & noexcept { return this->m_value; }

    [[nodiscard]] constexpr const T& get_unchecked() const& noexcept {
        return this->m_value;
    }

    [[nodiscard]] constexpr T take_unchecked() noexcept(
        std::is_nothrow_destructible_v<T>
    ) {
        T val = std::move(this->m_value);
        this->reset();
        return val;
    }

    constexpr void reset() noexcept(std::is_nothrow_destructible_v<T>) {
        if (!this->has_some()) { return; }
        std::destroy_at(std::addressof(m_value));
        this->m_has_some = false;
    }

private:
    union {
        T m_value;
    };
    bool m_has_some;
};

template <typename T>
class Option<T&> : public detail::OptionPointerImpl<T, Option<T&>> {
public:
    using detail::OptionPointerImpl<T, Option<T&>>::OptionPointerImpl;

    constexpr Option()
        : detail::OptionPointerImpl<T, Option<T&>>() {}
    constexpr Option(NoneType)
        : detail::OptionPointerImpl<T, Option<T&>>() {}
    constexpr explicit Option(T& value)
        : detail::OptionPointerImpl<T, Option<T&>>(&value) {}

    [[nodiscard]] constexpr T& take() {
        T& val = this->m_value;
        this->reset();
        return val;
    }

    [[nodiscard]] constexpr T& get() & { return *this->m_value; }

    [[nodiscard]] constexpr const T& get() const& { return *this->m_value; }
};

template <typename T>
class Option<T*> : public detail::OptionPointerImpl<T, Option<T*>> {
public:
    using detail::OptionPointerImpl<T, Option<T*>>::OptionPointerImpl;

    constexpr Option()
        : detail::OptionPointerImpl<T, Option<T*>>() {}
    constexpr Option(NoneType)
        : detail::OptionPointerImpl<T, Option<T*>>() {}
};

template <>
class Option<void> : detail::OptionEmptyImpl<void> {
    using detail::OptionEmptyImpl<void>::OptionEmptyImpl;
};

template <>
class Option<UnitType> : public detail::OptionEmptyImpl<UnitType> {
    using detail::OptionEmptyImpl<UnitType>::OptionEmptyImpl;
};

template <typename T>
Option<T> some(const T& value) {
    return Option<T>(value);
}

template <typename T>
Option<T> some(T&& value) {
    return Option<T>(std::forward<T>(value));
}

template <typename T, typename... Args>
    requires std::is_constructible_v<T, Args...>
Option<T> some(Args&&... args) {
    return Option<T>(std::forward<Args>(args)...);
}

template <typename T>
Option<T> none() {
    return Option<T>{None};
}

};  // namespace lib::option
