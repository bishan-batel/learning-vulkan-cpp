#pragma once

#include <preamble.hpp>
#include <option.hpp>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <filesystem>

[[nodiscard]] auto read_file_contents(StringView path) -> Vec<u8>;

class App {
public:

  static constexpr usize WIDTH = 800;
  static constexpr usize HEIGHT = 800;

  inline static constexpr std::array VALIDATION_LAYERS{
    "VK_LAYER_KHRONOS_validation"
  };

  inline static constexpr std::array DEVICE_EXTENSIONS{
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName,
#if _OSX
    "VK_KHR_portability_subset",
#endif
  };

#ifdef NDEBUG
  static constexpr bool ENABLE_VALIDATION_LAYERS{false};
#else
  static constexpr bool ENABLE_VALIDATION_LAYERS{true};
#endif

  static constexpr vk::ApplicationInfo APP_INFO{
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "No Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = vk::ApiVersion14,
  };

  auto run() -> void;

private:

  auto init_vulkan() -> void;

  auto create_instance() -> void;

  auto setup_debug_messenger() -> void;

  auto update() -> void;

  auto cleanup() -> void;

  auto pick_physical_device() -> void;

  auto create_logical_device() -> void;

  auto create_swap_chain() -> void;

  auto create_image_view() -> void;

  auto create_graphics_pipeline() -> void;

  [[nodiscard]] auto create_shader_module(Span<const u8> code) const
    -> vk::raii::ShaderModule;

  static auto is_device_suitable(const vk::raii::PhysicalDevice& device)
    -> bool;

  static auto find_graphics_queue_family(const vk::raii::PhysicalDevice& device)
    -> Option<u32>;

  [[nodiscard]] static auto get_required_extensions() -> Vec<const char*>;

  static vk::Bool32 debug_callback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* //
  );

  [[nodiscard]] static auto choose_swap_surface_format(
    const std::vector<vk::SurfaceFormatKHR>& available_formats
  ) -> vk::SurfaceFormatKHR;

  [[nodiscard]] static auto choose_swap_surface_present_mode(
    const Vec<vk::PresentModeKHR>& available_modes
  ) -> vk::PresentModeKHR;

  [[nodiscard]] auto choose_swap_extent(
    const vk::SurfaceCapabilitiesKHR& capabilities
  ) -> vk::Extent2D;

private:

  GLFWwindow* window{nullptr};
  vk::raii::Context context{};
  vk::raii::Instance instance{nullptr};
  vk::raii::PhysicalDevice physical_device{nullptr};
  vk::raii::Device device{nullptr};
  vk::raii::Queue graphics_queue{nullptr};
  vk::raii::SurfaceKHR surface{nullptr};

  vk::raii::SwapchainKHR swap_chain{nullptr};
  Vec<vk::Image> swap_chain_images{};
  Vec<vk::raii::ImageView> swap_chain_image_views{};

  vk::raii::Pipeline graphics_pipeline{nullptr};
  vk::raii::PipelineLayout pipeline_layout{nullptr};

  vk::SurfaceFormatKHR swap_chain_surface_format{};
  vk::PresentModeKHR swap_chain_surface_present_mode{};
  vk::Format swap_chain_image_format{vk::Format::eUndefined};
  vk::Extent2D swap_chain_extent{};
};
