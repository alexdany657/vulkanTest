#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

/* constants */

const int WIDTH = 800;
const int HEIGHT = 600;

const uint32_t validationLayerCount = 1;
const char **validationLayers; /* see init later in "initValidationLayers" */

const uint32_t deviceExtensionsCount = 1;
const char **deviceExtensions;

#ifdef NDEBUG
const int8_t enableValidationLayers = 0;
#else
const int8_t enableValidationLayers = 1;
#endif

/* to remove gcc's warnings abnout unused something */
int SHUT_UP_GCC_INT;
void *SHUT_UP_GCC_PTR;

/* structures */

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR *pCapabilities;
    uint32_t formatCount;
    VkSurfaceFormatKHR *formats;
    uint32_t presentModeCount;
    VkPresentModeKHR *presentModes;
};

struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    uint8_t graphicsFamilyHV:1;

    uint32_t presentFamily;
    uint8_t presentFamilyHV:1;
};

struct HelloTriangleApp {
    GLFWwindow *pWindow;
    VkInstance *pInstance;
    VkDebugUtilsMessengerEXT *pDebugMessenger;
    VkPhysicalDevice *pPhysicalDevice;
    VkDevice *pDevice;
    VkQueue *pGraphicsQueue;
    VkQueue *pPresentQueue;
    VkSurfaceKHR *pSurface;
    VkSwapchainKHR *pSwapChain;
    uint32_t swapChainImagesCount;
    VkImage *swapChainImages;
    VkFormat *pSwapChainFormat;
    VkExtent2D swapChainExtent;
    VkImageView *swapChainImageViews;
    VkRenderPass *pRenderPass;
    VkPipelineLayout *pPipelineLayout;
    VkPipeline *pGraphicsPipeline;
};

/* functions */

int initDeviceExtensions() {
    deviceExtensions = malloc(sizeof(char *) * deviceExtensionsCount);
    if (!deviceExtensions) {
        printf("OOM: dropping\n");
        return 1;
    }
    *deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    return 0;
}

int initValidationLayers() {
    validationLayers = malloc(sizeof(char *) * validationLayerCount);
    if (!validationLayers) {
        printf("OOM: dropping\n");
        return 1;
    }
    *validationLayers = "VK_LAYER_KHRONOS_validation";

    return 0;
}

char *readFile(const char *fname, uint32_t *sizeBuff) {
    FILE *fd = fopen(fname, "r");
    if (!fd) {
        printf("Failed to read file\n");
        return NULL;
    }

    uint32_t size = 0;
    while (fgetc(fd) != EOF) size++;

    fclose(fd);

    *sizeBuff = size;
    char *data = malloc(sizeof(char) * size);
    if (!data) {
        printf("OOM: dropping\n");
        return NULL;
    }

    fd = fopen(fname, "r");
    fread(data, 1, size, fd);
    fclose(fd);

    return data;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {
    SHUT_UP_GCC_INT = messageSeverity;
    SHUT_UP_GCC_INT = messageType;
    SHUT_UP_GCC_PTR = pUserData;
    printf("validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance *pInstance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(*pInstance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(*pInstance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

int DestroyDebugUtilsMessengerEXT(VkInstance *pInstance,
        VkDebugUtilsMessengerEXT *pDebugMessenger,
        const VkAllocationCallbacks *pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(*pInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(*pInstance, *pDebugMessenger, pAllocator);
        return 0;
    }

    printf("Extension not present\n");
    return 1;
}

int setupDebugMessenger(void *_app) {
    if (!enableValidationLayers) return 0;

    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    pApp->pDebugMessenger = malloc(sizeof(VkDebugUtilsMessengerEXT));
    if (!pApp->pDebugMessenger) {
        printf("OOM: dropping\n");
        return 1;
    }

    VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo = malloc(sizeof(VkDebugUtilsMessengerCreateInfoEXT));
    if (!pCreateInfo) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pCreateInfo, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));

    pCreateInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    pCreateInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    pCreateInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    pCreateInfo->pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(pApp->pInstance, pCreateInfo, NULL, pApp->pDebugMessenger) != VK_SUCCESS) {
        printf("Failed to set up debug messenger!\n");
        return 1;
    }

    return 0;
}

void *getRequiredExtensions(uint32_t *size) {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (!enableValidationLayers) {
        *size = glfwExtensionCount;
        return glfwExtensions;
    }
    const char **extensions;
    extensions = malloc(sizeof(char *) * (glfwExtensionCount + 1));
    memcpy(extensions, glfwExtensions, sizeof(char *) * glfwExtensionCount);
    if (!extensions) {
        printf("OOM: dropping\n");
        return NULL;
    }
    *size = glfwExtensionCount + 1;
    *(extensions + glfwExtensionCount) = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    return extensions;
}

int checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties *availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
    if (!availableLayers) {
        printf("OOM: dropping\n");
        return 1;
    }
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (uint32_t i = 0; i < validationLayerCount; ++i) {
        int8_t layerFound = 0;

        for (uint32_t j = 0; j < layerCount; ++j) {
            printf("Have:\t%s, need:\t%s\n", (availableLayers + j)->layerName, *(validationLayers + i));
            printf("%i\n", strcmp(*(validationLayers + i), (availableLayers + j)->layerName));
            if (!strcmp(*(validationLayers + i), (availableLayers + j)->layerName)) {
                layerFound = 1;
                break;
            }
        }

        if (!layerFound) {
            return 1;
        }
    }

    return 0;
}

VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR *capabilities) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    } else {
        VkExtent2D actualExtent = {WIDTH, HEIGHT};
        actualExtent.width =
            fmax(capabilities->minImageExtent.width,
            fmin(capabilities->maxImageExtent.width,
            actualExtent.width));
        actualExtent.height =
            fmax(capabilities->minImageExtent.height,
            fmin(capabilities->maxImageExtent.height,
            actualExtent.height));
        return actualExtent;
    }
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR *availablePresentModes, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        if (*(availablePresentModes + i) == VK_PRESENT_MODE_MAILBOX_KHR) {
            return *(availablePresentModes + i);
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

void *chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        if ((availableFormats + i)->format == VK_FORMAT_B8G8R8A8_UNORM && (availableFormats + i)->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats + i;
        }
    }
    return availableFormats;
}

