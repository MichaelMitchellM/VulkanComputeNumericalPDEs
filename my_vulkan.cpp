
#include "my_vulkan.hpp"

// alloc functions
VKAPI_ATTR void* VKAPI_CALL myalloc(void* p_user_data, size_t size, size_t alignment, VkSystemAllocationScope alloc_scope){
  return aligned_alloc(alignment, size);
}
VKAPI_ATTR void  VKAPI_CALL myfree(void* p_user_data, void* p_memory){
  free(p_memory);
}
VKAPI_ATTR void* VKAPI_CALL myrealloc(
  void* p_user_data, void* p_original, size_t size, size_t alignement,
  VkSystemAllocationScope alloc_scope){
  return realloc(p_original, size);
}

auto my_vulkan::get_instance_layer_properties(){
  uint32_t num_properties;
  std::vector<VkLayerProperties> res;
  auto err = vkEnumerateInstanceLayerProperties(&num_properties, nullptr);
  printf("vkEnumerateInstanceLayerProperties result %i\n", err);
  assert(!err);
  printf("number of instance layer properties: %u\n", num_properties);
  if(num_properties > 0u){
    auto instance_layers = new VkLayerProperties[num_properties];
    err = vkEnumerateInstanceLayerProperties(&num_properties, instance_layers);
    printf("vkEnumerateInstanceLayerProperties result %i\n", err);
    assert(!err);
    for(auto i = 0u; i < num_properties; ++i) res.push_back(instance_layers[i]);
    delete[] instance_layers;
  }
  return res;
}
auto my_vulkan::get_instance_extension_properties(){
  uint32_t num_properties;
  std::vector<VkExtensionProperties> res;
  auto err = vkEnumerateInstanceExtensionProperties(nullptr, &num_properties, nullptr);
  printf("vkEnumerateInstanceExtensionProperties result %i\n", err);
  assert(!err);
  if(num_properties > 0u){
    auto instance_extensions = new VkExtensionProperties[num_properties];
    err = vkEnumerateInstanceExtensionProperties(nullptr, &num_properties, instance_extensions);
    printf("vkEnumerateInstanceExtensionProperties result %i\n", err);
    assert(!err);
    for(auto i = 0u; i < num_properties; ++i) res.push_back(instance_extensions[i]);
    delete[] instance_extensions;
  }
  return res;
}
auto my_vulkan::get_physical_devices(){
  uint32_t num_devices;
  std::vector<VkPhysicalDevice> res;
  auto err = vkEnumeratePhysicalDevices(inst_, &num_devices, nullptr);
  printf("vkEnumeratePhysicalDevices result %i\n", err);
  assert(!err);
  if(num_devices > 0u){
    auto physical_devices = new VkPhysicalDevice[num_devices];
    err = vkEnumeratePhysicalDevices(inst_, &num_devices, physical_devices);
    printf("vkEnumeratePhysicalDevices result %i\n", err);
    assert(!err);
    for(auto i = 0u; i < num_devices; ++i) res.push_back(physical_devices[i]);
    delete[] physical_devices;
  }
  return res;
}
auto my_vulkan::get_device_layers_properties(){
  uint32_t num_layers;
  std::vector<VkLayerProperties> res;
  auto err = vkEnumerateDeviceLayerProperties(my_gpu_, &num_layers, nullptr);
  printf("vkEnumerateDeviceLayerProperties result %i\n", err);
  assert(!err);
  if(num_layers > 0u){
    auto device_layers = new VkLayerProperties[num_layers];
    err = vkEnumerateDeviceLayerProperties(my_gpu_, &num_layers, device_layers);
    printf("vkEnumerateDeviceLayerProperties result %i\n", err);
    assert(!err);
    for(auto i = 0u; i < num_layers; ++i) res.push_back(device_layers[i]);
    delete[] device_layers;
  }
  return res;
}

