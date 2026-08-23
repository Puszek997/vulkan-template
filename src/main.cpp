#include <cstdlib>

import vulkan;

template <typename... Ts>
class [[nodiscard]] Error {
public:
    using reason_type = std::variant<std::monostate, Ts...>;

    template <typename T>
        requires std::constructible_from<reason_type, std::in_place_type_t<std::remove_cvref_t<T>>, T>
    explicit constexpr Error(T&& value) noexcept(
        std::is_nothrow_constructible_v<reason_type, std::in_place_type_t<std::remove_cvref_t<T>>, T>
    )
        : m_reason { std::in_place_type<std::remove_cvref_t<T>>, std::forward<T>(value) }
    {
    }

    [[nodiscard]] static constexpr auto to_error() noexcept -> auto
    {
        return []<typename T>
            requires std::constructible_from<Error, T>
        [[nodiscard]] (T&& value) static constexpr noexcept(
            std::is_nothrow_constructible_v<Error, T>
        ) -> Error {
            return Error { std::forward<T>(value) };
        };
    }

    template <typename Self>
    [[nodiscard]] constexpr auto reason(this Self&& self) noexcept -> decltype(auto)
    {
        return (std::forward<Self>(self).m_reason);
    }

private:
    reason_type m_reason;
};

template <typename CharT>
concept CharType = std::same_as<CharT, char> || std::same_as<CharT, wchar_t>;

template <typename CharT>
    requires CharType<CharT>
[[nodiscard]] constexpr auto widen(const char* narrow, const wchar_t* wide) noexcept -> const CharT*
{
    if constexpr (std::same_as<CharT, char>) {
        return narrow;
    } else {
        return wide;
    }
}

template <typename CharT>
    requires CharType<CharT>
struct std::formatter<std::monostate, CharT> {
    [[nodiscard]] static constexpr auto parse(
        const std::basic_format_parse_context<CharT>& context
    ) noexcept -> std::basic_format_parse_context<CharT>::iterator
    {
        return context.begin();
    }

    // TODO: puszek_997 - add constexpr after P3391R2 is implemented
    // TODO: puszek_997 - use macro for widen?
    template <typename Out>
    [[nodiscard]] static auto format(
        [[maybe_unused]] const std::monostate& monostate,
        std::basic_format_context<Out, CharT>& context
    ) -> std::basic_format_context<Out, CharT>::iterator
    {
        return std::format_to(
            context.out(),
            widen<CharT>("{}", L"{}"),
            widen<CharT>("monostate", L"monostate")
        );
    }
};

template <typename T>
concept VkToStringable = requires(T&& value) {
    { vk::to_string(std::forward<T>(value)) } -> std::convertible_to<std::string>;
};

template <typename T, typename CharT>
    requires VkToStringable<T> && CharType<CharT>
struct std::formatter<T, CharT> : public std::formatter<const CharT*, CharT> { // NOLINT(cert-dcl58-cpp, bugprone-std-namespace-modification)
    // TODO: puszek_997 - add constexpr after P3391R2 is implemented
    // TODO: puszek_997 - use macro for widen?
    template <typename Out>
    [[nodiscard]] auto format(
        const T& value,
        std::basic_format_context<Out, CharT>& context
    ) const -> std::basic_format_context<Out, CharT>::iterator
    {
        return std::formatter<const CharT*, CharT>::format(
            widen<CharT>(vk::to_string(value).data(), L"vk::to_string()"),
            context
        );
    }
};

template <typename... Ts, typename CharT>
    requires CharType<CharT>
struct std::formatter<std::variant<Ts...>, CharT> { // NOLINT(cert-dcl58-cpp, bugprone-std-namespace-modification)
    [[nodiscard]] static constexpr auto parse(
        const std::basic_format_parse_context<CharT>& context
    ) noexcept -> std::basic_format_parse_context<CharT>::iterator
    {
        return context.begin();
    }

    // TODO: puszek_997 - add constexpr after P3391R2 is implemented
    // TODO: puszek_997 - use macro for widen?
    template <typename Out>
    [[nodiscard]] static auto format(
        const std::variant<Ts...>& variant,
        std::basic_format_context<Out, CharT>& context
    ) -> std::basic_format_context<Out, CharT>::iterator
    {
        return variant.visit(
            // TODO: puszek_997 - add constexpr after P3391R2 is implemented
            [&context]<typename T> [[nodiscard]] (T&& value) -> std::basic_format_context<Out, CharT>::iterator {
                return std::format_to(
                    context.out(),
                    widen<CharT>("variant({})", L"variant({})"),
                    std::forward<T>(value)
                );
            }
        );
    }
};

template <typename T>
    requires(!std::is_const_v<T>)
[[nodiscard]] constexpr auto store_into(T& out) noexcept -> auto
{
    return [&out]<typename U>
        requires std::assignable_from<T&, U>
    (U&& value) constexpr noexcept(
        std::is_nothrow_assignable_v<T&, U>
    ) -> void {
        out = std::forward<U>(value);
    };
}

auto main() -> std::int32_t
{
    return EXIT_SUCCESS;
}
