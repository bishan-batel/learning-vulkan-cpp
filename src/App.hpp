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

  auto update() -> void;

  auto cleanup() -> void;

private:
  GLFWwindow *window{nullptr};
  vk::raii::Context context{};
  vk::raii::Instance instance{nullptr};
};
