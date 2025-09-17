#pragma once

#include <vulkan/vulkan.hpp>
#include <option.hpp>

template<typename T>
[[nodiscard]] auto vk_to_crab(vk::Optional<T> opt) -> Option<T&> {
  return opt ? Option<T&>{*opt.get()} : Option<T&>{};
};

template<typename T>
[[nodiscard]] auto crab_to_vk(Option<T&> opt) -> vk::Optional<T> {
  return opt ? vk::Optional<T>{&opt.get_unchecked()} : vk::Optional<T>{};
};
