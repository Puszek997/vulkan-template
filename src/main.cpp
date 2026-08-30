#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdlib>

import vulkan;

template <typename... Ts>
    requires(std::same_as<std::remove_cvref_t<Ts>, Ts> && ...)
class [[nodiscard]] Error {
public:
    using ReasonType = std::variant<std::monostate, Ts...>;

    template <typename T>
        requires(std::same_as<std::remove_cvref_t<T>, Ts> || ...)
    explicit constexpr Error(T&& value) noexcept(
        std::is_nothrow_constructible_v<ReasonType, std::in_place_type_t<std::remove_cvref_t<T>>, T>
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
    ReasonType m_reason;
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

#ifdef NDEBUG
inline constexpr bool ENABLE_VALIDATION_LAYERS { false };
#else
inline constexpr bool ENABLE_VALIDATION_LAYERS { true };
#endif

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
                      .and_then(std::bind_front(&Application::create_surface, this))
                      .and_then(std::bind_front(&Application::pick_physical_device, this))
                      .and_then(std::bind_front(&Application::create_logical_device, this))
                      .and_then(std::bind_front(&Application::create_swap_chain, this))
                      .and_then(std::bind_front(&Application::create_image_views, this))
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

        std::uint32_t required_instance_extension_count { 0 };
        const char* const* const required_instance_extension_names { glfwGetRequiredInstanceExtensions(&required_instance_extension_count) };
        if (required_instance_extension_names == nullptr) {
            return std::expected<void, ApplicationError> { std::unexpect, Result::eError };
        }

        std::vector<const char*> required_instance_extensions {
            required_instance_extension_names,
            std::ranges::next(required_instance_extension_names, required_instance_extension_count)
        };
        required_instance_extensions.emplace_back(vk::KHRGetSurfaceCapabilities2ExtensionName);

        if constexpr (ENABLE_VALIDATION_LAYERS) {
            required_instance_extensions.emplace_back(vk::EXTDebugUtilsExtensionName);

            static constexpr std::array<const char*, 1> VALIDATION_LAYERS {
                "VK_LAYER_KHRONOS_validation",
            };

            const vk::InstanceCreateInfo instance_create_info {
                .pApplicationInfo = &APPLICATION_INFO,
                .enabledLayerCount = static_cast<std::uint32_t>(VALIDATION_LAYERS.size()),
                .ppEnabledLayerNames = VALIDATION_LAYERS.data(),
                .enabledExtensionCount = static_cast<std::uint32_t>(required_instance_extensions.size()),
                .ppEnabledExtensionNames = required_instance_extensions.data(),
            };

            return m_context
                .createInstance(instance_create_info)
                .transform(store_into(m_instance))
                .transform_error(ApplicationError::to_error());
        } else {
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
    }

    [[nodiscard]] auto create_surface() noexcept -> std::expected<void, ApplicationError>
    {
        VkSurfaceKHR surface { nullptr };
        if (vk::Result result { glfwCreateWindowSurface(*m_instance, m_window, nullptr, &surface) };
            result != vk::Result::eSuccess) {
            return std::expected<void, ApplicationError> { std::unexpect, result };
        }

        m_surface = vk::raii::SurfaceKHR(m_instance, surface);

        return std::expected<void, ApplicationError> { std::in_place };
    }

    // TODO: puszek_997 - lambda w and_then ktora zwroci vector vectorow vk::Bool32 surface_support?
    // TODO: puszek_997 - i tak device moze byc zle :(, check device features, extensions
    [[nodiscard]] auto pick_physical_device() noexcept -> std::expected<void, ApplicationError>
    {
        return m_instance
            .enumeratePhysicalDevices()
            .transform_error(ApplicationError::to_error())
            .and_then([this] [[nodiscard]] (const std::vector<vk::raii::PhysicalDevice>& physical_devices) noexcept -> std::expected<void, ApplicationError> {
                for (const vk::raii::PhysicalDevice& physical_device : physical_devices) {
                    if (physical_device.getProperties2().properties.apiVersion < vk::ApiVersion14) {
                        continue;
                    }

                    for (auto&& [index, queue_family_properties2] : physical_device.getQueueFamilyProperties2() | std::views::enumerate) {
                        vk::Bool32 surface_support { vk::False };

                        // TODO: puszek_997 - maybe change this if to macro so it will feel like rust early return "?"
                        if (
                            std::expected<void, ApplicationError> result {
                                physical_device
                                    .getSurfaceSupportKHR(static_cast<std::uint32_t>(index), *m_surface)
                                    .transform(store_into(surface_support))
                                    .transform_error(ApplicationError::to_error()) };
                            !result.has_value()
                        ) {
                            return result;
                        }

                        if (
                            static_cast<bool>(queue_family_properties2.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)
                            && static_cast<bool>(surface_support)
                        ) {
                            m_physical_device = physical_device;
                            m_queue_family_index = static_cast<std::uint32_t>(index);
                            return std::expected<void, ApplicationError> { std::in_place };
                        }
                    }
                }

                return std::expected<void, ApplicationError> { std::unexpect, Result::eError };
            });
    }

    [[nodiscard]] auto create_logical_device() noexcept -> std::expected<void, ApplicationError>
    {
        static constexpr float QUEUE_PRIORITY { 0.5F };
        const std::array<vk::DeviceQueueCreateInfo, 1> device_queue_create_infos { {
            {
                .queueFamilyIndex = m_queue_family_index,
                .queueCount = 1,
                .pQueuePriorities = &QUEUE_PRIORITY,
            },
        } };

        const vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        >
            feature_chain {
                { },
                { .shaderDrawParameters = vk::True },
                {
                    .synchronization2 = vk::True,
                    .dynamicRendering = vk::True,
                },
                { .extendedDynamicState = vk::True }
            };

        static constexpr std::array<const char*, 1> REQUIRED_DEVICE_EXTENSIONS {
            vk::KHRSwapchainExtensionName
        };

        const vk::DeviceCreateInfo device_create_info {
            .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = static_cast<std::uint32_t>(device_queue_create_infos.size()),
            .pQueueCreateInfos = device_queue_create_infos.data(),
            .enabledExtensionCount = static_cast<std::uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size()),
            .ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data(),
        };

        return m_physical_device
            .createDevice(device_create_info)
            .transform([this](vk::raii::Device&& device) -> void {
                m_device = std::move(device);
                m_queue = m_device.getQueue(m_queue_family_index, 0);
            })
            .transform_error(ApplicationError::to_error());
    }