void *querySwapChainSupport(void *_app, void *_device) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    VkPhysicalDevice *pDevice = (VkPhysicalDevice *)_device;

    struct SwapChainSupportDetails *pDetails = malloc(sizeof(struct SwapChainSupportDetails));
    if (!pDetails) {
        printf("OOM: dropping\n");
        return NULL;
    }
    memset(pDetails, 0, sizeof(struct SwapChainSupportDetails));

    pDetails->pCapabilities = malloc(sizeof(VkSurfaceCapabilitiesKHR));
    if (!pDetails->pCapabilities) {
        printf("OOM: dropping\n");
        return NULL;
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*pDevice, *(pApp->pSurface), pDetails->pCapabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(*pDevice, *(pApp->pSurface), &(pDetails->formatCount), NULL);
    if (pDetails->formatCount != 0) {
        pDetails->formats = malloc(sizeof(VkSurfaceFormatKHR) * pDetails->formatCount);
        if (!pDetails->formats) {
            printf("OOM: dropping\n");
            return NULL;
        }
        vkGetPhysicalDeviceSurfaceFormatsKHR(*pDevice, *(pApp->pSurface), &(pDetails->formatCount), pDetails->formats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(*pDevice, *(pApp->pSurface), &(pDetails->presentModeCount), NULL);
    if (pDetails->presentModeCount != 0) {
        pDetails->presentModes = malloc(sizeof(VkPresentModeKHR) * pDetails->presentModeCount);
        if (!pDetails->presentModes) {
            printf("OOM: dropping\n");
            return NULL;
        }
        vkGetPhysicalDeviceSurfacePresentModesKHR(*pDevice, *(pApp->pSurface), &(pDetails->presentModeCount), pDetails->presentModes);
    }

    return pDetails;
}

void *findQueueFamilies(void *_app, void *_device) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    VkPhysicalDevice *pDevice = (VkPhysicalDevice *)_device;

    struct QueueFamilyIndices *pIndices = malloc(sizeof(struct QueueFamilyIndices));
    if (!(pIndices)) {
        printf("OOM: dropping\n");
        return NULL;
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*pDevice, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *pQueueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(*pDevice, &queueFamilyCount, pQueueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if ((pQueueFamilies + i)->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            pIndices->graphicsFamily = i;
            pIndices->graphicsFamilyHV = 1;
        }


        VkBool32 presentSupport = (VkBool32)0;
        vkGetPhysicalDeviceSurfaceSupportKHR(*pDevice, i, *(pApp->pSurface), &presentSupport);
        if (presentSupport) {
            pIndices->presentFamily = i;
            pIndices->presentFamilyHV = 1;
        }

        if (pIndices->graphicsFamilyHV && pIndices->presentFamilyHV) {
            break;
        }
    }

    return pIndices;
}

int8_t checkDeviceExtensionsSupport(void *_device) {
    VkPhysicalDevice *pDevice = (VkPhysicalDevice *)_device;

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(*pDevice, NULL, &extensionCount, NULL);

    VkExtensionProperties *availableExtensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
    if (!availableExtensions) {
        printf("OOM: dropping\n");
        return 1;
    }
    vkEnumerateDeviceExtensionProperties(*pDevice, NULL, &extensionCount, availableExtensions);

    for (uint32_t i = 0; i < deviceExtensionsCount; ++i) {
        uint8_t found = 0;

        for (uint32_t j = 0; j < extensionCount; ++j) {
#ifndef NDEBUG
            printf("need:\t%s\navailable:\t%s\n", *(deviceExtensions + i), (availableExtensions + j)->extensionName);
#endif
            if (!strcmp(*(deviceExtensions + i), (availableExtensions + j)->extensionName)) {
                found = 1;
                break;
            }
        }

        if (!found) {
            return 1;
        }
    }

    return 0;
}

int8_t isDeviceSuitable(void *pApp, void *_device) {
    VkPhysicalDevice *pDevice = (VkPhysicalDevice *)_device;

    VkPhysicalDeviceProperties *pDeviceProperties = malloc(sizeof(VkPhysicalDeviceProperties));
    vkGetPhysicalDeviceProperties(*pDevice, pDeviceProperties);
    
    VkPhysicalDeviceFeatures *pDeviceFeatures = malloc(sizeof(VkPhysicalDeviceFeatures));
    vkGetPhysicalDeviceFeatures(*pDevice, pDeviceFeatures);

    struct QueueFamilyIndices *pIndices = (struct QueueFamilyIndices *)findQueueFamilies(pApp, pDevice);
    if (!pIndices) {
        return 0;
    }
    struct SwapChainSupportDetails *pSwapChainSupport = querySwapChainSupport(pApp, pDevice);
    if (!pSwapChainSupport) {
        return 0;
    }

    int8_t indicesComplete = pIndices && pIndices->graphicsFamilyHV && pIndices->presentFamilyHV;
    int8_t extensionsSupported = !checkDeviceExtensionsSupport(pDevice);
    int8_t swapChainAdequate = pSwapChainSupport->formatCount && pSwapChainSupport->presentModeCount;

    return indicesComplete && extensionsSupported && swapChainAdequate;
}

VkShaderModule *createShaderModule(void *_app, const char *code, uint64_t size) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;
    VkShaderModuleCreateInfo *pCreateInfo = malloc(sizeof(VkShaderModuleCreateInfo));
    if (!pCreateInfo) {
        printf("OOM: dropping\n");
        return NULL;
    }
    memset(pCreateInfo, 0, sizeof(VkShaderModuleCreateInfo));
    pCreateInfo->sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    pCreateInfo->codeSize = size;
    pCreateInfo->pCode = (const uint32_t *)code;

    VkShaderModule *pShaderModule = malloc(sizeof(VkShaderModule));
    if (!pShaderModule) {
        printf("OOM: dropping\n");
        return NULL;
    }
    if (vkCreateShaderModule(*(pApp->pDevice), pCreateInfo, NULL, pShaderModule) != VK_SUCCESS) {
        printf("Failed to create shader module\n");
        return NULL;
    }

    return pShaderModule;
}

int createGraphicsPipeline(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    uint32_t vertShaderCodeSize, fragShaderCodeSize;
    char *vertShaderCode = readFile("vert.spv", &vertShaderCodeSize);
    char *fragShaderCode = readFile("frag.spv", &fragShaderCodeSize);

    VkShaderModule *vertShaderModule = createShaderModule(pApp, vertShaderCode, vertShaderCodeSize);
    if (!vertShaderModule) {
        printf("Failed to create vertex shader module\n");
        return 1;
    }
    VkShaderModule *fragShaderModule = createShaderModule(pApp, fragShaderCode, fragShaderCodeSize);
    if (!fragShaderModule) {
        printf("Failed to create fragment shader module\n");
        return 1;
    }

    VkPipelineShaderStageCreateInfo *pShaderStages = malloc(sizeof(VkPipelineShaderStageCreateInfo) * 2);
    if (!pShaderStages) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pShaderStages, 0, sizeof(VkPipelineShaderStageCreateInfo) * 2);
    pShaderStages->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pShaderStages->stage = VK_SHADER_STAGE_VERTEX_BIT;
    pShaderStages->module = *vertShaderModule;
    pShaderStages->pName = "main";

    (pShaderStages + 1)->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    (pShaderStages + 1)->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    (pShaderStages + 1)->module = *fragShaderModule;
    (pShaderStages + 1)->pName = "main";

    VkPipelineVertexInputStateCreateInfo *pVertexInputInfo = malloc(sizeof(VkPipelineVertexInputStateCreateInfo));
    if (!pVertexInputInfo) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pVertexInputInfo, 0, sizeof(VkPipelineVertexInputStateCreateInfo));
    pVertexInputInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    VkPipelineInputAssemblyStateCreateInfo *pInputAssembly = malloc(sizeof(VkPipelineInputAssemblyStateCreateInfo));
    if (!pInputAssembly) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pInputAssembly, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo));
    pInputAssembly->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pInputAssembly->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pInputAssembly->primitiveRestartEnable = VK_FALSE;

    VkViewport *pViewport = malloc(sizeof(VkViewport));
    if (!pViewport) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pViewport, 0, sizeof(VkViewport));
    pViewport->x = 0.0f;
    pViewport->y = 0.0f;
    pViewport->width = (float)pApp->swapChainExtent.width;
    pViewport->height = (float)pApp->swapChainExtent.height;
    pViewport->minDepth = 0.0f;
    pViewport->maxDepth = 1.0f;

    VkRect2D *pScissor = malloc(sizeof(VkRect2D));
    if (!pScissor) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pScissor, 0, sizeof(VkRect2D));
    pScissor->offset.x = 0;
    pScissor->offset.y = 0;
    pScissor->extent = pApp->swapChainExtent;

    VkPipelineViewportStateCreateInfo *pViewportState = malloc(sizeof(VkPipelineViewportStateCreateInfo));
    if (!pViewportState) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pViewportState, 0, sizeof(VkPipelineViewportStateCreateInfo));
    pViewportState->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pViewportState->viewportCount = 1;
    pViewportState->pViewports = pViewport;
    pViewportState->scissorCount = 1;
    pViewportState->pScissors = pScissor;

    VkPipelineRasterizationStateCreateInfo *pRasterizer = malloc(sizeof(VkPipelineRasterizationStateCreateInfo));
    if (!pRasterizer) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pRasterizer, 0, sizeof(VkPipelineRasterizationStateCreateInfo));
    pRasterizer->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pRasterizer->depthClampEnable = VK_FALSE;
    pRasterizer->rasterizerDiscardEnable = VK_FALSE;
    pRasterizer->polygonMode = VK_POLYGON_MODE_FILL;
    pRasterizer->lineWidth = 1.0f;
    pRasterizer->cullMode = VK_CULL_MODE_BACK_BIT;
    pRasterizer->frontFace = VK_FRONT_FACE_CLOCKWISE;
    pRasterizer->depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo *pMultisampling = malloc(sizeof(VkPipelineMultisampleStateCreateInfo));
    if (!pMultisampling) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pMultisampling, 0, sizeof(VkPipelineMultisampleStateCreateInfo));
    pMultisampling->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pMultisampling->sampleShadingEnable = VK_FALSE;
    pMultisampling->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pMultisampling->minSampleShading = 1.0f;

    VkPipelineColorBlendAttachmentState *pColorBlendAttachment = malloc(sizeof(VkPipelineColorBlendAttachmentState));
    if (!pColorBlendAttachment) {
        printf("OOM: dorpping\n");
        return 1;
    }
    memset(pColorBlendAttachment, 0, sizeof(VkPipelineColorBlendAttachmentState));
    pColorBlendAttachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pColorBlendAttachment->blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo *pColorBlending = malloc(sizeof(VkPipelineColorBlendStateCreateInfo));
    if (!pColorBlending) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pColorBlending, 0, sizeof(VkPipelineColorBlendStateCreateInfo));
    pColorBlending->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pColorBlending->logicOpEnable = VK_FALSE;
    pColorBlending->attachmentCount = 1;
    pColorBlending->pAttachments = pColorBlendAttachment;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo *pDynamicState = malloc(sizeof(VkPipelineDynamicStateCreateInfo));
    if (!pDynamicState) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pDynamicState, 0, sizeof(VkPipelineDynamicStateCreateInfo));
    pDynamicState->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pDynamicState->dynamicStateCount = 2;
    pDynamicState->pDynamicStates = dynamicStates;

    pApp->pPipelineLayout = malloc(sizeof(VkPipelineLayout));
    if (!(pApp->pPipelineLayout)) {
        printf("OOM: dropping\n");
        return 1;
    }
    VkPipelineLayoutCreateInfo *pPipelineLayoutInfo = malloc(sizeof(VkPipelineLayoutCreateInfo));
    if (!pPipelineLayoutInfo) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pPipelineLayoutInfo, 0, sizeof(VkPipelineLayoutCreateInfo));
    pPipelineLayoutInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(*(pApp->pDevice), pPipelineLayoutInfo, NULL, pApp->pPipelineLayout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        return 1;
    }

    VkGraphicsPipelineCreateInfo *pPipelineInfo = malloc(sizeof(VkGraphicsPipelineCreateInfo));
    if (!pPipelineInfo) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pPipelineInfo, 0, sizeof(VkGraphicsPipelineCreateInfo));
    pPipelineInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pPipelineInfo->stageCount = 2;
    pPipelineInfo->pStages = pShaderStages;

    pPipelineInfo->pVertexInputState = pVertexInputInfo;
    pPipelineInfo->pInputAssemblyState = pInputAssembly;
    pPipelineInfo->pViewportState = pViewportState;
    pPipelineInfo->pRasterizationState = pRasterizer;
    pPipelineInfo->pMultisampleState = pMultisampling;
    pPipelineInfo->pColorBlendState = pColorBlending;

    pPipelineInfo->layout = *(pApp->pPipelineLayout);
    pPipelineInfo->renderPass = *(pApp->pRenderPass);
    pPipelineInfo->subpass = 0;

    pApp->pGraphicsPipeline = malloc(sizeof(VkPipeline));
    if (!(pApp->pGraphicsPipeline)) {
        printf("OOM: dropping\n");
        return 1;
    }
    if (vkCreateGraphicsPipelines(*(pApp->pDevice), VK_NULL_HANDLE, 1, pPipelineInfo, NULL, pApp->pGraphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        return 1;
    }

    vkDestroyShaderModule(*(pApp->pDevice), *vertShaderModule, NULL);
    vkDestroyShaderModule(*(pApp->pDevice), *fragShaderModule, NULL);

    return 0;
}

