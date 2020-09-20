#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include <stdio.h>
#include <string.h>

/* constants */

const int WIDTH = 800;
const int HEIGHT = 600;

const uint32_t validationLayerCount = 1;
const char **validationLayers; /* see init later in "initValidationLayers" */

#ifdef NDEBUG
const int8_t enableValidationLayers = 0;
#else
const int8_t enableValidationLayers = 1;
#endif

/* to remove gcc's warnings abnout unused something */
int SHUT_UP_GCC_INT;
void *SHUT_UP_GCC_PTR;

/* structures */

struct HelloTriangleApp {
    GLFWwindow *pWindow;
    VkInstance *pInstance;
    VkDebugUtilsMessengerEXT *pDebugMessenger;
};

/* functions */

int initValidationLayers() {
    validationLayers = malloc(sizeof(char *) * validationLayerCount);
    if (!validationLayers) {
        printf("OOM: dropping\n");
        return 1;
    }
    *validationLayers = "VK_LAYER_KHRONOS_validation";

    return 0;
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

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(pApp->pInstance, pApp->pDebugMessenger, NULL);
    }

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
