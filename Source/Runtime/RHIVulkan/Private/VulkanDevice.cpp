// ToyEngine RHIVulkan Module
// Vulkan Device、Swapchain、资源与帧同步阶段 B 实现

#include "VulkanDevice.h"

#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptors.h"
#include "VulkanPipeline.h"
#include "VulkanSampler.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "RHI.h"
#include "Log/Log.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <cstddef>
#include <limits>
#include <optional>
#include <set>
#include <string>

namespace TE {

namespace {

constexpr uint32_t RequiredVulkanVersion = VK_API_VERSION_1_3;
constexpr const char* ValidationLayerName = "VK_LAYER_KHRONOS_validation";

struct FQueueFamilies
{
    std::optional<uint32_t> Graphics;
    std::optional<uint32_t> Present;

    [[nodiscard]] bool IsComplete() const { return Graphics.has_value() && Present.has_value(); }
};

struct FSwapchainSupport
{
    VkSurfaceCapabilitiesKHR Capabilities{};
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT type,
                                             const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                             void* userData)
{
    (void)type;
    (void)userData;
    const char* const message = callbackData && callbackData->pMessage
        ? callbackData->pMessage
        : "Unknown validation message";
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        TE_LOG_ERROR("[Vulkan Validation] {}", message);
    }
    else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        TE_LOG_WARN("[Vulkan Validation] {}", message);
    }
    else
    {
        TE_LOG_DEBUG("[Vulkan Validation] {}", message);
    }
    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT MakeDebugMessengerCreateInfo()
{
    return {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = DebugCallback,
    };
}

bool HasValidationLayer()
{
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    return std::any_of(layers.begin(), layers.end(), [](const VkLayerProperties& layer)
    {
        return std::strcmp(layer.layerName, ValidationLayerName) == 0;
    });
}

FQueueFamilies FindQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

    FQueueFamilies result;
    for (uint32_t index = 0; index < count; ++index)
    {
        if ((properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            result.Graphics = index;
        }

        VkBool32 presentSupported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupported);
        if (presentSupported == VK_TRUE)
        {
            result.Present = index;
        }
        if (result.IsComplete())
        {
            break;
        }
    }
    return result;
}

bool HasRequiredDeviceExtensions(const VkPhysicalDevice device)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> properties(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, properties.data());
    return std::any_of(properties.begin(), properties.end(), [](const VkExtensionProperties& extension)
    {
        return std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
    });
}

FSwapchainSupport QuerySwapchainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface)
{
    FSwapchainSupport support;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support.Capabilities);

    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
    support.Formats.resize(count);
    if (count > 0)
    {
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, support.Formats.data());
    }

    count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
    support.PresentModes.resize(count);
    if (count > 0)
    {
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, support.PresentModes.data());
    }
    return support;
}

bool SupportsDynamicRendering(const VkPhysicalDevice device)
{
    VkPhysicalDeviceVulkan13Features vulkan13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    };
    VkPhysicalDeviceFeatures2 features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vulkan13,
    };
    vkGetPhysicalDeviceFeatures2(device, &features);
    return vulkan13.dynamicRendering == VK_TRUE;
}

VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    const std::array preferred = {
        VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
        VkSurfaceFormatKHR{VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
        VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
    };
    for (const auto& requested : preferred)
    {
        const auto found = std::find_if(formats.begin(), formats.end(), [&requested](const VkSurfaceFormatKHR& format)
        {
            return format.format == requested.format && format.colorSpace == requested.colorSpace;
        });
        if (found != formats.end())
        {
            return *found;
        }
    }
    return formats.front();
}

VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes, const bool vsync)
{
    if (vsync)
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != modes.end())
    {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }
    if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != modes.end())
    {
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkCompositeAlphaFlagBitsKHR ChooseCompositeAlpha(const VkCompositeAlphaFlagsKHR supported)
{
    constexpr std::array candidates = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (const auto candidate : candidates)
    {
        if ((supported & candidate) != 0)
        {
            return candidate;
        }
    }
    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

RHIFormat ToRHIFormat(const VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_B8G8R8A8_UNORM: return RHIFormat::BGRA8_UNorm;
    case VK_FORMAT_B8G8R8A8_SRGB: return RHIFormat::BGRA8_sRGB;
    case VK_FORMAT_R8G8B8A8_UNORM: return RHIFormat::RGBA8_UNorm;
    case VK_FORMAT_R8G8B8A8_SRGB: return RHIFormat::RGBA8_sRGB;
    default: return RHIFormat::Undefined;
    }
}

RHIFrameStatus ToFrameStatus(const VkResult result)
{
    if (result == VK_ERROR_DEVICE_LOST)
    {
        return RHIFrameStatus::DeviceLost;
    }
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        return RHIFrameStatus::OutOfDate;
    }
    return result == VK_SUCCESS ? RHIFrameStatus::Ready : RHIFrameStatus::Error;
}