int createRenderPass(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    VkAttachmentDescription *pColorAttachment = malloc(sizeof(VkAttachmentDescription));
    if (!pColorAttachment) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pColorAttachment, 0, sizeof(VkAttachmentDescription));
    pColorAttachment->format = *(pApp->pSwapChainFormat);
    pColorAttachment->samples = VK_SAMPLE_COUNT_1_BIT;
    pColorAttachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    pColorAttachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    pColorAttachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    pColorAttachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    pColorAttachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    pColorAttachment->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference *pColorAttachmentRef = malloc(sizeof(VkAttachmentReference));
    if (!pColorAttachmentRef) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pColorAttachmentRef, 0, sizeof(VkAttachmentReference));
    pColorAttachmentRef->attachment = 0;
    pColorAttachmentRef->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription *pSubpass = malloc(sizeof(VkSubpassDescription));
    if (!pSubpass) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pSubpass, 0, sizeof(VkSubpassDescription));
    pSubpass->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    pSubpass->colorAttachmentCount = 1;
    pSubpass->pColorAttachments = pColorAttachmentRef;

    pApp->pRenderPass = malloc(sizeof(VkRenderPass));
    if (!(pApp->pRenderPass)) {
        printf("OOM: dropping\n");
        return 1;
    }

    VkRenderPassCreateInfo *pRenderPassInfo = malloc(sizeof(VkRenderPassCreateInfo));
    if (!pRenderPassInfo) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pRenderPassInfo, 0, sizeof(VkRenderPassCreateInfo));
    pRenderPassInfo->sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pRenderPassInfo->attachmentCount = 1;
    pRenderPassInfo->pAttachments = pColorAttachment;
    pRenderPassInfo->subpassCount = 1;
    pRenderPassInfo->pSubpasses = pSubpass;

    if (vkCreateRenderPass(*(pApp->pDevice), pRenderPassInfo, NULL, pApp->pRenderPass) != VK_SUCCESS) {
        printf("Failed to create render pass\n");
        return 1;
    }

    return 0;
}