my_vulkan::my_vulkan(const char* app_name, const char* engine_name)
  :
    app_name_(app_name),
    engine_name_(engine_name),
    init_success_(false),
    image_loaded_(false),
    result_image_(nullptr)
{
  alloc_cb_.pfnAllocation = myalloc;
  alloc_cb_.pfnFree = myfree;
  alloc_cb_.pfnReallocation = myrealloc;

  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = nullptr,
    .pApplicationName   = app_name,
    .applicationVersion = 0u,
    .pEngineName        = engine_name,
    .engineVersion      = 0,
    .apiVersion         = VK_API_VERSION_1_0
  };

  // instance info
  /*
  auto test = get_instance_layer_properties();
  for(auto& t : test){
    printf("%s\n", t.layerName);
  }
  */

  auto num_enabled_instance_layers = 0u;
  const char* instance_validation_layers[] = {
    //"VK_LAYER_LUNARG_standard_validation",
  };

  auto num_enabled_extensions = 0u;
  const char* instance_extensions[] = {
    //""
  };

  VkInstanceCreateInfo inst_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = nullptr,
    .pApplicationInfo  = &app_info,
    .enabledLayerCount = num_enabled_instance_layers,
    .ppEnabledLayerNames     = instance_validation_layers,
    .enabledExtensionCount   = num_enabled_extensions,
    .ppEnabledExtensionNames = instance_extensions
  };

  auto err = vkCreateInstance(&inst_info, &alloc_cb_, &inst_);
  printf("vkCreateInstance result : %i\n", err);
  assert(!err);

  printf("number of devices: %u\n", get_physical_devices().size());
  my_gpu_ = get_physical_devices()[0];


  // Did this to get the values for my gpu
  // could make dynamic, so it finds the proper thing for whatever system its on
  //auto q_family_prop_count = 0u;
  //vkGetPhysicalDeviceQueueFamilyProperties(my_gpu_, &q_family_prop_count, 0);
  //VkQueueFamilyProperties* q_family_props = new VkQueueFamilyProperties[q_family_prop_count];
  //vkGetPhysicalDeviceQueueFamilyProperties(my_gpu_, &q_family_prop_count, q_family_props);

  queue_family_index_ = 0u; // values gotten for my gpu

  // https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#devsandqueues-queue-creation
  float q_priorities[1] = { 1.0f };
  VkDeviceQueueCreateInfo device_queue_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .queueFamilyIndex = queue_family_index_,
    .queueCount       = 1u, // we want 1 queue
    .pQueuePriorities = q_priorities
  };

  // https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#devsandqueues-device-creation
  VkDeviceCreateInfo device_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .queueCreateInfoCount    = 1u,
    .pQueueCreateInfos       = &device_queue_info,
    .enabledLayerCount       = 0u,
    .ppEnabledLayerNames     = nullptr,
    .enabledExtensionCount   = 0u,
    .ppEnabledExtensionNames = nullptr,
    .pEnabledFeatures        = nullptr
  };

  err = vkCreateDevice(my_gpu_, &device_info, &alloc_cb_, &my_device_);
  printf("vkCreateDevice result: %i\n", err);
  assert(!err);

  // https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#devsandqueues-queue-creation

  // https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#memory-device
  VkPhysicalDeviceMemoryProperties device_mem_props;
  vkGetPhysicalDeviceMemoryProperties(my_gpu_, &device_mem_props);

  for(auto i = 0u; i < device_mem_props.memoryTypeCount; ++i){
    if((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & device_mem_props.memoryTypes[i].propertyFlags)
      && (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & device_mem_props.memoryTypes[i].propertyFlags))
      {
        printf("%u : %lu\n", i, device_mem_props.memoryHeaps[device_mem_props.memoryTypes[i].heapIndex].size);
      }
  }

  // for my gpu it seems like heap index 3 is what we need.
  heap_index_ = 3u;
  init_success_ = true;
}

my_vulkan::~my_vulkan(){
  if(init_success_){
    unload_image();
    vkDestroyDevice(my_device_, &alloc_cb_);
    vkDestroyInstance(inst_, &alloc_cb_);
  }
}