[[nodiscard]] uint32_t FindMemoryType(const VkPhysicalDevice physicalDevice,
                                      const uint32_t typeBits,
                                      const VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index)
    {
        if ((typeBits & (1u << index)) != 0 &&
            (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties)
        {
            return index;
        }
    }
    return UINT32_MAX;
}

} // namespace

VulkanDevice::VulkanDevice(const RHIDeviceCreateDesc& desc)
{
    m_Traits.backendType = ERHIBackendType::Vulkan;
    m_Traits.bNativeNDCDepthZeroToOne = true;
    m_Traits.bNativeNDCYAxisUp = false;
    m_Traits.bNativeTextureOriginTopLeft = true;
    m_Traits.bSupportsClipControl = false;
    m_Traits.bRTSampleRequiresFlipY = false;
    m_Traits.bSupportsSceneRendering = true;
    m_Traits.bSupportsFullSceneRendering = false;
    m_Valid = Initialize(desc);
}

VulkanDevice::~VulkanDevice()
{
    WaitIdle();
    m_TransientUniformBuffer.reset();
    DestroySwapchain();
    DestroyFrameResources();
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
    if (m_ImmediateCommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_Device, m_ImmediateCommandPool, nullptr);
        m_ImmediateCommandPool = VK_NULL_HANDLE;
        m_ImmediateCommandBuffer = VK_NULL_HANDLE;
    }
    if (m_Device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_Device, nullptr);
        m_Device = VK_NULL_HANDLE;
    }
    if (m_Surface != VK_NULL_HANDLE && m_Instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }
    DestroyDebugMessenger();
    if (m_Instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_Instance, nullptr);
        m_Instance = VK_NULL_HANDLE;
    }
    TE_LOG_INFO("[RHIVulkan] Vulkan Device destroyed");
}

bool VulkanDevice::Initialize(const RHIDeviceCreateDesc& desc)
{
    if (!desc.nativeWindowHandle)
    {
        TE_LOG_ERROR("[RHIVulkan] Native GLFW window handle is required");
        return false;
    }
    if (glfwVulkanSupported() != GLFW_TRUE)
    {
        TE_LOG_ERROR("[RHIVulkan] GLFW reports Vulkan is unavailable");
        return false;
    }

#if defined(TE_VULKAN_VALIDATION)
    m_ValidationEnabled = true;
#endif
    if (m_ValidationEnabled && !HasValidationLayer())
    {
        TE_LOG_ERROR("[RHIVulkan] {} is required but unavailable", ValidationLayerName);
        return false;
    }

    if (!CreateInstance() ||
        !CreateDebugMessenger() ||
        !CreateSurface(desc.nativeWindowHandle) ||
        !SelectPhysicalDevice() ||
        !CreateLogicalDevice() ||
        !CreateDescriptorPool() ||
        !CreateImmediateResources() ||
        !CreateFrameResources(std::max(2u, desc.framesInFlight)))
    {
        return false;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
    const uint32_t frameCount = static_cast<uint32_t>(m_Frames.size());
    if (!m_TransientUniformAllocator.Initialize(desc.transientUniformBytesPerFrame,
                                                frameCount,
                                                std::max<VkDeviceSize>(1, properties.limits.minUniformBufferOffsetAlignment)))
    {
        TE_LOG_ERROR("[RHIVulkan] Failed to initialize transient uniform allocator");
        return false;
    }
    RHIBufferDesc transientDesc;
    transientDesc.usage = RHIBufferUsage::Uniform | RHIBufferUsage::CopyDestination;
    transientDesc.memoryUsage = RHIMemoryUsage::CPUToGPU;
    transientDesc.size = m_TransientUniformAllocator.GetTotalSize();
    transientDesc.debugName = "RHIVulkan_TransientUniformRing";
    m_TransientUniformBuffer = CreateBuffer(transientDesc);
    if (!m_TransientUniformBuffer)
    {
        TE_LOG_ERROR("[RHIVulkan] Failed to create transient uniform ring");
        return false;
    }

    m_VSync = desc.vsync;
    TE_LOG_INFO("[RHIVulkan] Stage B initialized: resources + descriptors + Dynamic Rendering + {} frames in flight",
                m_Frames.size());
    return true;
}

bool VulkanDevice::CreateInstance()
{
    uint32_t loaderVersion = VK_API_VERSION_1_0;
    if (vkEnumerateInstanceVersion)
    {
        vkEnumerateInstanceVersion(&loaderVersion);
    }
    if (loaderVersion < RequiredVulkanVersion)
    {
        TE_LOG_ERROR("[RHIVulkan] Vulkan 1.3 loader required, found {}.{}.{}",
                     VK_API_VERSION_MAJOR(loaderVersion),
                     VK_API_VERSION_MINOR(loaderVersion),
                     VK_API_VERSION_PATCH(loaderVersion));
        return false;
    }

    uint32_t extensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (!glfwExtensions || extensionCount == 0)
    {
        TE_LOG_ERROR("[RHIVulkan] GLFW returned no required instance extensions");
        return false;
    }
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);
    if (m_ValidationEnabled)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    const VkApplicationInfo applicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "ToyEngine Sandbox",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = "ToyEngine",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .apiVersion = RequiredVulkanVersion,
    };
    const VkDebugUtilsMessengerCreateInfoEXT debugInfo = MakeDebugMessengerCreateInfo();
    const char* validationLayer = ValidationLayerName;
    const VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = m_ValidationEnabled ? &debugInfo : nullptr,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = m_ValidationEnabled ? 1u : 0u,
        .ppEnabledLayerNames = m_ValidationEnabled ? &validationLayer : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    const VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
    if (result != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] vkCreateInstance failed: {}", static_cast<int32_t>(result));
        return false;
    }

    TE_LOG_INFO("[RHIVulkan] Vulkan instance created (loader {}.{}.{}, validation={})",
                VK_API_VERSION_MAJOR(loaderVersion),
                VK_API_VERSION_MINOR(loaderVersion),
                VK_API_VERSION_PATCH(loaderVersion),
                m_ValidationEnabled);
    return true;
}