    [[nodiscard]] auto create_swap_chain() noexcept -> std::expected<void, ApplicationError>
    {
        const vk::PhysicalDeviceSurfaceInfo2KHR physical_device_surface_info2_khr {
            .surface = *m_surface
        };

        return m_physical_device
            .getSurfaceFormats2KHR(physical_device_surface_info2_khr)
            .transform([this](const std::vector<vk::SurfaceFormat2KHR>& surface_formats) constexpr noexcept -> void {
                const std::ranges::borrowed_iterator_t<const std::vector<vk::SurfaceFormat2KHR>&> surface_format_it {
                    std::ranges::find_if(
                        surface_formats,
                        [] [[nodiscard]] (const vk::SurfaceFormat2KHR& surface_format) static constexpr noexcept -> bool {
                            return surface_format.surfaceFormat.format == vk::Format::eB8G8R8A8Srgb
                                && surface_format.surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                        }
                    )
                };

                m_swap_chain_surface_format2_khr = surface_format_it != surface_formats.end() ? *surface_format_it : surface_formats.at(0);
            })
            .and_then([&physical_device_surface_info2_khr, this] [[nodiscard]] noexcept -> std::expected<vk::SurfaceCapabilities2KHR, vk::Result> {
                return m_physical_device
                    .getSurfaceCapabilities2KHR(physical_device_surface_info2_khr)
                    .transform([this] [[nodiscard]] (const vk::SurfaceCapabilities2KHR& surface_capabilities2_khr) noexcept -> vk::SurfaceCapabilities2KHR {
                        if (surface_capabilities2_khr.surfaceCapabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
                            m_swap_chain_extent = surface_capabilities2_khr.surfaceCapabilities.currentExtent;
                            return surface_capabilities2_khr;
                        }

                        std::int32_t width { };
                        std::int32_t height { };
                        glfwGetFramebufferSize(m_window, &width, &height);

                        m_swap_chain_extent = {
                            .width = std::ranges::clamp(
                                static_cast<std::uint32_t>(width),
                                surface_capabilities2_khr.surfaceCapabilities.minImageExtent.width,
                                surface_capabilities2_khr.surfaceCapabilities.maxImageExtent.width
                            ),
                            .height = std::ranges::clamp(
                                static_cast<std::uint32_t>(height),
                                surface_capabilities2_khr.surfaceCapabilities.minImageExtent.height,
                                surface_capabilities2_khr.surfaceCapabilities.maxImageExtent.height
                            )
                        };

                        return surface_capabilities2_khr;
                    });
            })
            .transform(
                [] [[nodiscard]] (
                    const vk::SurfaceCapabilities2KHR& surface_capabilities2_khr
                ) static constexpr noexcept -> std::tuple<std::uint32_t, vk::SurfaceCapabilities2KHR> {
                    std::uint32_t min_image_count { std::max<std::uint32_t>(3, surface_capabilities2_khr.surfaceCapabilities.minImageCount) };
                    if (
                        (0 != surface_capabilities2_khr.surfaceCapabilities.maxImageCount)
                        && (surface_capabilities2_khr.surfaceCapabilities.maxImageCount < min_image_count)
                    ) {
                        min_image_count = surface_capabilities2_khr.surfaceCapabilities.maxImageCount;
                    }

                    return std::tuple<std::uint32_t, vk::SurfaceCapabilities2KHR> { min_image_count, surface_capabilities2_khr };
                }
            )
            .and_then(
                [this] [[nodiscard]] (
                    std::tuple<std::uint32_t, vk::SurfaceCapabilities2KHR>&& swap_chain_properties
                ) noexcept -> std::expected<std::tuple<std::uint32_t, vk::SurfaceCapabilities2KHR, vk::PresentModeKHR>, vk::Result> {
                    return m_physical_device
                        .getSurfacePresentModesKHR(*m_surface)
                        .transform(
                            [captured_swap_chain_properties = std::move(swap_chain_properties)] [[nodiscard]] (
                                const std::vector<vk::PresentModeKHR>& present_modes
                            ) mutable noexcept -> std::tuple<std::uint32_t, vk::SurfaceCapabilities2KHR, vk::PresentModeKHR> {
                                return std::tuple_cat(
                                    std::move(captured_swap_chain_properties),
                                    std::tuple<vk::PresentModeKHR> {
                                        std::ranges::contains(present_modes, vk::PresentModeKHR::eMailbox)
                                            ? vk::PresentModeKHR::eMailbox
                                            : vk::PresentModeKHR::eFifo }
                                );
                            }
                        );
                }
            )
            .and_then(
                [this] [[nodiscard]] (
                    const std::tuple<std::uint32_t, vk::SurfaceCapabilities2KHR, vk::PresentModeKHR>& swap_chain_properties
                ) noexcept -> std::expected<void, vk::Result> {
                    const vk::SwapchainCreateInfoKHR swap_chain_create_info {
                        .surface = *m_surface,
                        .minImageCount = std::get<std::uint32_t>(swap_chain_properties),
                        .imageFormat = m_swap_chain_surface_format2_khr.surfaceFormat.format,
                        .imageColorSpace = m_swap_chain_surface_format2_khr.surfaceFormat.colorSpace,
                        .imageExtent = m_swap_chain_extent,
                        .imageArrayLayers = 1,
                        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                        .imageSharingMode = vk::SharingMode::eExclusive,
                        .preTransform = std::get<vk::SurfaceCapabilities2KHR>(swap_chain_properties).surfaceCapabilities.currentTransform,
                        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                        .presentMode = std::get<vk::PresentModeKHR>(swap_chain_properties),
                        .clipped = vk::True,
                        .oldSwapchain = nullptr
                    };

                    return m_device
                        .createSwapchainKHR(swap_chain_create_info)
                        .and_then([this] [[nodiscard]] (vk::raii::SwapchainKHR&& swap_chain) noexcept -> std::expected<void, vk::Result> {
                            m_swap_chain = std::move(swap_chain);
                            return m_swap_chain
                                .getImages()
                                .transform(store_into(m_swap_chain_images));
                        });
                }
            )
            .transform_error(ApplicationError::to_error());
    }

