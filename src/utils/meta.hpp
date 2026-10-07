#pragma once

#include <type_traits>
#include <variant>

namespace meta {

template <typename T, typename Variant>
struct is_variant_member;

template <typename T, typename... Types>
struct is_variant_member<T, std::variant<Types...>>
    : std::disjunction<std::is_same<T, Types>...> {};

}  // namespace meta