int createImageViews(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    pApp->swapChainImageViews = malloc(sizeof(VkImageView) * pApp->swapChainImagesCount);
    if (!pApp->swapChainImageViews) {
        printf("OOM: dropping\n");
        return 1;
    }

    for (uint32_t i = 0; i < pApp->swapChainImagesCount; ++i) {
        VkImageViewCreateInfo *pCreateInfo = malloc(sizeof(VkImageViewCreateInfo));
        if (!pCreateInfo) {
            printf("OOM: dropping\n");
            return 1;
        }
        memset(pCreateInfo, 0, sizeof(VkImageViewCreateInfo));

        pCreateInfo->sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        pCreateInfo->image = *(pApp->swapChainImages + i);
        pCreateInfo->viewType = VK_IMAGE_VIEW_TYPE_2D;
        pCreateInfo->format = *(pApp->pSwapChainFormat);
#ifndef NDEBUG
        printf("%p, %i\n", pApp->pSwapChainFormat, *(pApp->pSwapChainFormat));
#endif
        pCreateInfo->components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        pCreateInfo->components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        pCreateInfo->components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        pCreateInfo->components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        pCreateInfo->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        pCreateInfo->subresourceRange.baseMipLevel = 0;
        pCreateInfo->subresourceRange.levelCount = 1;
        pCreateInfo->subresourceRange.baseArrayLayer = 0;
        pCreateInfo->subresourceRange.layerCount = 1;

        if (vkCreateImageView(*(pApp->pDevice), pCreateInfo, NULL, pApp->swapChainImageViews + i) != VK_SUCCESS) {
            printf("Failed to create image views\n");
            return 1;
        }
    }

    return 0;
}