bool VulkanDevice::CreateDebugMessenger()
{
    if (!m_ValidationEnabled)
    {
        return true;
    }
    const auto createFunction = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
    if (!createFunction)
    {
        TE_LOG_ERROR("[RHIVulkan] vkCreateDebugUtilsMessengerEXT is unavailable");
        return false;
    }
    const VkDebugUtilsMessengerCreateInfoEXT createInfo = MakeDebugMessengerCreateInfo();
    const VkResult result = createFunction(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
    if (result != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] Debug messenger creation failed: {}", static_cast<int32_t>(result));
        return false;
    }
    return true;
}

bool VulkanDevice::CreateSurface(void* nativeWindowHandle)
{
    const VkResult result = glfwCreateWindowSurface(
        m_Instance,
        static_cast<GLFWwindow*>(nativeWindowHandle),
        nullptr,
        &m_Surface);
    if (result != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] glfwCreateWindowSurface failed: {}", static_cast<int32_t>(result));
        return false;
    }
    return true;
}

bool VulkanDevice::SelectPhysicalDevice()
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
    if (count == 0)
    {
        TE_LOG_ERROR("[RHIVulkan] No Vulkan physical device found");
        return false;
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

    int bestScore = -1;
    for (const VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.apiVersion < RequiredVulkanVersion)
        {
            continue;
        }

        const FQueueFamilies queues = FindQueueFamilies(device, m_Surface);
        if (!queues.IsComplete() || !HasRequiredDeviceExtensions(device) || !SupportsDynamicRendering(device))
        {
            continue;
        }
        const FSwapchainSupport swapchain = QuerySwapchainSupport(device, m_Surface);
        if (swapchain.Formats.empty() || swapchain.PresentModes.empty())
        {
            continue;
        }
        const int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1000 : 100;
        if (score > bestScore)
        {
            bestScore = score;
            m_PhysicalDevice = device;
            m_GraphicsQueueFamily = *queues.Graphics;
            m_PresentQueueFamily = *queues.Present;
        }
    }
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        TE_LOG_ERROR("[RHIVulkan] No device supports graphics, present, swapchain and Dynamic Rendering");
        return false;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
    TE_LOG_INFO("[RHIVulkan] Selected GPU: {} (Vulkan {}.{}.{})",
                properties.deviceName,
                VK_API_VERSION_MAJOR(properties.apiVersion),
                VK_API_VERSION_MINOR(properties.apiVersion),
                VK_API_VERSION_PATCH(properties.apiVersion));
    return true;
}

bool VulkanDevice::CreateLogicalDevice()
{
    const std::set<uint32_t> uniqueFamilies = {m_GraphicsQueueFamily, m_PresentQueueFamily};
    constexpr float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(uniqueFamilies.size());
    for (const uint32_t family : uniqueFamilies)
    {
        queueInfos.push_back({
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family,
            .queueCount = 1,
            .pQueuePriorities = &priority,
        });
    }

    VkPhysicalDeviceVulkan13Features vulkan13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE,
    };
    const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    const VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &vulkan13,
        .queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &extension,
    };
    const VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
    if (result != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] vkCreateDevice failed: {}", static_cast<int32_t>(result));
        return false;
    }
    vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);
    return true;
}

bool VulkanDevice::CreateDescriptorPool()
{
    constexpr std::array poolSizes = {
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 512},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 512},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 128},
    };
    const VkDescriptorPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1024,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };
    if (vkCreateDescriptorPool(m_Device, &createInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] Failed to create descriptor pool");
        return false;
    }
    return true;
}

