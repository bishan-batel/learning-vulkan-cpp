#include "App.hpp"
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <fmt/printf.h>
#include <fmt/ranges.h>
#include <queue>
#include <spdlog/common.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

auto App::init_vulkan() -> void {

  create_instance();
  setup_debug_messenger();
  pick_physical_device();
  create_logical_device();
  create_swap_chain();
  create_image_view();
  create_graphics_pipeline();
  create_command_pool();
  create_command_buffer();
}

auto read_file_contents(const StringView path) -> Vec<u8> {
  std::ifstream file{path.data(), std::ios::binary | std::ios::ate};

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  std::vector<u8> buffer{};
  buffer.reserve(static_cast<usize>(file.tellg()));

  for (i32 i = 0; i < file.tellg(); i++) {
    buffer.push_back(0);
  }

  file.seekg(0, std::ios::beg);
  file.read(
    reinterpret_cast<char*>(buffer.data()),
    static_cast<ptrdiff>(buffer.size())
  );

  return buffer;
}

auto App::run() -> void {
  init_vulkan();

  update();

  cleanup();
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
  Vec<vk::ExtensionProperties> extension_properties{
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

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  spdlog::info("Creating GLFW window");
  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

auto App::update() -> void {
  spdlog::info("Update loop started");

  while (not glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

auto App::cleanup() -> void {

  pipeline_layout.clear();
  graphics_pipeline.clear();
  swap_chain_image_views.clear();
  swap_chain.clear();
  graphics_queue.clear();
  device.clear();
  surface.clear();
  physical_device.clear();

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
  extensions.push_back("VK_EXT_metal_surface");
  extensions.push_back(vk::KHRSurfaceExtensionName);
#endif

  return extensions;
}

auto App::setup_debug_messenger() -> void {
  if (not ENABLE_VALIDATION_LAYERS) {
    return;
  }
  spdlog::info("Setting up debug messenger");

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
  spdlog::info("Picking physical device");

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

  for (const StringView extension: DEVICE_EXTENSIONS) {
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
  -> Option<u32> {

  const Vec<vk::QueueFamilyProperties> queue_families{
    device.getQueueFamilyProperties()
  };

  for (u32 i = 0; i < queue_families.size(); i++) {
    const vk::QueueFamilyProperties& property{queue_families.at(i)};

    if ((property.queueFlags & vk::QueueFlagBits::eGraphics)
        != static_cast<vk::QueueFlagBits>(0)) {
      return i;
    }
  }

  return crab::none;
}

auto App::create_logical_device() -> void {
  spdlog::info("Creating logical device");

  const Option<u32> graphics_queue_index{
    find_graphics_queue_family(physical_device)
  };

  if (graphics_queue_index.is_none()) {
    throw std::runtime_error{
      std::format("Could not find graphics queue family for device")
    };
  }

  const f32 priorities{0.f};
  vk::DeviceQueueCreateInfo queue_create_info{
    .queueFamilyIndex = graphics_queue_index.get_unchecked(),
    .queueCount = 1,
    .pQueuePriorities = &priorities
  };

  const vk::StructureChain<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan13Features,
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
    feature_name{
      {},
      {.dynamicRendering = true},
      {.extendedDynamicState = true}
    };

  vk::DeviceCreateInfo device_create_info{
    .pNext = &feature_name.get<vk::PhysicalDeviceFeatures2>(),
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queue_create_info,
    .enabledExtensionCount = static_cast<u32>(DEVICE_EXTENSIONS.size()),
    .ppEnabledExtensionNames = DEVICE_EXTENSIONS.data()

  };

  device = vk::raii::Device{
    physical_device,
    device_create_info,
  };

  graphics_queue = vk::raii::Queue{
    device,
    graphics_queue_index.get_unchecked(),
    0,
  };

  // create surface
  VkSurfaceKHR vk_surface{};

  if (glfwCreateWindowSurface(*instance, window, nullptr, &vk_surface)) {
    const char* error{};
    glfwGetError(&error);
    spdlog::warn("GLFW: {}", error);
    throw std::runtime_error("Failed to create window surface");
  }

  surface = {instance, vk_surface};
}

auto App::create_swap_chain() -> void {
  const vk::SurfaceCapabilitiesKHR surface_capabilities{
    physical_device.getSurfaceCapabilitiesKHR(surface)
  };

  const Vec<vk::SurfaceFormatKHR> available_formats{
    physical_device.getSurfaceFormatsKHR(surface)
  };

  const Vec<vk::PresentModeKHR> presentation_modes{
    physical_device.getSurfacePresentModesKHR(surface)
  };

  swap_chain_surface_format = choose_swap_surface_format(available_formats);

  swap_chain_surface_present_mode =
    choose_swap_surface_present_mode(presentation_modes);
  swap_chain_extent = choose_swap_extent(surface_capabilities);

  u32 min_image_count = std::max(3u, surface_capabilities.minImageCount);
  min_image_count = {
    (surface_capabilities.maxImageCount > 0
     and min_image_count > surface_capabilities.maxImageCount)
      ? surface_capabilities.maxImageCount
      : min_image_count
  };

  u32 image_count = surface_capabilities.minImageCount + 1;

  if (surface_capabilities.maxImageCount > 0
      && image_count > surface_capabilities.maxImageCount) {
    image_count = surface_capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR swap_chain_create_info{
    .flags = vk::SwapchainCreateFlagsKHR(),
    .surface = surface,
    .minImageCount = min_image_count,
    .imageFormat = swap_chain_surface_format.format,
    .imageColorSpace = swap_chain_surface_format.colorSpace,
    .imageExtent = swap_chain_extent,
    .imageArrayLayers = 1,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
    .imageSharingMode = vk::SharingMode::eExclusive,
    .preTransform = surface_capabilities.currentTransform,
    .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
    .presentMode = choose_swap_surface_present_mode(presentation_modes),
    .clipped = true,
    .oldSwapchain = nullptr
  };

  swap_chain_image_format = swap_chain_create_info.imageFormat;

  swap_chain = vk::raii::SwapchainKHR(device, swap_chain_create_info);
  swap_chain_images = swap_chain.getImages();
}

auto App::choose_swap_surface_format(
  const std::vector<vk::SurfaceFormatKHR>& available_formats
) -> vk::SurfaceFormatKHR {
  for (const vk::SurfaceFormatKHR& format: available_formats) {
    if (format.format != vk::Format::eB8G8R8A8Srgb) {
      continue;
    }

    if (format.colorSpace != vk::ColorSpaceKHR::eSrgbNonlinear) {
      continue;
    }

    return format;
  }
  return available_formats[0];
}

auto App::choose_swap_surface_present_mode(
  const Vec<vk::PresentModeKHR>& available_modes
) -> vk::PresentModeKHR {
  for (const auto& mode: available_modes) {
    if (mode == vk::PresentModeKHR::eMailbox) {
      return mode;
    }
  }
  return vk::PresentModeKHR::eFifo;
}

auto App::choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities)
  -> vk::Extent2D {
  if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
    return capabilities.currentExtent;
  }

  i32 width{}, height{};
  glfwGetFramebufferSize(window, &width, &height);

  return {
    std::clamp<u32>(
      width,
      capabilities.minImageExtent.width,
      capabilities.maxImageExtent.width
    ),
    std::clamp<u32>(
      height,
      capabilities.minImageExtent.height,
      capabilities.maxImageExtent.height
    )
  };
}

auto App::create_image_view() -> void {
  swap_chain_image_views.clear();

  const vk::ImageSubresourceRange subresource_range{
    .aspectMask = vk::ImageAspectFlagBits::eColor,
    .baseMipLevel = 0,
    .levelCount = 1,
    .baseArrayLayer = 0,
    .layerCount = 1,
  };

  vk::ImageViewCreateInfo image_view_create_info{
    .viewType = vk::ImageViewType::e2D,
    .format = swap_chain_image_format,
    .subresourceRange = subresource_range
  };

  for (const vk::Image& image: swap_chain_images) {
    image_view_create_info.image = image;
    swap_chain_image_views.emplace_back(device, image_view_create_info);
  }
}

auto App::create_graphics_pipeline() -> void {
  spdlog::info("Creating Graphics Pipeline");
  const Vec<u8> shader_code{read_file_contents("shaders/slang.spv")};

  spdlog::info("Creating shader module");
  vk::raii::ShaderModule module = create_shader_module(shader_code);

  const vk::PipelineShaderStageCreateInfo vert_shader_stage_info{
    .stage = vk::ShaderStageFlagBits::eVertex,
    .module = module,
    .pName = "vertMain"
  };

  const vk::PipelineShaderStageCreateInfo frag_shader_stage_info{
    .stage = vk::ShaderStageFlagBits::eFragment,
    .module = module,
    .pName = "fragMain"
  };

  const vk::PipelineShaderStageCreateInfo stages[] = {
    vert_shader_stage_info,
    frag_shader_stage_info
  };

  constexpr std::array DYNAMIC_STATES = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  const vk::PipelineDynamicStateCreateInfo dynamic_state{
    .dynamicStateCount = static_cast<uint32_t>(DYNAMIC_STATES.size()),
    .pDynamicStates = DYNAMIC_STATES.data()
  };

  const vk::PipelineVertexInputStateCreateInfo vertex_input_info;

  const vk::PipelineInputAssemblyStateCreateInfo input_assembly{
    .topology = vk::PrimitiveTopology::eTriangleList
  };

  const vk::Viewport viewport{
    0.0f,
    0.0f,
    static_cast<f32>(swap_chain_extent.width),
    static_cast<f32>(swap_chain_extent.height),
    0.0f,
    1.0f
  };

  const vk::PipelineViewportStateCreateInfo viewport_state{
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1
  };

  const vk::PipelineRasterizationStateCreateInfo rasterizer{
    .depthClampEnable = vk::False,
    .rasterizerDiscardEnable = vk::False,
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eBack,
    .frontFace = vk::FrontFace::eClockwise,
    .depthBiasEnable = vk::False,
    .depthBiasSlopeFactor = 1.0f,
    .lineWidth = 1.0f
  };

  const vk::PipelineMultisampleStateCreateInfo multisampling{
    .rasterizationSamples = vk::SampleCountFlagBits::e1,
    .sampleShadingEnable = vk::False,
  };

  const vk::PipelineColorBlendAttachmentState color_blend_attachment{
    .blendEnable = vk::False,
    .colorWriteMask = vk::ColorComponentFlagBits::eR //
                    | vk::ColorComponentFlagBits::eG
                    | vk::ColorComponentFlagBits::eB
                    | vk::ColorComponentFlagBits::eA,
  };

  const vk::PipelineColorBlendStateCreateInfo color_blend_state{
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment
  };

  vk::PipelineLayoutCreateInfo layout_info{
    .setLayoutCount = 0,
    .pushConstantRangeCount = 0
  };

  spdlog::info("Creating pipeline layout");
  pipeline_layout = vk::raii::PipelineLayout(device, layout_info);

  vk::PipelineRenderingCreateInfo pipeling_rendering_create_info{
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &swap_chain_image_format
  };

  vk::GraphicsPipelineCreateInfo pipeline_info{
    .pNext = &pipeling_rendering_create_info,
    .stageCount = 2,
    .pStages = &stages[0],
    .pVertexInputState = &vertex_input_info,
    .pInputAssemblyState = &input_assembly,
    .pViewportState = &viewport_state,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pColorBlendState = &color_blend_state,
    .pDynamicState = &dynamic_state,
    .layout = pipeline_layout,
    .renderPass = nullptr,
    .basePipelineHandle = VK_NULL_HANDLE
  };

  spdlog::info("Creating graphics pipeline");

  graphics_pipeline = vk::raii::Pipeline{
    device,
    nullptr,
    pipeline_info,
  };

  // colorBlendAttachment.blendEnable = vk::True;
  // colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
  // colorBlendAttachment.dstColorBlendFactor =
  // vk::BlendFactor::eOneMinusSrcAlpha; colorBlendAttachment.colorBlendOp =
  // vk::BlendOp::eAdd; colorBlendAttachment.srcAlphaBlendFactor =
  // vk::BlendFactor::eOne; colorBlendAttachment.dstAlphaBlendFactor =
  // vk::BlendFactor::eZero; colorBlendAttachment.alphaBlendOp =
  // vk::BlendOp::eAdd;
}

auto App::create_shader_module(Span<const u8> code) const
  -> vk::raii::ShaderModule {
  spdlog::info("{}", code.size());
  vk::ShaderModuleCreateInfo info{
    .codeSize = code.size_bytes(),
    .pCode = reinterpret_cast<const uint32_t*>(code.data()) // NOLINT
  };

  vk::raii::ShaderModule module{device, info};
  return module;
}

auto App::create_command_pool() -> void {
  vk::CommandPoolCreateInfo pool_info{
    .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    .queueFamilyIndex = graphics_index,
  };

  command_pool = vk::raii::CommandPool{device, pool_info};
}

auto App::create_command_buffer() -> void {}
