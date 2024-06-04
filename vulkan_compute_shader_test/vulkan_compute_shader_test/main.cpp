#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <vec4.hpp>
#include <mat4x4.hpp>

#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>


std::vector<char> readFile(const std::string& filepath) {
    std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filepath);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

int main() {

    // Create vulkan instance
    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = nullptr;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = nullptr;
    applicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    const char* validationLayer = "VK_LAYER_KHRONOS_validation" ;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = 1;
    instanceCreateInfo.ppEnabledLayerNames = &validationLayer;

    VkInstance instance;
    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create instance");
    }

    // Get physical device info
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("RUNTIME ERROR: Failed to find physical device");
    }
    std::cout << "Physical device count: " << deviceCount << std::endl << std::endl;
    std::vector<VkPhysicalDevice> physicalDeviceList(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDeviceList.data());

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDevice physicalDevice;

    int deviceNumber = 0;
    std::cout << "Available devices: " << std::endl;
    for (const auto& device : physicalDeviceList) {
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << "   Device(" << deviceNumber << "): " << deviceProperties.deviceName << std::endl;
    }

    int selectedDevice = -1;
    if (deviceCount > 1) {
        std::cout << std::endl;
        while (selectedDevice < 0 || selectedDevice >= deviceCount) {
            std::cout << ">> Select device (0-" << deviceCount - 1 << "): ";
            std::cin >> selectedDevice;
            if (!std::cin.good()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Must enter valid number" << std::endl;
                selectedDevice = -1;
            }
            else if (selectedDevice < 0 || selectedDevice >= deviceCount) {
                std::cout << "Must enter valid number" << std::endl;
            }
        }
    }
    else {
        selectedDevice = 0;
    }
    std::cout << std::endl;

    physicalDevice = physicalDeviceList[selectedDevice];
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("RUNTIME ERROR: Invalid device (VK_NULL_HANDLE)");
    }

    // Print physical device info
    std::cout << deviceProperties.deviceName << " (Device "<< selectedDevice <<") selected." << std::endl;
    std::cout << "    Vulkan version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion) <<
        "." << VK_VERSION_MINOR(deviceProperties.apiVersion) <<
        "." << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
    std::cout << "    Max compute shared memory size: " << deviceProperties.limits.maxComputeSharedMemorySize / 1024 << "KB" << std::endl << std::endl;

    // Get compute queue index
    uint32_t queueFamilyPropCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropCount, nullptr);

    std::cout << "Queue family prop count: " << queueFamilyPropCount << std::endl;
    std::vector<VkQueueFamilyProperties> queueFamilyPropVec(queueFamilyPropCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropCount, queueFamilyPropVec.data());

    uint32_t computeQueueIndex = 0;
    for (const auto& prop : queueFamilyPropVec) {
        if (prop.queueCount > 0 && prop.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            break;
        }
        ++computeQueueIndex;
    }

    std::cout << "Compute queue family index: " << computeQueueIndex << std::endl << std::endl;

    // Create vulkan device
    VkDevice vulkanDevice = {};
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfoVec;

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueFamilyIndex = computeQueueIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
    deviceQueueCreateInfoVec.push_back(deviceQueueCreateInfo);

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfoVec.data();
    deviceCreateInfo.enabledLayerCount = 0;

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &vulkanDevice) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create vulkan device");
    }

    // Create input/output buffers
    const uint32_t elements = 10;
    const uint32_t bufferSize = elements * sizeof(uint32_t);

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.size = bufferSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = &computeQueueIndex;

    VkBuffer inBuffer;
    VkBuffer outBuffer;

    if (vkCreateBuffer(vulkanDevice, &bufferCreateInfo, nullptr, &inBuffer) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create input buffer");
    }
    if (vkCreateBuffer(vulkanDevice, &bufferCreateInfo, nullptr, &outBuffer) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create output buffer");
    }

    // Allocate input/output buffer memory
    VkMemoryRequirements inBufferMemoryReq;
    VkMemoryRequirements outBufferMemoryReq;
    vkGetBufferMemoryRequirements(vulkanDevice, inBuffer, &inBufferMemoryReq);
    vkGetBufferMemoryRequirements(vulkanDevice, inBuffer, &outBufferMemoryReq);

    VkPhysicalDeviceMemoryProperties physicalDeviceMemProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemProps);

    uint32_t memoryTypeIndex = uint32_t(~0);
    VkDeviceSize memoryHeapSize = uint32_t(~0);
    for (uint32_t i = 0; i < physicalDeviceMemProps.memoryTypeCount; ++i) {
        VkMemoryType memoryType = physicalDeviceMemProps.memoryTypes[i];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memoryTypeIndex = i;
            memoryHeapSize = physicalDeviceMemProps.memoryHeaps[i].size;
            break;
        }
    }

    std::cout << "Memory type index: " << memoryTypeIndex << std::endl;
    std::cout << "Memory heap size: " << memoryHeapSize << std::endl << std::endl;


    VkMemoryAllocateInfo inBufferMemoryAllocateInfo = {};
    inBufferMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    inBufferMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
    inBufferMemoryAllocateInfo.allocationSize = inBufferMemoryReq.size;

    VkMemoryAllocateInfo outBufferMemoryAllocateInfo = {};
    outBufferMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    outBufferMemoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
    outBufferMemoryAllocateInfo.allocationSize = outBufferMemoryReq.size;

    VkDeviceMemory inBufferMemory;
    VkDeviceMemory outBufferMemory;

    if (vkAllocateMemory(vulkanDevice, &inBufferMemoryAllocateInfo, nullptr, &inBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to allocate input buffer memory");
    }
    if (vkAllocateMemory(vulkanDevice, &outBufferMemoryAllocateInfo, nullptr, &outBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to allocate output buffer memory");
    }

    // Map data to input buffer
    std::vector<uint32_t> dataVec(elements);

    for (uint32_t i = 0; i < elements; ++i) {
        dataVec[i] = i;
    }

    void* data;
    vkMapMemory(vulkanDevice, inBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, dataVec.data(), bufferSize);
    vkUnmapMemory(vulkanDevice, inBufferMemory);

    if (vkBindBufferMemory(vulkanDevice, inBuffer, inBufferMemory, 0) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to bind input buffer");
    }
    if (vkBindBufferMemory(vulkanDevice, outBuffer, outBufferMemory, 0) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to bind output buffer");
    }

    // Create shader module
    auto compShader = readFile("compute_shader.comp.spv");

    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = compShader.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(compShader.data());

    VkShaderModule compShaderModule;
    if (vkCreateShaderModule(vulkanDevice, &shaderModuleCreateInfo, nullptr, &compShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create shader module");
    }


    // Create descriptor set layout
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2];

    for (uint32_t i = 0; i < 2; i++) {
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding = i;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
        descriptorSetLayoutBindings[i] = binding;
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = 2;
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(vulkanDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create descriptor set layout");
    }
    
    // Create compute pipeline
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    
    VkPipelineLayout pipelineLayout;
    if(vkCreatePipelineLayout(vulkanDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create pipeline layout");
    }
    
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkPipelineCache pipelineCache;
    if (vkCreatePipelineCache(vulkanDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache)) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create pipeline cache");
    }
    
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
    pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfo.pName = "main";
    pipelineShaderStageCreateInfo.module = compShaderModule;
    pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    VkComputePipelineCreateInfo computePipelineCreateInfo = {};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = pipelineLayout;
    computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;

    VkPipeline computePipeline;
    if (vkCreateComputePipelines(vulkanDevice, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &computePipeline)) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create compute pipeline");
    }
    
    // Create descriptor pool and allocate descriptor set
    VkDescriptorPoolSize descriptorPoolSize = {};
    descriptorPoolSize.descriptorCount = 2;
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.maxSets = 1;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(vulkanDevice, &descriptorPoolCreateInfo, nullptr, &descriptorPool)) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create descriptor pool");
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    std::vector<VkDescriptorSet> descriptorSetVec(1);
    if (vkAllocateDescriptorSets(vulkanDevice, &descriptorSetAllocateInfo, descriptorSetVec.data()) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to allocate descriptor sets");
    }

    VkDescriptorSet descriptorSet = descriptorSetVec.front();

    VkDescriptorBufferInfo inBufferInfo = {};
    inBufferInfo.buffer = inBuffer;
    inBufferInfo.offset = 0;
    inBufferInfo.range = elements * sizeof(uint32_t);

    VkDescriptorBufferInfo outBufferInfo = {};
    outBufferInfo.buffer = outBuffer;
    outBufferInfo.offset = 0;
    outBufferInfo.range = elements * sizeof(uint32_t);
    
    VkWriteDescriptorSet writeInBufferDescriptorSet = {};
    writeInBufferDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInBufferDescriptorSet.dstBinding = 0;
    writeInBufferDescriptorSet.dstArrayElement = 0;
    writeInBufferDescriptorSet.descriptorCount = 1;
    writeInBufferDescriptorSet.dstSet = descriptorSet;
    writeInBufferDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeInBufferDescriptorSet.pBufferInfo = &inBufferInfo;

    VkWriteDescriptorSet writeOutBufferDescriptorSet = {};
    writeOutBufferDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeOutBufferDescriptorSet.dstBinding = 1;
    writeOutBufferDescriptorSet.dstArrayElement = 0;
    writeOutBufferDescriptorSet.descriptorCount = 1;
    writeOutBufferDescriptorSet.dstSet = descriptorSet;
    writeOutBufferDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeOutBufferDescriptorSet.pBufferInfo = &outBufferInfo;

    const std::vector<VkWriteDescriptorSet> writeDescriptorSetVec = { writeInBufferDescriptorSet, writeOutBufferDescriptorSet };

    vkUpdateDescriptorSets(vulkanDevice, 2, writeDescriptorSetVec.data(), 0, nullptr);

    // Create command pool
    VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
    cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex = computeQueueIndex;

    VkCommandPool commandPool;
    if (vkCreateCommandPool(vulkanDevice, &cmdPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create command pool");
    }

    // Allocate command buffers
    uint32_t cmdBufferCount = 1;
    VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
    cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocateInfo.commandPool = commandPool;
    cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocateInfo.commandBufferCount = cmdBufferCount;

    std::vector<VkCommandBuffer> cmdBufferVec(cmdBufferCount);
    if (vkAllocateCommandBuffers(vulkanDevice, &cmdBufferAllocateInfo, cmdBufferVec.data()) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to allocate command buffers");
    }
    VkCommandBuffer commandBuffer = cmdBufferVec.front();

    // Dispatch commands
    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to begin command buffer");
    }
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDispatch(commandBuffer, elements, 1, 1);
    vkEndCommandBuffer(commandBuffer);

    VkQueue queue;
    vkGetDeviceQueue(vulkanDevice, computeQueueIndex, 0, &queue);

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence;
    if (vkCreateFence(vulkanDevice, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create fence");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed submit command buffer to queue");
    }

    vkWaitForFences(vulkanDevice, 1, &fence, true, UINT64_MAX);

    // Display results
    std::vector<uint32_t> dataInVec(elements);
    void* dataIn;
    vkMapMemory(vulkanDevice, inBufferMemory, 0, bufferSize, 0, &dataIn);
    memcpy(dataInVec.data(), dataIn, bufferSize);
    vkUnmapMemory(vulkanDevice, inBufferMemory);

    std::cout << "Input buffer: ";
    for (uint32_t i = 0; i < elements; ++i) {
        std::cout << dataInVec[i] << " ";
    }
    std::cout << std::endl;

    std::vector<uint32_t> dataOutVec(elements);
    void* dataOut;
    vkMapMemory(vulkanDevice, outBufferMemory, 0, bufferSize, 0, &dataOut);
    memcpy(dataOutVec.data(), dataOut, bufferSize);
    vkUnmapMemory(vulkanDevice, outBufferMemory);

    std::cout << "Output buffer: ";
    for (uint32_t i = 0; i < elements; ++i) {
        std::cout << dataOutVec[i] << " ";
    }
    std::cout << std::endl;

    // Cleanup
    vkResetCommandPool(vulkanDevice, commandPool, 0);
    vkDestroyFence(vulkanDevice, fence, nullptr);
    vkDestroyPipeline(vulkanDevice, computePipeline, nullptr);
    vkDestroyPipelineCache(vulkanDevice, pipelineCache, nullptr);
    vkDestroyPipelineLayout(vulkanDevice, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(vulkanDevice, descriptorSetLayout, nullptr);
    vkDestroyShaderModule(vulkanDevice, compShaderModule, nullptr);
    vkDestroyDescriptorPool(vulkanDevice, descriptorPool, nullptr);
    vkDestroyCommandPool(vulkanDevice, commandPool, nullptr);
    vkFreeMemory(vulkanDevice, inBufferMemory, nullptr);
    vkFreeMemory(vulkanDevice, outBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice, inBuffer, nullptr);
    vkDestroyBuffer(vulkanDevice, outBuffer, nullptr);
    vkDestroyDevice(vulkanDevice, nullptr);
    vkDestroyInstance(instance, nullptr);

    return EXIT_SUCCESS;
}