bool VulkanDevice::CreateImmediateResources()
{
    const VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_GraphicsQueueFamily,
    };
    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_ImmediateCommandPool) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] Failed to create immediate command pool");
        return false;
    }
    const VkCommandBufferAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_ImmediateCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    if (vkAllocateCommandBuffers(m_Device, &allocateInfo, &m_ImmediateCommandBuffer) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] Failed to allocate immediate command buffer");
        return false;
    }
    return true;
}

bool VulkanDevice::CreateBufferAllocation(const VkDeviceSize size,
                                          const VkBufferUsageFlags usage,
                                          const VkMemoryPropertyFlags properties,
                                          VkBuffer& outBuffer,
                                          VkDeviceMemory& outMemory) const
{
    outBuffer = VK_NULL_HANDLE;
    outMemory = VK_NULL_HANDLE;
    const VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &outBuffer) != VK_SUCCESS)
    {
        return false;
    }
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(m_Device, outBuffer, &requirements);
    const uint32_t memoryType = FindMemoryType(m_PhysicalDevice, requirements.memoryTypeBits, properties);
    if (memoryType == UINT32_MAX)
    {
        vkDestroyBuffer(m_Device, outBuffer, nullptr);
        outBuffer = VK_NULL_HANDLE;
        return false;
    }
    const VkMemoryAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = memoryType,
    };
    if (vkAllocateMemory(m_Device, &allocateInfo, nullptr, &outMemory) != VK_SUCCESS ||
        vkBindBufferMemory(m_Device, outBuffer, outMemory, 0) != VK_SUCCESS)
    {
        if (outMemory != VK_NULL_HANDLE) vkFreeMemory(m_Device, outMemory, nullptr);
        vkDestroyBuffer(m_Device, outBuffer, nullptr);
        outBuffer = VK_NULL_HANDLE;
        outMemory = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

bool VulkanDevice::CreateImageAllocation(const uint32_t width,
                                         const uint32_t height,
                                         const VkFormat format,
                                         const VkImageUsageFlags usage,
                                         const VkSampleCountFlagBits sampleCount,
                                         VkImage& outImage,
                                         VkDeviceMemory& outMemory) const
{
    outImage = VK_NULL_HANDLE;
    outMemory = VK_NULL_HANDLE;
    const VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = sampleCount,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (vkCreateImage(m_Device, &imageInfo, nullptr, &outImage) != VK_SUCCESS)
    {
        return false;
    }
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(m_Device, outImage, &requirements);
    const uint32_t memoryType = FindMemoryType(
        m_PhysicalDevice, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryType == UINT32_MAX)
    {
        vkDestroyImage(m_Device, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return false;
    }
    const VkMemoryAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = memoryType,
    };
    if (vkAllocateMemory(m_Device, &allocateInfo, nullptr, &outMemory) != VK_SUCCESS ||
        vkBindImageMemory(m_Device, outImage, outMemory, 0) != VK_SUCCESS)
    {
        if (outMemory != VK_NULL_HANDLE) vkFreeMemory(m_Device, outMemory, nullptr);
        vkDestroyImage(m_Device, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        outMemory = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

bool VulkanDevice::ExecuteImmediate(const std::function<void(VkCommandBuffer)>& recorder)
{
    if (!recorder || m_ImmediateCommandBuffer == VK_NULL_HANDLE)
    {
        return false;
    }
    if (vkResetCommandPool(m_Device, m_ImmediateCommandPool, 0) != VK_SUCCESS)
    {
        return false;
    }
    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    if (vkBeginCommandBuffer(m_ImmediateCommandBuffer, &beginInfo) != VK_SUCCESS)
    {
        return false;
    }
    recorder(m_ImmediateCommandBuffer);
    if (vkEndCommandBuffer(m_ImmediateCommandBuffer) != VK_SUCCESS)
    {
        return false;
    }
    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_ImmediateCommandBuffer,
    };
    if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        return false;
    }
    return vkQueueWaitIdle(m_GraphicsQueue) == VK_SUCCESS;
}

bool VulkanDevice::UploadBuffer(const VkBuffer destination,
                                const VkDeviceSize destinationOffset,
                                const void* data,
                                const VkDeviceSize size)
{
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (!CreateBufferAllocation(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                staging, memory))
    {
        return false;
    }
    void* mapped = nullptr;
    bool success = vkMapMemory(m_Device, memory, 0, size, 0, &mapped) == VK_SUCCESS;
    if (success)
    {
        std::memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(m_Device, memory);
        success = ExecuteImmediate([&](const VkCommandBuffer commandBuffer)
        {
            const VkBufferCopy copy{.dstOffset = destinationOffset, .size = size};
            vkCmdCopyBuffer(commandBuffer, staging, destination, 1, &copy);
        });
    }
    vkDestroyBuffer(m_Device, staging, nullptr);
    vkFreeMemory(m_Device, memory, nullptr);
    return success;
}

bool VulkanDevice::UploadTexture2D(const VkImage destination,
                                   const uint32_t width,
                                   const uint32_t height,
                                   const VkImageAspectFlags aspectMask,
                                   const void* data,
                                   const uint64_t dataSize,
                                   const uint64_t sourceRowPitch,
                                   const uint64_t tightRowPitch)
{
    if (!data || width == 0 || height == 0 || sourceRowPitch < tightRowPitch ||
        dataSize < sourceRowPitch * height)
    {
        return false;
    }
    const VkDeviceSize uploadSize = tightRowPitch * height;
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (!CreateBufferAllocation(uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                staging, memory))
    {
        return false;
    }
    void* mapped = nullptr;
    bool success = vkMapMemory(m_Device, memory, 0, uploadSize, 0, &mapped) == VK_SUCCESS;
    if (success)
    {
        const auto* source = static_cast<const std::byte*>(data);
        auto* target = static_cast<std::byte*>(mapped);
        for (uint32_t row = 0; row < height; ++row)
        {
            std::memcpy(target + row * tightRowPitch, source + row * sourceRowPitch,
                        static_cast<size_t>(tightRowPitch));
        }
        vkUnmapMemory(m_Device, memory);
        success = ExecuteImmediate([&](const VkCommandBuffer commandBuffer)
        {
            VkImageMemoryBarrier toTransfer{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = destination,
                .subresourceRange = {aspectMask, 0, 1, 0, 1},
            };
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &toTransfer);
            const VkBufferImageCopy copy{
                .imageSubresource = {aspectMask, 0, 0, 1},
                .imageExtent = {width, height, 1},
            };
            vkCmdCopyBufferToImage(commandBuffer, staging, destination,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            VkImageMemoryBarrier toShader = toTransfer;
            toShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            toShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            toShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &toShader);
        });
    }
    vkDestroyBuffer(m_Device, staging, nullptr);
    vkFreeMemory(m_Device, memory, nullptr);
    return success;
}

bool VulkanDevice::CreateFrameResources(const uint32_t frameCount)
{
    m_Frames.resize(frameCount);
    for (auto& frame : m_Frames)
    {
        const VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = m_GraphicsQueueFamily,
        };
        if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &frame.CommandPool) != VK_SUCCESS)
        {
            TE_LOG_ERROR("[RHIVulkan] Failed to create frame command pool");
            return false;
        }

        const VkCommandBufferAllocateInfo allocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = frame.CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        if (vkAllocateCommandBuffers(m_Device, &allocateInfo, &frame.NativeCommandBuffer) != VK_SUCCESS)
        {
            TE_LOG_ERROR("[RHIVulkan] Failed to allocate frame command buffer");
            return false;
        }
        frame.CommandBuffer = std::make_unique<VulkanCommandBuffer>(frame.NativeCommandBuffer);

        const VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        const VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &frame.ImageAvailable) != VK_SUCCESS ||
            vkCreateFence(m_Device, &fenceInfo, nullptr, &frame.InFlight) != VK_SUCCESS)
        {
            TE_LOG_ERROR("[RHIVulkan] Failed to create frame synchronization objects");
            return false;
        }
    }
    return true;
}