    [[nodiscard]] auto create_image_views() noexcept -> std::expected<void, ApplicationError>
    {
        // TODO: puszek_997 - assert(swapChainImageViews.empty()); from vk tutorial
        vk::ImageViewCreateInfo image_view_create_info {
            .viewType = vk::ImageViewType::e2D,
            .format = m_swap_chain_surface_format2_khr.surfaceFormat.format,
            .components = {
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity,
            },
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        m_swap_chain_image_views.reserve(m_swap_chain_images.size());

        for (const vk::Image& image : m_swap_chain_images) {
            image_view_create_info.image = image;
            // TODO: puszek_997 - maybe change this if to macro so it will feel like rust early return "?"
            if (
                std::expected<void, ApplicationError> result {
                    m_device
                        .createImageView(image_view_create_info)
                        .transform([this](vk::raii::ImageView&& image_view) noexcept -> void {
                            m_swap_chain_image_views.emplace_back(std::move(image_view));
                        })
                        .transform_error(ApplicationError::to_error()) };
                !result.has_value()
            ) {
                return result;
            }
        }

        return std::expected<void, ApplicationError> { std::in_place };
    }

    GLFWwindow* m_window { nullptr };
    vk::raii::Context m_context;
    vk::raii::Instance m_instance { nullptr };
    vk::raii::SurfaceKHR m_surface { nullptr };
    vk::raii::PhysicalDevice m_physical_device { nullptr };
    vk::raii::Device m_device { nullptr };
    vk::raii::Queue m_queue { nullptr };
    vk::SurfaceFormat2KHR m_swap_chain_surface_format2_khr { };
    vk::Extent2D m_swap_chain_extent { };
    vk::raii::SwapchainKHR m_swap_chain { nullptr };
    std::vector<vk::Image> m_swap_chain_images;
    std::vector<vk::raii::ImageView> m_swap_chain_image_views;
    std::uint32_t m_queue_family_index { 0 };
    bool m_valid { false };
    [[maybe_unused]] std::array<std::byte, 3> m_padding { };
};

auto main() -> std::int32_t
{
    Application application { };
    if (application.valid()) {
        application.run();
    }

    return EXIT_SUCCESS;
}