void my_vulkan::load_image(double** img, double** mask, uint32_t width, uint32_t height,
  double variables[num_variables_])
{
  width_  = width;
  height_ = height;
  result_image_ = new double*[width];
  for(auto i = 0u; i < width_; ++i){
    result_image_[i] = new double[height];
  }
  // max my gpu can take with out adjusting the program
  assert(width * height <= 2147483647u);
  // original image, mask, new image, hence times 3
  auto bytes_per_image = sizeof(double) * width * height;
  auto bytes_for_variables = num_variables_ * sizeof(double);
  memory_to_alloc_ = 3u * bytes_per_image + bytes_for_variables;

  printf("memory_to_alloc %u bytes\n", memory_to_alloc_);

  VkMemoryAllocateInfo mem_alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = nullptr,
    .allocationSize  = memory_to_alloc_,
    .memoryTypeIndex = heap_index_
  };

  auto err = vkAllocateMemory(my_device_, &mem_alloc_info, &alloc_cb_, &my_memory_);
  printf("vkAllocateMemory result: %i\n", err);
  assert(!err);

  // load images
  double* transfer;
  err = vkMapMemory(my_device_, my_memory_, 0, VK_WHOLE_SIZE, 0, (void**)&transfer);
  printf("vkMapMemory result: %i\n", err);
  assert(!err);

  // image
  for(auto i = 0u; i < width; ++i){
    for(auto j = 0u; j < height; ++j){
      transfer[j * width + i] = img[i][j];
    }
  }

  // mask
  auto offset = bytes_per_image / sizeof(double);
  for(auto i = 0u; i < width; ++i){
    for(auto j = 0u; j < height; ++j){
      transfer[offset + j * width + i] = mask[i][j];
    }
  }

  offset = 3u * bytes_per_image / sizeof(double);
  for(auto i = 0u; i < num_variables_; ++i){
    transfer[offset + i] = variables[i];
  }

  vkUnmapMemory(my_device_, my_memory_);

  // https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#resources-buffers
  unsigned queueFamilyIndex = 0u;
  VkBufferCreateInfo buffer_create_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size  = bytes_per_image,
    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 1u,
    .pQueueFamilyIndices   = &queueFamilyIndex
  };

  VkBufferCreateInfo variables_buffer_create_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size  = bytes_for_variables,
    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 1u,
    .pQueueFamilyIndices   = &queueFamilyIndex
  };

  err = vkCreateBuffer(my_device_, &buffer_create_info, &alloc_cb_, &buffer_0_);
  printf("vkCreateBuffer result %i\n", err);
  assert(!err);

  err = vkCreateBuffer(my_device_, &buffer_create_info, &alloc_cb_, &buffer_1_);
  printf("vkCreateBuffer result %i\n", err);
  assert(!err);

  err = vkCreateBuffer(my_device_, &buffer_create_info, &alloc_cb_, &buffer_2_);
  printf("vkCreateBuffer result %i\n", err);
  assert(!err);

  err = vkCreateBuffer(my_device_, &variables_buffer_create_info, &alloc_cb_, &buffer_variables_);
  printf("vkCreateBuffer result %i\n", err);
  assert(!err);

  err = vkBindBufferMemory(my_device_, buffer_0_, my_memory_, 0);
  printf("vkBindBufferMemory result %i\n", err);
  assert(!err);

  err = vkBindBufferMemory(my_device_, buffer_1_, my_memory_, bytes_per_image);
  printf("vkBindBufferMemory result %i\n", err);
  assert(!err);

  err = vkBindBufferMemory(my_device_, buffer_2_, my_memory_, 2u * bytes_per_image);
  printf("vkBindBufferMemory result %i\n", err);
  assert(!err);

  err = vkBindBufferMemory(my_device_, buffer_variables_, my_memory_, 3u * bytes_per_image);
  printf("vkBindBufferMemory result %i\n", err);
  assert(!err);

  load_shader();

  load_desc();

  load_pipeline();

  load_command_pool();

  image_loaded_ = true;
}

void my_vulkan::unload_image(){
  if(image_loaded_){
    for(auto i = 0u; i < width_; ++i){
      delete result_image_[i];
    }
    delete result_image_;
    vkDestroyBuffer(my_device_, buffer_0_, &alloc_cb_);
    vkDestroyBuffer(my_device_, buffer_1_, &alloc_cb_);
    vkDestroyBuffer(my_device_, buffer_2_, &alloc_cb_);
    vkDestroyBuffer(my_device_, buffer_variables_, &alloc_cb_);
    vkFreeMemory(my_device_, my_memory_, &alloc_cb_);
    image_loaded_ = false;
  }
}