RHIFrameStatus VulkanDevice::CreateSwapchain(const uint32_t width, const uint32_t height, const bool vsync)
{
    if (width == 0 || height == 0)
    {
        return RHIFrameStatus::Skipped;
    }

    const FSwapchainSupport support = QuerySwapchainSupport(m_PhysicalDevice, m_Surface);
    if (support.Formats.empty() || support.PresentModes.empty())
    {
        TE_LOG_ERROR("[RHIVulkan] Surface has no usable formats or present modes");
        return RHIFrameStatus::Error;
    }

    const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(support.Formats);
    const VkPresentModeKHR presentMode = ChoosePresentMode(support.PresentModes, vsync);
    VkExtent2D extent = support.Capabilities.currentExtent;
    if (extent.width == std::numeric_limits<uint32_t>::max())
    {
        extent.width = std::clamp(width,
                                  support.Capabilities.minImageExtent.width,
                                  support.Capabilities.maxImageExtent.width);
        extent.height = std::clamp(height,
                                   support.Capabilities.minImageExtent.height,
                                   support.Capabilities.maxImageExtent.height);
    }
    if (extent.width == 0 || extent.height == 0)
    {
        // 最小化可能发生在 BeginFrame 尺寸检查之后；此时延后重建而不是创建非法交换链。
        return RHIFrameStatus::Skipped;
    }

    uint32_t imageCount = support.Capabilities.minImageCount + 1;
    if (support.Capabilities.maxImageCount > 0)
    {
        imageCount = std::min(imageCount, support.Capabilities.maxImageCount);
    }

    const std::array queueFamilies = {m_GraphicsQueueFamily, m_PresentQueueFamily};
    const bool separateQueues = m_GraphicsQueueFamily != m_PresentQueueFamily;
    const VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_Surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = separateQueues ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = separateQueues ? 2u : 0u,
        .pQueueFamilyIndices = separateQueues ? queueFamilies.data() : nullptr,
        .preTransform = support.Capabilities.currentTransform,
        .compositeAlpha = ChooseCompositeAlpha(support.Capabilities.supportedCompositeAlpha),
        .presentMode = presentMode,
        .clipped = VK_TRUE,
    };
    const VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain);
    if (result != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] vkCreateSwapchainKHR failed: {}", static_cast<int32_t>(result));
        return ToFrameStatus(result);
    }

    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
    m_SwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());
    m_SwapchainImageViews.resize(imageCount);
    for (uint32_t index = 0; index < imageCount; ++index)
    {
        const VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_SwapchainImages[index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surfaceFormat.format,
            .components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1,
                0, 1,
            },
        };
        if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_SwapchainImageViews[index]) != VK_SUCCESS)
        {
            TE_LOG_ERROR("[RHIVulkan] Failed to create swapchain image view {}", index);
            return RHIFrameStatus::Error;
        }
    }

    m_SwapchainDepthTextures.reserve(imageCount);
    for (uint32_t index = 0; index < imageCount; ++index)
    {
        RHITextureDesc depthDesc;
        depthDesc.width = extent.width;
        depthDesc.height = extent.height;
        depthDesc.format = RHIFormat::D32_Float;
        depthDesc.usage = RHITextureUsage::DepthStencilAttachment;
        depthDesc.mipLevels = 1;
        depthDesc.generateMips = false;
        depthDesc.srgb = false;
        depthDesc.initialState = RHIResourceState::DepthWrite;
        depthDesc.debugName = "RHIVulkan_SwapchainDepth_" + std::to_string(index);
        auto depthTexture = std::make_unique<VulkanTexture>(*this, depthDesc);
        if (!depthTexture->IsValid())
        {
            TE_LOG_ERROR("[RHIVulkan] Failed to create swapchain depth texture {}", index);
            return RHIFrameStatus::Error;
        }
        m_SwapchainDepthTextures.push_back(std::move(depthTexture));
    }

    const VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    m_RenderFinishedSemaphores.resize(imageCount);
    for (uint32_t index = 0; index < imageCount; ++index)
    {
        if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr,
                              &m_RenderFinishedSemaphores[index]) != VK_SUCCESS)
        {
            TE_LOG_ERROR("[RHIVulkan] Failed to create present semaphore {}", index);
            return RHIFrameStatus::Error;
        }
    }

    m_SwapchainFormat = surfaceFormat.format;
    m_BackBufferFormat = ToRHIFormat(surfaceFormat.format);
    m_SwapchainExtent = extent;
    m_ImagesInFlight.assign(imageCount, VK_NULL_HANDLE);
    m_ImageInitialized.assign(imageCount, false);
    m_LastRequestedWidth = width;
    m_LastRequestedHeight = height;
    m_VSync = vsync;
    m_SwapchainDirty = false;

    TE_LOG_INFO("[RHIVulkan] Swapchain created: {}x{}, images={}, format={}, presentMode={}",
                extent.width, extent.height, imageCount,
                static_cast<int32_t>(surfaceFormat.format),
                static_cast<int32_t>(presentMode));
    return RHIFrameStatus::Ready;
}

