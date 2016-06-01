
#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

class my_vulkan{
private:
  const char* app_name_;
  const char* engine_name_;

  bool init_success_;
  bool image_loaded_;

  // result
  double** result_image_;

  // image info
  unsigned width_;
  unsigned height_;

  // general
  VkInstance       inst_;
  VkPhysicalDevice my_gpu_;
  VkDevice         my_device_;
  VkQueue          my_queue_;
  VkDeviceMemory   my_memory_;

  // buffer
  static const unsigned num_variables_ = 5u;
  static const unsigned num_buffers_   = 4u;
  VkBuffer buffer_0_;
  VkBuffer buffer_1_;
  VkBuffer buffer_2_;
  VkBuffer buffer_variables_;

  // shader
  VkShaderModule shader_module_;

  // descriptor
  VkDescriptorSetLayout desc_set_layout_;
  VkDescriptorPool desc_pool_;
  VkDescriptorSet  desc_set_;

  // pipeline
  VkPipelineLayout pipeline_layout_;
  VkPipeline pipeline_;

  // command pool
  VkCommandPool command_pool_;
  VkCommandBuffer command_buffer_;

  // stuff
  unsigned queue_family_index_;
  unsigned heap_index_;
  unsigned memory_to_alloc_;

  VkAllocationCallbacks alloc_cb_;

private:
  // TODO mask
  my_vulkan(const char* app_name, const char* engine_name);

  void load_shader();
  //void unload_shader();

  void load_desc();
  //void unload_desc();

  void load_pipeline();
  //void unload_pipleline();

  void load_command_pool();
  //void unload_command_pool();

  auto get_instance_layer_properties();
  auto get_instance_extension_properties();
  auto get_physical_devices();
  auto get_device_layers_properties();

public:
  ~my_vulkan();
  static std::shared_ptr<my_vulkan> create(
    const char* app_name, const char* engine_name){
    return std::shared_ptr<my_vulkan>{new my_vulkan{ app_name, engine_name }};
  }
  void load_image(double** img, double** mask, uint32_t width, uint32_t height,
    double variables[num_variables_]);
  void unload_image();

  void run(unsigned steps);

  auto result_image(){ return result_image_; };

};
