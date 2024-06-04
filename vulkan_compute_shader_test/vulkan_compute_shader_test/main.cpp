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
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

    for (uint32_t i = 0; i < 1; ++i) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = i;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
        descriptorSetLayoutBindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = descriptorSetLayoutBindings.size();
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(vulkanDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("RUNTIME ERROR: Failed to create descriptor set layout");
    }

    // Cleanup
    vkDestroyDescriptorSetLayout(vulkanDevice, descriptorSetLayout, nullptr);
    vkDestroyShaderModule(vulkanDevice, compShaderModule, nullptr);
    vkFreeMemory(vulkanDevice, inBufferMemory, nullptr);
    vkFreeMemory(vulkanDevice, outBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice, inBuffer, nullptr);
    vkDestroyBuffer(vulkanDevice, outBuffer, nullptr);
    vkDestroyDevice(vulkanDevice, nullptr);
    vkDestroyInstance(instance, nullptr);

    return EXIT_SUCCESS;
}