RHIFrameStatus VulkanDevice::RecreateSwapchain(const uint32_t width, const uint32_t height, const bool vsync)
{
    if (width == 0 || height == 0)
    {
        return RHIFrameStatus::Skipped;
    }
    const VkResult waitResult = vkDeviceWaitIdle(m_Device);
    if (waitResult != VK_SUCCESS)
    {
        return ToFrameStatus(waitResult);
    }
    DestroySwapchain();
    return CreateSwapchain(width, height, vsync);
}

void VulkanDevice::DestroySwapchain()
{
    if (m_Device == VK_NULL_HANDLE)
    {
        return;
    }
    m_SwapchainDepthTextures.clear();
    for (const VkImageView view : m_SwapchainImageViews)
    {
        if (view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, view, nullptr);
        }
    }
    m_SwapchainImageViews.clear();
    for (const VkSemaphore semaphore : m_RenderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_Device, semaphore, nullptr);
        }
    }
    m_RenderFinishedSemaphores.clear();
    m_SwapchainImages.clear();
    m_ImagesInFlight.clear();
    m_ImageInitialized.clear();
    if (m_Swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }
    m_BackBufferFormat = RHIFormat::Undefined;
    m_SwapchainDirty = true;
}

void VulkanDevice::DestroyFrameResources()
{
    if (m_Device == VK_NULL_HANDLE)
    {
        m_Frames.clear();
        return;
    }
    for (auto& frame : m_Frames)
    {
        frame.CommandBuffer.reset();
        if (frame.ImageAvailable != VK_NULL_HANDLE) vkDestroySemaphore(m_Device, frame.ImageAvailable, nullptr);
        if (frame.InFlight != VK_NULL_HANDLE) vkDestroyFence(m_Device, frame.InFlight, nullptr);
        if (frame.CommandPool != VK_NULL_HANDLE) vkDestroyCommandPool(m_Device, frame.CommandPool, nullptr);
    }
    m_Frames.clear();
}

