#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <vec4.hpp>
#include <mat4x4.hpp>

#include <iostream>
#include <vector>
#include <cassert>


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
        if (prop.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            break;
        }
        ++computeQueueIndex;
    }

    std::cout << "Compute queue family index: " << computeQueueIndex << std::endl << std::endl;

    vkDestroyInstance(instance, nullptr);
    
    return EXIT_SUCCESS;
}