void my_vulkan::load_shader(){

  size_t size;

  FILE* fp = fopen("comp.spv", "rb");
  assert(fp); // make sure we got the file

  fseek(fp, 0l, SEEK_END);
  size = ftell(fp); // get the size of the file
  assert(size > 0); // make sure the size is not 0

  fseek(fp, 0l, SEEK_SET); // go back to start

  uint8_t* shader_code = new uint8_t[size];
  size_t retval = fread(shader_code, size, 1, fp);
  // since the size is the whole file,
  // retval should be one since we read the whole file as a single chunk.
  assert(retval == 1);

  fclose(fp);

  VkShaderModuleCreateInfo shader_mod_create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .codeSize = size,
    .pCode    = (uint32_t*)shader_code
  };

  auto err = vkCreateShaderModule(my_device_, &shader_mod_create_info, &alloc_cb_, &shader_module_);
  printf("vkCreateShaderModule result %i\n", err);
  assert(!err);

}

void my_vulkan::load_desc(){
  VkDescriptorSetLayoutBinding desc_set_layout_bindings[num_buffers_] = {
    {
      .binding = 0u,
      .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount    = 1u,
      .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = nullptr
    },
    {
      .binding = 1u,
      .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount    = 1u,
      .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = nullptr
    },
    {
      .binding = 2u,
      .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount    = 1u,
      .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = nullptr
    },
    {
      .binding = 3u,
      .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount    = 1u,
      .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = nullptr
    }
  };

  VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .bindingCount = num_buffers_,
    .pBindings    = desc_set_layout_bindings
  };

  auto err = vkCreateDescriptorSetLayout(my_device_, &desc_set_layout_create_info, &alloc_cb_, &desc_set_layout_);
  printf("vkCreateDescriptorSetLayout result %i\n", err);
  assert(!err);

  VkDescriptorPoolSize desc_pool_size = {
      .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = num_buffers_
  };

  VkDescriptorPoolCreateInfo desc_pool_create_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags =  0,
    .maxSets = 1u,
    .poolSizeCount = 1u,
    .pPoolSizes    = &desc_pool_size
  };

  err = vkCreateDescriptorPool(my_device_, &desc_pool_create_info, &alloc_cb_, &desc_pool_);
  printf("vkCreateDescriptorPool result %i\n", err);
  assert(!err);

  VkDescriptorSetAllocateInfo desc_set_alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = nullptr,
    .descriptorPool = desc_pool_,
    .descriptorSetCount = 1u,
    .pSetLayouts = &desc_set_layout_
  };

  err = vkAllocateDescriptorSets(my_device_, &desc_set_alloc_info, &desc_set_);
  printf("vkAllocateDescriptorSets result %i\n", err);
  assert(!err);

  VkDescriptorBufferInfo desc_buffer_info_0 = {
    .buffer = buffer_0_,
    .offset = 0,
    .range  = VK_WHOLE_SIZE
  };

  VkDescriptorBufferInfo desc_buffer_info_1 = {
    .buffer = buffer_1_,
    .offset = 0,
    .range  = VK_WHOLE_SIZE
  };

  VkDescriptorBufferInfo desc_buffer_info_2 = {
    .buffer = buffer_2_,
    .offset = 0,
    .range  = VK_WHOLE_SIZE
  };

  VkDescriptorBufferInfo desc_buffer_info_variables = {
    .buffer = buffer_variables_,
    .offset = 0,
    .range  = VK_WHOLE_SIZE
  };

  VkWriteDescriptorSet write_desc_set[num_buffers_] = {
    {
      .sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext  = nullptr,
      .dstSet = desc_set_,
      .dstBinding = 0u,
      .dstArrayElement = 0u,
      .descriptorCount = 1u,
      .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pImageInfo  = nullptr,
      .pBufferInfo = &desc_buffer_info_0,
      .pTexelBufferView = nullptr
    },
    {
      .sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext  = nullptr,
      .dstSet = desc_set_,
      .dstBinding = 1u,
      .dstArrayElement = 0u,
      .descriptorCount = 1u,
      .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pImageInfo  = nullptr,
      .pBufferInfo = &desc_buffer_info_1,
      .pTexelBufferView = nullptr
    },
    {
      .sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext  = nullptr,
      .dstSet = desc_set_,
      .dstBinding = 2u,
      .dstArrayElement = 0u,
      .descriptorCount = 1u,
      .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pImageInfo  = nullptr,
      .pBufferInfo = &desc_buffer_info_2,
      .pTexelBufferView = nullptr
    },
    {
      .sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext  = nullptr,
      .dstSet = desc_set_,
      .dstBinding = 3u,
      .dstArrayElement = 0u,
      .descriptorCount = 1u,
      .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pImageInfo  = nullptr,
      .pBufferInfo = &desc_buffer_info_variables,
      .pTexelBufferView = nullptr
    }
  };

  vkUpdateDescriptorSets(my_device_, num_buffers_, write_desc_set, 0u, nullptr);
}

