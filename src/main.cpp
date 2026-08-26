#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
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

class Application {
public:
    enum struct Result : std::uint8_t {
        eSuccess,
        eError
    };

    using ApplicationError = Error<vk::Result, Result>;

    explicit Application() noexcept
    {
        m_valid = create_window()
                      .and_then(std::bind_front(&Application::create_instance, this))
                      .has_value();
    }

    Application(const Application&) = delete;
    auto operator=(const Application&) -> Application& = delete;
    Application(Application&&) noexcept = delete;
    auto operator=(Application&&) noexcept -> Application& = delete;

    ~Application() noexcept
    {
        glfwTerminate();
    }

    // TODO: puszek_997 - loop func?
    auto run() noexcept -> void
    {
        while (glfwWindowShouldClose(m_window) == GLFW_FALSE) {
            glfwPollEvents();
        }
    }

    [[nodiscard]] auto valid() const noexcept -> bool
    {
        return m_valid;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return m_valid;
    }

private:
    [[nodiscard]] auto create_window() noexcept -> std::expected<void, ApplicationError>
    {
        if (glfwInit() == GLFW_FALSE) {
            return std::expected<void, ApplicationError> { std::unexpect, Result::eError };
        }

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_TRUE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
        glfwWindowHint(GLFW_RED_BITS, 8);
        glfwWindowHint(GLFW_GREEN_BITS, 8);
        glfwWindowHint(GLFW_BLUE_BITS, 8);
        glfwWindowHint(GLFW_ALPHA_BITS, 8);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
        glfwWindowHint(GLFW_STENCIL_BITS, 8);
        glfwWindowHint(GLFW_AUX_BUFFERS, 0);
        glfwWindowHint(GLFW_SAMPLES, 0);
        glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
        glfwWindowHint(GLFW_STEREO, GLFW_FALSE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_WIN32_KEYBOARD_MENU, GLFW_FALSE);
        glfwWindowHintString(GLFW_COCOA_FRAME_NAME, "");
        glfwWindowHintString(GLFW_WAYLAND_APP_ID, "");
        glfwWindowHintString(GLFW_X11_CLASS_NAME, "");
        glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "");

        m_window = glfwCreateWindow(800, 600, "Hello Triangle", nullptr, nullptr);
        if (m_window == nullptr) {
            return std::expected<void, ApplicationError> { std::unexpect, Result::eError };
        }

        return std::expected<void, ApplicationError> { std::in_place };
    }

    [[nodiscard]] auto create_instance() noexcept -> std::expected<void, ApplicationError>
    {
        static constexpr vk::ApplicationInfo APPLICATION_INFO {
            .pApplicationName = "Hello Triangle",
            .applicationVersion = vk::makeVersion(0, 1, 0),
            .pEngineName = "No Engine",
            .engineVersion = vk::makeVersion(0, 1, 0),
            .apiVersion = vk::ApiVersion14
        };

        std::uint32_t required_instance_extension_count { };
        const char* const* const required_instance_extension_names { glfwGetRequiredInstanceExtensions(&required_instance_extension_count) };
        if (required_instance_extension_names == nullptr) {
            return std::expected<void, ApplicationError> { std::unexpect, Result::eError };
        }

        const std::vector<const char*> required_instance_extensions {
            required_instance_extension_names,
            std::ranges::next(required_instance_extension_names, required_instance_extension_count)
        };

        const vk::InstanceCreateInfo instance_create_info {
            .pApplicationInfo = &APPLICATION_INFO,
            .enabledExtensionCount = static_cast<std::uint32_t>(required_instance_extensions.size()),
            .ppEnabledExtensionNames = required_instance_extensions.data(),
        };

        return m_context
            .createInstance(instance_create_info)
            .transform(store_into(m_instance))
            .transform_error(ApplicationError::to_error());
    }

    GLFWwindow* m_window { nullptr };
    vk::raii::Context m_context;
    vk::raii::Instance m_instance { nullptr };
    bool m_valid { false };
    [[maybe_unused]] std::array<std::byte, 7> m_padding { };
};

auto main() -> std::int32_t
{
    Application application { };
    if (application.valid()) {
        application.run();
    }

    return EXIT_SUCCESS;
}
