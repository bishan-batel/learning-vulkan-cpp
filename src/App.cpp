#include "App.hpp"
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <vulkan/vulkan_structs.hpp>

auto App::run() -> void {
  init_vulkan();
  update();
  cleanup();
}

auto App::init_vulkan() -> void {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

auto App::create_instance() -> void {
  u32 extension_count{0};
  Span<const char*> extensions{
    glfwGetRequiredInstanceExtensions(&extension_count),
    extension_count
  };

  Vec<vk::ExtensionProperties> extension_properties{
    context.enumerateInstanceExtensionProperties()
  };

  for (const char* extension: extensions) {
    const bool supported{
      ranges::none_of(
        extension_properties,
        [&](const vk::ExtensionProperties& property) {
          return StringView{property.extensionName} == extension;
        }
      ),
    };

    if (not supported) {
      throw std::runtime_error(
        fmt::format("GLFW Extension {:q} is not supported", extension)
      );
    }
  }

  vk::InstanceCreateInfo creation_info{
    .pApplicationInfo = &APP_INFO,
    .enabledExtensionCount = extension_count,
    .ppEnabledExtensionNames = extensions.data()
  };

  instance = vk::raii::Instance{context, creation_info};
}

auto App::update() -> void {
  while (not glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

auto App::cleanup() -> void {
  glfwDestroyWindow(window);
  glfwTerminate();
}
