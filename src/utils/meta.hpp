#pragma once

#include <type_traits>
#include <variant>

namespace meta {

template <typename T, typename Variant>
struct is_variant_member : std::false_type {};

template <typename T, typename... Types>
struct is_variant_member<T, std::variant<Types...>>
    : std::disjunction<std::is_same<T, Types>...> {};

template <typename T, typename Variant>
inline constexpr bool is_variant_member_v = is_variant_member<T, Variant>::value;

template <typename T, typename Variant>
concept variant_member = is_variant_member_v<T, Variant>;

}  // namespace meta