int createSwapChain(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    pApp->pSwapChain = malloc(sizeof(VkSwapchainKHR));
    if (!pApp->pSwapChain) {
        printf("OOM: dropping\n");
        return 1;
    }

    struct SwapChainSupportDetails *pSwapChainSupport = (struct SwapChainSupportDetails *)querySwapChainSupport(pApp, pApp->pPhysicalDevice);

    VkSurfaceFormatKHR *pSurfaceFormat = (VkSurfaceFormatKHR *)chooseSwapSurfaceFormat(pSwapChainSupport->formats, pSwapChainSupport->formatCount);
#ifndef NDEBUG
    printf("%p, %i\n", pSurfaceFormat, pSurfaceFormat->format);
#endif

    VkPresentModeKHR presentMode = chooseSwapPresentMode(pSwapChainSupport->presentModes, pSwapChainSupport->presentModeCount);

    VkExtent2D extent = chooseSwapExtent(pSwapChainSupport->pCapabilities);

    uint32_t imageCount = pSwapChainSupport->pCapabilities->minImageCount + 1;

    if (pSwapChainSupport->pCapabilities->maxImageCount > 0 && imageCount > pSwapChainSupport->pCapabilities->maxImageCount) {
       imageCount = pSwapChainSupport->pCapabilities->maxImageCount; 
    }

    VkSwapchainCreateInfoKHR *pCreateInfo = malloc(sizeof(VkSwapchainCreateInfoKHR));
    if (!pCreateInfo) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pCreateInfo, 0, sizeof(VkSwapchainCreateInfoKHR));
    pCreateInfo->sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    pCreateInfo->surface = *(pApp->pSurface);
    pCreateInfo->minImageCount = imageCount;
    pCreateInfo->imageFormat = pSurfaceFormat->format;
    pCreateInfo->imageColorSpace = pSurfaceFormat->colorSpace;
    pCreateInfo->imageExtent = extent;
    pCreateInfo->imageArrayLayers = 1;
    pCreateInfo->imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    struct QueueFamilyIndices *pIndices = (struct QueueFamilyIndices *)findQueueFamilies(pApp, pApp->pPhysicalDevice);
    if (!pIndices) {
        printf("Failed to find queue families\n");
        return 1;
    }
    uint32_t queueFamilyIndices[] = {pIndices->graphicsFamily, pIndices->presentFamily};

    if (pIndices->graphicsFamily != pIndices->presentFamily) {
        pCreateInfo->imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        pCreateInfo->queueFamilyIndexCount = 2;
        pCreateInfo->pQueueFamilyIndices = queueFamilyIndices;
    } else {
        pCreateInfo->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    pCreateInfo->preTransform = pSwapChainSupport->pCapabilities->currentTransform;
    pCreateInfo->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    pCreateInfo->presentMode = presentMode;
    pCreateInfo->clipped = VK_TRUE;

    pCreateInfo->oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(*(pApp->pDevice), pCreateInfo, NULL, pApp->pSwapChain) != VK_SUCCESS) {
        printf("Failed to create swap chain\n");
        return 1;
    }

    vkGetSwapchainImagesKHR(*(pApp->pDevice), *(pApp->pSwapChain), &(pApp->swapChainImagesCount), NULL);
    pApp->swapChainImages = malloc(sizeof(VkImage) * pApp->swapChainImagesCount);
    if (!pApp->swapChainImages) {
        printf("OOM: dropping\n");
        return 1;
    }
    vkGetSwapchainImagesKHR(*(pApp->pDevice), *(pApp->pSwapChain), &(pApp->swapChainImagesCount), pApp->swapChainImages);

    pApp->pSwapChainFormat = &(pSurfaceFormat->format);
    pApp->swapChainExtent = extent;

    return 0;
}

int createSurface(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    pApp->pSurface = malloc(sizeof(VkSurfaceKHR));
    if (!pApp->pSurface) {
        printf("OOM: dropping\n");
        return 1;
    }

    if (glfwCreateWindowSurface(*(pApp->pInstance), (pApp->pWindow), NULL, pApp->pSurface) != VK_SUCCESS) {
        printf("OOM: dropping\n");
        return 1;
    }

    return 0;
}