void my_vulkan::load_pipeline(){

  VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .setLayoutCount = 1u,
    .pSetLayouts    = &desc_set_layout_,
    .pushConstantRangeCount = 0u,
    .pPushConstantRanges    = nullptr
  };

  auto err = vkCreatePipelineLayout(my_device_, &pipeline_layout_create_info, &alloc_cb_, &pipeline_layout_);
  printf("vkCreatePipelineLayout result %i\n", err);
  assert(!err);

  VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext  = nullptr,
    .flags  = 0,
    .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
    .module = shader_module_,
    .pName  = "main",
    .pSpecializationInfo = nullptr
  };

  VkComputePipelineCreateInfo comp_pipeline_create_info = {
    .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .pNext  = nullptr,
    .flags  = 0,
    .stage  = pipeline_shader_stage_create_info,
    .layout = pipeline_layout_,
    .basePipelineHandle = 0,
    .basePipelineIndex  = 0
  };

  err = vkCreateComputePipelines(my_device_, VK_NULL_HANDLE, 1u, &comp_pipeline_create_info, &alloc_cb_, &pipeline_);
  printf("vkCreateComputePipelines result %i\n", err);
  assert(!err);
}

void my_vulkan::load_command_pool(){
  VkCommandPoolCreateInfo command_pool_create_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .queueFamilyIndex = queue_family_index_
  };

  auto err = vkCreateCommandPool(my_device_, &command_pool_create_info, &alloc_cb_, &command_pool_);
  printf("vkCreateCommandPool result %i\n", err);
  assert(!err);

  VkCommandBufferAllocateInfo command_buffer_alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = nullptr,
    .commandPool        = command_pool_,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1u
  };

  err = vkAllocateCommandBuffers(my_device_, &command_buffer_alloc_info, &command_buffer_);
  printf("vkAllocateCommandBuffers result %i\n", err);
  assert(!err);

  VkCommandBufferBeginInfo command_buffer_begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = nullptr,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = nullptr
  };

  err = vkBeginCommandBuffer(command_buffer_, &command_buffer_begin_info);
  printf("vkBeginCommandBuffer result %i\n", err);
  assert(!err);

  vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);

  vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipeline_layout_, 0u, 1u, &desc_set_, 0u, nullptr);

  // my gpu can take a work group #0 size of 0x7fffffff = 2147483647
  // it can have 1 per pixel of a 46340 x 46350 image!
  vkCmdDispatch(command_buffer_, width_, height_, 1u);

  err = vkEndCommandBuffer(command_buffer_);
  printf("vkEndCommandBuffer result %i\n", err);
  assert(!err);
}

void my_vulkan::run(unsigned steps){
  vkGetDeviceQueue(my_device_, queue_family_index_, 0u, &my_queue_);

  VkResult err;

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = nullptr,
    .waitSemaphoreCount = 0u,
    .pWaitSemaphores    = nullptr,
    .pWaitDstStageMask  = nullptr,
    .commandBufferCount = 1u,
    .pCommandBuffers    = &command_buffer_,
    .signalSemaphoreCount = 0u,
    .pSignalSemaphores    = nullptr
  };

  // SUSPECT
  // for large step sizes my gpu will become unresponsive.
  for(auto n = 0u; n < steps; ++n){
    err = vkQueueSubmit(my_queue_, 1u, &submit_info, VK_NULL_HANDLE);
    //printf("vkQueueSubmit result %i\n", err);
    assert(!err);

    err = vkQueueWaitIdle(my_queue_);
    //printf("vkQueueWaitIdle result %i\n", err);
    assert(!err);
  }

  double* transfer;
  err = vkMapMemory(my_device_, my_memory_, 0, memory_to_alloc_, 0, (void**)&transfer);
  printf("vkMapMemory result %i\n", err);
  assert(!err);

  auto offset = 0u; //2u * width_ * height_;
  for(auto i = 0u; i < width_; ++i){
    for(auto j = 0u; j < height_; ++j){
      auto index = j * width_ + i + offset;
      result_image_[i][j] = transfer[index];
    }
  }

  vkUnmapMemory(my_device_, my_memory_);
}
