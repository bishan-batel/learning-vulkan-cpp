cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(cs300-a1 VERSION 0.1)

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)

if (APPLE) 
	find_package(Vulkan REQUIRED MoltenVK)
else()
	find_package(Vulkan REQUIRED)
endif()

add_compile_options(-fdiagnostics-color=always)
add_compile_options(
	-Wextra 
	-Wall -Wpedantic 
	-std=c++20
)

include(cmake/CPM.cmake)

CPMAddPackage("gh:bishan-batel/crab#reference-type-support")

if (WIN32)
	CPMAddPackage("gh:gabime/spdlog@1.15.3")
	CPMAddPackage("gh:fmtlib/fmt#7.1.3")
else() 
	find_package(spdlog REQUIRED)
	find_package(fmt REQUIRED)
endif()

set(SLANGC_EXECUTABLE slangc)

function (add_slang_shader_target TARGET)
  cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
  set (SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)
  set (ENTRY_POINTS -entry vertMain -entry fragMain)
  add_custom_command (
          OUTPUT ${SHADERS_DIR}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )
  add_custom_command (
          OUTPUT  ${SHADERS_DIR}/slang.spv
          COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
          COMMENT "Compiling Slang Shaders"
          VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

add_executable(learn-vulkan
	./src/main.cpp
	./src/App.cpp
)

set(SHADER_SLANG_SOURCES ${PROJECT_SOURCE_DIR}/shaders/triangle.slang)

add_slang_shader_target(learn-vulkan-shaders SOURCES ${SHADER_SLANG_SOURCES})
add_dependencies(learn-vulkan learn-vulkan-shaders)

target_link_libraries(learn-vulkan PUBLIC glm::glm glfw Vulkan::Vulkan crab fmt spdlog)

target_compile_definitions(learn-vulkan PUBLIC 
	"VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1"
	"GLFW_INCLUDE_VULKAN=1"
)

if(APPLE)
	target_compile_definitions(learn-vulkan PUBLIC "_OSX=1")
elseif(NOT WIN32)
	target_compile_definitions(learn-vulkan PUBLIC "_LINUX=1")
endif()