int createLogicalDevice(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    pApp->pDevice = malloc(sizeof(VkDevice));
    if (!pApp->pDevice) {
        printf("OOM: dropping\n");
        return 1;
    }

    pApp->pGraphicsQueue = malloc(sizeof(VkQueue));
    if (!pApp->pGraphicsQueue) {
        printf("OOM: dropping\n");
        return 1;
    }

    pApp->pPresentQueue = malloc(sizeof(VkQueue));
    if (!pApp->pPresentQueue) {
        printf("OOM: dropping\n");
        return 1;
    }

    struct QueueFamilyIndices *pIndices = findQueueFamilies(pApp, pApp->pPhysicalDevice);
    int queueCreateInfoCount = 1;
    if (pIndices->graphicsFamily != pIndices->presentFamily) {
        queueCreateInfoCount++;
    }

    VkDeviceQueueCreateInfo *pQueueCreateInfos = malloc(sizeof(VkDeviceQueueCreateInfo) * queueCreateInfoCount);
    if (!pQueueCreateInfos) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pQueueCreateInfos, 0, sizeof(VkDeviceQueueCreateInfo) * queueCreateInfoCount);
    pQueueCreateInfos->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    pQueueCreateInfos->queueFamilyIndex = pIndices->graphicsFamily;
    pQueueCreateInfos->queueCount = 1;
    float queuePriority = 1.0f;
    pQueueCreateInfos->pQueuePriorities = &queuePriority;

    if (queueCreateInfoCount > 2) {
        (pQueueCreateInfos+1)->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        (pQueueCreateInfos+1)->queueFamilyIndex = pIndices->presentFamily;
        (pQueueCreateInfos+1)->queueCount = 1;
        (pQueueCreateInfos+1)->pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceFeatures *pDeviceFeatures = malloc(sizeof(VkPhysicalDeviceFeatures));
    if (!pDeviceFeatures) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pDeviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));

    VkDeviceCreateInfo *pCreateInfo = malloc(sizeof(VkDeviceCreateInfo));
    if (!pCreateInfo) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pCreateInfo, 0, sizeof(VkDeviceCreateInfo));

    pCreateInfo->sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    pCreateInfo->pQueueCreateInfos = pQueueCreateInfos;
    pCreateInfo->queueCreateInfoCount = queueCreateInfoCount;
    pCreateInfo->pEnabledFeatures = pDeviceFeatures;

    if (enableValidationLayers) {
        pCreateInfo->enabledLayerCount = validationLayerCount;
        pCreateInfo->ppEnabledLayerNames = validationLayers;
    } else {
        pCreateInfo->enabledLayerCount = 0;
    }
    pCreateInfo->enabledExtensionCount = deviceExtensionsCount;
    pCreateInfo->ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(*(pApp->pPhysicalDevice), pCreateInfo, NULL, pApp->pDevice) != VK_SUCCESS) {
        printf("Failed to create logical device\n");
        return 1;
    }

    vkGetDeviceQueue(*(pApp->pDevice), pIndices->graphicsFamily, 0, pApp->pGraphicsQueue);
    vkGetDeviceQueue(*(pApp->pDevice), pIndices->presentFamily, 0, pApp->pPresentQueue);

    return 0;
}

int pickPhysicalDevice(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    pApp->pPhysicalDevice = malloc(sizeof(VkPhysicalDevice));
    if (!pApp->pPhysicalDevice) {
        printf("OOM: dropping\n");
        return 1;
    }
    *pApp->pPhysicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(*(pApp->pInstance), &deviceCount, NULL);

    if (!deviceCount) {
        printf("Failed to find GPUs with Vulkan support\n");
        return 1;
    }
    
    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    if (!devices) {
        printf("OOM: dropping\n");
        return 1;
    }
    vkEnumeratePhysicalDevices(*(pApp->pInstance), &deviceCount, devices);

    for (uint32_t i = 0; i < deviceCount; ++i) {
        if (isDeviceSuitable(pApp, devices + i)) {
            pApp->pPhysicalDevice = devices + i;
            break;
        }
    }

    if (*pApp->pPhysicalDevice == VK_NULL_HANDLE) {
        printf("Failed to find a suitable GPU\n");
        return 1;
    }

    return 0;
}

int createInstance(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    if (enableValidationLayers && checkValidationLayerSupport()) {
        printf("Validation layers requested, but not available\n");
        return 1;
    }

    pApp->pInstance = malloc(sizeof(VkInstance));
    if (!pApp->pInstance) {
        printf("OOM: dropping\n");
        return 1;
    }

    VkApplicationInfo *pAppInfo = malloc(sizeof(VkApplicationInfo));
    if (!pAppInfo) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pAppInfo, 0, sizeof(VkApplicationInfo));

    pAppInfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    pAppInfo->pApplicationName = "Hello Triangle";
    pAppInfo->applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    pAppInfo->pEngineName = "No Engine";
    pAppInfo->engineVersion = VK_MAKE_VERSION(1, 0, 0);
    pAppInfo->apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo *pCreateInfo = malloc(sizeof(VkInstanceCreateInfo));
    if (!pCreateInfo) {
        printf("OOM: dropping\n");
        return 1;
    }
    memset(pCreateInfo, 0, sizeof(VkInstanceCreateInfo));

    pCreateInfo->sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    pCreateInfo->pApplicationInfo = pAppInfo;

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    glfwExtensions = getRequiredExtensions(&glfwExtensionCount);

    pCreateInfo->enabledExtensionCount = glfwExtensionCount;
    pCreateInfo->ppEnabledExtensionNames = glfwExtensions;
    if (enableValidationLayers) {
        pCreateInfo->enabledLayerCount = (int32_t)validationLayerCount;
        pCreateInfo->ppEnabledLayerNames = validationLayers;
    } else {
        pCreateInfo->enabledLayerCount = 0;
    }

    if (vkCreateInstance(pCreateInfo, NULL, pApp->pInstance) != VK_SUCCESS) {
        printf("Failed to create instance\n");
        return 1;
    }

    /* garbage: for fun */

