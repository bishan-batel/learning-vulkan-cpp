#include "App.hpp"
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <fmt/printf.h>
#include <fmt/ranges.h>
#include <queue>
#include <spdlog/common.h>
#include <vulkan/vulkan_structs.hpp>
#include <spdlog/spdlog.h>

auto App::run() -> void {
  init_vulkan();
  create_instance();
  pick_physical_device();

  update();

  cleanup();
}

auto App::init_vulkan() -> void {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  spdlog::info("Creating GLFW window");
  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

auto App::create_instance() -> void {
  // get GLFW extensions
  Vec<const char*> extensions{get_required_extensions()};

  spdlog::info(
    "GLFW Required Extensions ({} total): \n{}",
    extensions.size(),
    extensions
  );

  // enumerate supported extensions
  const Vec<vk::ExtensionProperties> extension_properties{
    context.enumerateInstanceExtensionProperties()
  };

  spdlog::info(
    "Vulkan Extension Properties: \n{}",
    extension_properties
      | views::transform([](const vk::ExtensionProperties& x) {
          return StringView{x.extensionName};
        })
  );

  for (const char* extension: extensions) {
    const bool supported{
      ranges::any_of(
        extension_properties,
        [&](const vk::ExtensionProperties& property) {
          return StringView{property.extensionName} == extension;
        }
      ),
    };

    if (not supported) {
      throw std::runtime_error(
        fmt::format("GLFW Extension {} is not supported", extension)
      );
    }
  }

  Vec<const char*> required_validation_layers{};

  // ensure validation layers are met
  if (ENABLE_VALIDATION_LAYERS) {
    required_validation_layers.assign(
      VALIDATION_LAYERS.begin(),
      VALIDATION_LAYERS.end()
    );
  }

  const Vec<vk::LayerProperties> layer_properties{
    context.enumerateInstanceLayerProperties()
  };

  for (const StringView layer: required_validation_layers) {
    const bool supported = ranges::any_of(
      layer_properties,
      [layer](const vk::LayerProperties& property) {
        return layer == property.layerName;
      }
    );

    if (not supported) {
      throw std::runtime_error(
        fmt::format("Unsupported Vulkan Validation Layer '{}'", layer)
      );
    }
  }

  // in-struct initialisers are vile
  vk::InstanceCreateInfo info{};

  info.pApplicationInfo = &APP_INFO,
  info.enabledLayerCount = VALIDATION_LAYERS.size(),
  info.ppEnabledLayerNames = VALIDATION_LAYERS.data(),
  info.enabledExtensionCount = extensions.size(),
  info.ppEnabledExtensionNames = extensions.data(),

#if _OSX
  info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

  spdlog::info("Creating VK Instance");
  instance = context.createInstance(info);
}

auto App::update() -> void {
  while (not glfwWindowShouldClose(window)) {
    glfwPollEvents();
    glfwSwapBuffers(window);
  }
}

auto App::cleanup() -> void {
  spdlog::info("Killing window");
  glfwDestroyWindow(window);
  //
  spdlog::info("Terminating GLFW");
  glfwTerminate();
}

auto App::get_required_extensions() -> Vec<const char*> {
  u32 extension_count = 0;
  Span<const char*> glfw_extensions{
    glfwGetRequiredInstanceExtensions(&extension_count),
    extension_count,
  };

  Vec<const char*> extensions{glfw_extensions.begin(), glfw_extensions.end()};

  if (ENABLE_VALIDATION_LAYERS) {
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
  }

#if _OSX
  extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

  return extensions;
}

auto App::setup_debug_messanger() -> void {
  if (not ENABLE_VALIDATION_LAYERS) {
    return;
  }

  constexpr vk::DebugUtilsMessageSeverityFlagsEXT SEVERITY_FLAGS{
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
  };

  constexpr vk::DebugUtilsMessageTypeFlagsEXT MESSAGE_TYPE_FLAGS =
    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
    | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
    | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

  const vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info{
    .messageSeverity = SEVERITY_FLAGS,
    .messageType = MESSAGE_TYPE_FLAGS,
    .pfnUserCallback = &debug_callback
  };

  auto debug_messager =
    instance.createDebugUtilsMessengerEXT(debug_utils_create_info);
}

vk::Bool32 App::debug_callback(
  vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
  vk::DebugUtilsMessageTypeFlagsEXT type,
  const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
  void* //
) {

  spdlog::level::level_enum level{};
  switch (severity) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
      level = spdlog::level::level_enum::info;
      break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
      level = spdlog::level::level_enum::warn;
      break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
      level = spdlog::level::level_enum::err;
      break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
    default: level = spdlog::level::level_enum::trace; break;
  }

  spdlog::log(
    level,
    "Layer: {} msg: {}",
    vk::to_string(type),
    callback_data->pMessage
  );

  return vk::False;
}

auto App::pick_physical_device() -> void {
  assert(instance != nullptr);

  Vec<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

  if (devices.empty()) {
    throw std::runtime_error{"Could not find a GPU with Vulkan support"};
  }

  for (vk::raii::PhysicalDevice& device: devices) {
    if (not is_device_suitable(device)) {
      continue;
    }

    physical_device = std::move(device);
    break;
  }

  if (physical_device == nullptr) {
    throw std::runtime_error{
      "Could not find any suitable GPU's (Graphics Support)"
    };
  }
}

auto App::is_device_suitable(const vk::raii::PhysicalDevice& device) -> bool {
  const vk::PhysicalDeviceProperties properties{device.getProperties()};

  if (properties.apiVersion < VK_API_VERSION_1_3) {
    spdlog::trace("Unsupporetd API version");
    return false;
  }

  const bool has_queue_family = find_graphics_queue_family(device).is_some();

  if (not has_queue_family) {
    spdlog::trace("No queue family found");
    return false;
  }

  const Vec<vk::ExtensionProperties> extensions{
    device.enumerateDeviceExtensionProperties()
  };

  for (const StringView extension: PHYSICAL_DEVICE_EXTENSIONS) {
    const bool is_supported = ranges::any_of(
      extensions,
      [extension](const vk::ExtensionProperties& requested) {
        return requested.extensionName == extension;
      }
    );

    if (not is_supported) {
      return false;
    }
  }

  return true;
}

auto App::find_graphics_queue_family(const vk::raii::PhysicalDevice& device)
  -> Option<usize> {

  const Vec<vk::QueueFamilyProperties> queue_families{
    device.getQueueFamilyProperties()
  };

  for (usize i = 0; i < queue_families.size(); i++) {
    const vk::QueueFamilyProperties& property{queue_families.at(i)};

    if ((property.queueFlags & vk::QueueFlagBits::eGraphics)
        != static_cast<vk::QueueFlagBits>(0)) {
      return crab::some(i);
    }
  }

  return crab::none;
}