void VulkanDevice::DestroyDebugMessenger()
{
    if (m_DebugMessenger == VK_NULL_HANDLE || m_Instance == VK_NULL_HANDLE)
    {
        return;
    }
    const auto destroyFunction = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (destroyFunction)
    {
        destroyFunction(m_Instance, m_DebugMessenger, nullptr);
    }
    m_DebugMessenger = VK_NULL_HANDLE;
}

RHIFrameStatus VulkanDevice::BeginFrame(const RHIFrameBeginInfo& beginInfo, RHIFrameContext& outContext)
{
    outContext = {};
    if (!m_Valid || m_FrameActive)
    {
        TE_LOG_ERROR("[RHIVulkan] BeginFrame called in invalid state");
        return RHIFrameStatus::Error;
    }
    if (beginInfo.framebufferWidth == 0 || beginInfo.framebufferHeight == 0)
    {
        return RHIFrameStatus::Skipped;
    }

    if (m_Swapchain == VK_NULL_HANDLE || m_SwapchainDirty ||
        beginInfo.framebufferWidth != m_LastRequestedWidth ||
        beginInfo.framebufferHeight != m_LastRequestedHeight ||
        beginInfo.vsync != m_VSync)
    {
        const RHIFrameStatus swapchainStatus = RecreateSwapchain(
            beginInfo.framebufferWidth,
            beginInfo.framebufferHeight,
            beginInfo.vsync);
        if (swapchainStatus != RHIFrameStatus::Ready)
        {
            return swapchainStatus;
        }
    }

    FFrameResources& frame = m_Frames[m_CurrentFrame];
    VkResult result = vkWaitForFences(m_Device, 1, &frame.InFlight, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
    {
        return ToFrameStatus(result);
    }

    result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX,
                                   frame.ImageAvailable, VK_NULL_HANDLE, &m_ActiveImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_SwapchainDirty = true;
        return RHIFrameStatus::OutOfDate;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        return ToFrameStatus(result);
    }
    if (result == VK_SUBOPTIMAL_KHR)
    {
        m_SwapchainDirty = true;
    }

    const VkFence imageFence = m_ImagesInFlight[m_ActiveImageIndex];
    if (imageFence != VK_NULL_HANDLE && imageFence != frame.InFlight)
    {
        result = vkWaitForFences(m_Device, 1, &imageFence, VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS)
        {
            return ToFrameStatus(result);
        }
    }

    result = vkResetCommandPool(m_Device, frame.CommandPool, 0);
    if (result != VK_SUCCESS)
    {
        return ToFrameStatus(result);
    }
    m_TransientUniformAllocator.BeginFrame(m_CurrentFrame);
    frame.CommandBuffer->PrepareSwapchainTarget(
        m_SwapchainImageViews[m_ActiveImageIndex],
        m_SwapchainDepthTextures[m_ActiveImageIndex]->GetImageView(),
        m_SwapchainExtent);
    frame.CommandBuffer->Begin();

    RecordSwapchainTransition(frame.NativeCommandBuffer,
                              m_SwapchainImages[m_ActiveImageIndex],
                              m_ImageInitialized[m_ActiveImageIndex]
                                  ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                  : VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    m_FrameActive = true;
    outContext.commandBuffer = frame.CommandBuffer.get();
    outContext.frameNumber = m_FrameNumber;
    outContext.frameIndex = m_CurrentFrame;
    outContext.swapChainImageIndex = m_ActiveImageIndex;
    return RHIFrameStatus::Ready;
}

RHIFrameStatus VulkanDevice::EndFrame(RHIFrameContext& context)
{
    FFrameResources& frame = m_Frames[m_CurrentFrame];
    if (!m_FrameActive || context.commandBuffer != frame.CommandBuffer.get())
    {
        TE_LOG_ERROR("[RHIVulkan] EndFrame received invalid frame context");
        return RHIFrameStatus::Error;
    }

    RecordSwapchainTransition(frame.NativeCommandBuffer,
                              m_SwapchainImages[m_ActiveImageIndex],
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    frame.CommandBuffer->End();
    if (!frame.CommandBuffer->IsExecutable())
    {
        m_FrameActive = false;
        return RHIFrameStatus::Error;
    }

    VkResult result = vkResetFences(m_Device, 1, &frame.InFlight);
    if (result != VK_SUCCESS)
    {
        m_FrameActive = false;
        return ToFrameStatus(result);
    }

    constexpr VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSemaphore renderFinished = m_RenderFinishedSemaphores[m_ActiveImageIndex];
    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame.ImageAvailable,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &frame.NativeCommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinished,
    };
    result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, frame.InFlight);
    if (result != VK_SUCCESS)
    {
        m_FrameActive = false;
        return ToFrameStatus(result);
    }
    m_ImagesInFlight[m_ActiveImageIndex] = frame.InFlight;
    m_ImageInitialized[m_ActiveImageIndex] = true;

    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinished,
        .swapchainCount = 1,
        .pSwapchains = &m_Swapchain,
        .pImageIndices = &m_ActiveImageIndex,
    };
    result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        m_SwapchainDirty = true;
    }

    context = {};
    m_FrameActive = false;
    ++m_FrameNumber;
    m_CurrentFrame = (m_CurrentFrame + 1) % static_cast<uint32_t>(m_Frames.size());
    return ToFrameStatus(result);
}

