
#pragma once

#include <preamble.hpp>
#include <result.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

class VulkanError final : public crab::Error {
public:

  vk::Result result;
};