#ifndef NDEBUG

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    VkExtensionProperties *extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

    printf("Required extensions:\n");
    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
        printf("\t%s\n", *(glfwExtensions + i));
    }

    printf("Available extensions:\n");
    for (uint32_t i = 0; i < extensionCount; ++i) {
        printf("\t%s\n", (extensions + i)->extensionName);
    }

#endif

    return 0;
}

int initWindow(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pApp->pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);

    return 0;
}

int initVulkan(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    if (createInstance(pApp)) {
        printf("Failed to create instance\n");
        return 1;
    }

    if (setupDebugMessenger(pApp)) {
        printf("Failed to setup debug messenger\n");
        return 1;
    }

    if (createSurface(pApp)) {
        printf("Failed to create surface\n");
        return 1;
    }
    
    if (pickPhysicalDevice(pApp)) {
        printf("Failed to pick physical device\n");
        return 1;
    }

    if (createLogicalDevice(pApp)) {
        printf("Failed to create logical device\n");
        return 1;
    }

    if (createSwapChain(pApp)) {
        printf("Failed to create swap chain\n");
        return 1;
    }

    if (createImageViews(pApp)) {
        printf("Fialed to create image views\n");
        return 1;
    }

    if (createRenderPass(pApp)) {
        printf("Failed to create render pass\n");
        return 1;
    }

    if (createGraphicsPipeline(pApp)) {
        printf("Failed to create graphics pipeline\n");
        return 1;
    }

    return 0;
}

int mainLoop(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    while (!glfwWindowShouldClose(pApp->pWindow)) {
        glfwPollEvents();
    }

    return 0;
}

int cleanup(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    vkDestroyPipeline(*(pApp->pDevice), *(pApp->pGraphicsPipeline), NULL);

    vkDestroyPipelineLayout(*(pApp->pDevice), *(pApp->pPipelineLayout), NULL);

    vkDestroyRenderPass(*(pApp->pDevice), *(pApp->pRenderPass), NULL);

    for (uint32_t i = 0; i < pApp->swapChainImagesCount; ++i) {
        vkDestroyImageView(*(pApp->pDevice), *(pApp->swapChainImageViews + i), NULL);
    }

    vkDestroySwapchainKHR(*(pApp->pDevice), *(pApp->pSwapChain), NULL);

    vkDestroyDevice(*(pApp->pDevice), NULL);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(pApp->pInstance, pApp->pDebugMessenger, NULL);
    }

    vkDestroySurfaceKHR(*(pApp->pInstance), *(pApp->pSurface), NULL);
    vkDestroyInstance(*(pApp->pInstance), NULL);

    glfwDestroyWindow(pApp->pWindow);
    glfwTerminate();

    return 0;
}

int run(void *_app) {
    struct HelloTriangleApp *pApp = (struct HelloTriangleApp *)_app;

    if (initWindow(pApp)) {
        printf("Failed to initialize window\n");
        return 1;
    }

    if (initVulkan(pApp)) {
        printf("Failed to initialize vulkan\n");
        return 1;
    }

    if (mainLoop(pApp)) {
        printf("Failure in mainloop\n");
        return 1;
    }

    if (cleanup(pApp)) {
        printf("Failed to do cleanup\n");
        return 1;
    }

    return 0;
}

int init() {
    if (initValidationLayers()) {
        printf("Failed to init required validation layers\n");
        return 1;
    }

    if (initDeviceExtensions()) {
        printf("Failed to init required extensions\n");
        return 1;
    }

    return 0;
}

int main() {

    if (init()) {
        printf("Failed to init\n");
        return 1;
    }

    struct HelloTriangleApp *pApp = malloc(sizeof(struct HelloTriangleApp));

    if (!pApp) {
        printf("OOM: dropping\n");
        return 1;
    }

    if (run(pApp)) {
        printf("Failed to run app\n");
        return 1;
    }

    return 0;
}