void VulkanDevice::WaitIdle()
{
    if (m_Device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m_Device);
    }
}

void VulkanDevice::RecordSwapchainTransition(const VkCommandBuffer commandBuffer,
                                             const VkImage image,
                                             const VkImageLayout oldLayout,
                                             const VkImageLayout newLayout) const
{
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 1,
            0, 1,
        },
    };

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
            ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
            : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else
    {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;
        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    vkCmdPipelineBarrier(commandBuffer,
                         sourceStage,
                         destinationStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
}

bool VulkanDevice::AllocateTransientUniform(const void* data,
                                            uint64_t size,
                                            RHITransientUniformAllocation& outAllocation)
{
    outAllocation = {};
    if (!data || size == 0 || !m_TransientUniformBuffer)
    {
        return false;
    }
    uint64_t offset = 0;
    if (!m_TransientUniformAllocator.Allocate(size, offset) ||
        !m_TransientUniformBuffer->UpdateData(data, size, offset))
    {
        TE_LOG_ERROR("[RHIVulkan] Transient uniform ring exhausted or upload failed ({} bytes)", size);
        return false;
    }
    outAllocation = {m_TransientUniformBuffer.get(), offset, size};
    return true;
}

std::unique_ptr<RHIBuffer> VulkanDevice::CreateBuffer(const RHIBufferDesc& desc)
{
    auto buffer = std::make_unique<VulkanBuffer>(*this, desc);
    return buffer->IsValid() ? std::move(buffer) : nullptr;
}

std::unique_ptr<RHIShader> VulkanDevice::CreateShader(const RHIShaderDesc& desc)
{
    auto shader = std::make_unique<VulkanShader>(*this, desc);
    return shader->IsValid() ? std::move(shader) : nullptr;
}

std::unique_ptr<RHIPipeline> VulkanDevice::CreatePipeline(const RHIPipelineDesc& desc)
{
    auto pipeline = std::make_unique<VulkanPipeline>(*this, desc);
    return pipeline->IsValid() ? std::move(pipeline) : nullptr;
}

std::unique_ptr<RHICommandBuffer> VulkanDevice::CreateCommandBuffer()
{
    TE_LOG_ERROR("[RHIVulkan] Standalone command buffers are outside Stage B; use the frame command buffer");
    return nullptr;
}

std::unique_ptr<RHITexture> VulkanDevice::CreateTexture(const RHITextureDesc& desc)
{
    auto texture = std::make_unique<VulkanTexture>(*this, desc);
    return texture->IsValid() ? std::move(texture) : nullptr;
}

std::unique_ptr<RHISampler> VulkanDevice::CreateSampler(const RHISamplerDesc& desc)
{
    auto sampler = std::make_unique<VulkanSampler>(*this, desc);
    return sampler->IsValid() ? std::move(sampler) : nullptr;
}

std::unique_ptr<RHIBindGroup> VulkanDevice::CreateBindGroup(const RHIBindGroupDesc& desc)
{
    auto bindGroup = std::make_unique<VulkanBindGroup>(*this, desc);
    return bindGroup->IsValid() ? std::move(bindGroup) : nullptr;
}

std::unique_ptr<RHIBindGroupLayout> VulkanDevice::CreateBindGroupLayout(const RHIBindGroupLayoutDesc& desc)
{
    auto layout = std::make_unique<VulkanBindGroupLayout>(*this, desc);
    return layout->IsValid() ? std::move(layout) : nullptr;
}

std::unique_ptr<RHIPipelineLayout> VulkanDevice::CreatePipelineLayout(const RHIPipelineLayoutDesc& desc)
{
    auto layout = std::make_unique<VulkanPipelineLayout>(*this, desc);
    return layout->IsValid() ? std::move(layout) : nullptr;
}

std::unique_ptr<RHIRenderTarget> VulkanDevice::CreateRenderTarget(const RHIRenderTargetDesc& desc)
{
    (void)desc;
    TE_LOG_ERROR("[RHIVulkan] Offscreen RenderTarget creation is outside Stage B");
    return nullptr;
}

Matrix4 VulkanDevice::AdjustProjectionMatrix(const Matrix4& projection) const
{
    Matrix4 adjusted = projection;
    adjusted(1, 1) *= -1.0f;
    return adjusted;
}

} // namespace TE
