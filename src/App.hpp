#pragma once

#include <iostream>
#include <preamble.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class App {
public:

  static constexpr usize WIDTH = 800;
  static constexpr usize HEIGHT = 800;

  inline static constexpr std::array VALIDATION_LAYERS{
    "VK_LAYER_KHRONOS_validation"
  };

#ifdef NDEBUG
  static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
  static constexpr bool ENABLE_VALIDATION_LAYERS = true;
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

  auto setup_debug_messanger() -> void;

  auto update() -> void;

  auto cleanup() -> void;

  auto debug_callback() -> void {}

  [[nodiscard]] static auto get_required_extensions() -> Vec<const char*>;

  static vk::Bool32 debug_callback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* //
  );

private:

  GLFWwindow* window{nullptr};
  vk::raii::Context context{};
  vk::raii::Instance instance{nullptr};
};
