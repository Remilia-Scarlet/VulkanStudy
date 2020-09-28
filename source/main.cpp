
#include <algorithm>
#include <iostream>
#include <vector>

#include "vulkan/vulkan.h"
#include "common.h"

//准备启用的layer
const char* g_enabled_global_layers[] = { "VK_LAYER_KHRONOS_validation" };

//准备启用的extension
const char* g_enabled_global_extensions[] = {
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME
};
const char* g_enabled_device_extension[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//窗口大小
int g_window_width = 500;
int g_window_height = 500;
HINSTANCE g_hinstance = NULL;
HWND g_hwnd = NULL;

//Vulkan
VkInstance g_instance = VK_NULL_HANDLE;
VkPhysicalDevice g_gpu = VK_NULL_HANDLE;
VkSurfaceKHR g_surface = VK_NULL_HANDLE;
VkDevice g_device = VK_NULL_HANDLE;
VkPhysicalDeviceMemoryProperties g_memory_properties;
std::vector<VkQueueFamilyProperties> g_queue_family_props;
uint32_t g_present_queue_family_index = UINT32_MAX;
VkQueue g_present_queue = VK_NULL_HANDLE;
VkSurfaceFormatKHR g_surface_format;
constexpr int FRAME_LAG = 2;//2帧做双缓冲
VkSemaphore g_image_acquired_semaphores[FRAME_LAG]; //在vkAcquireNextImageKHR时需要带的semaphore，在vkQueueSubmit前等待
VkSemaphore g_draw_complete_semaphores[FRAME_LAG]; //在vkQueueSubmit时带的semaphore，在vkQueuePresentKHR前等待
VkFence g_fences[FRAME_LAG]; //在vkQueueSubmit时带的fence，在每一帧draw的开头等待，确保cpu不会领先gpu多于FRAME_LAG帧
VkCommandPool g_cmd_pool;
VkCommandBuffer g_init_cmd;//这个cb只在初始化时，提供一些特殊cmd的buffer，例如set image layout

void EnumerateGlobalExtAndLayer()
{
	//枚举全局Layer
	uint32_t instance_layer_count = 0;
	VkResult err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
	std::vector<VkLayerProperties> global_layers(instance_layer_count);
	err = vkEnumerateInstanceLayerProperties(&instance_layer_count, global_layers.data());

	std::cout << "InstanceLayer:" << std::endl;
	for (VkLayerProperties& prop : global_layers)
	{
		std::cout << "name:\t" << prop.layerName << std::endl;
		std::cout << "desc:\t" << prop.description << std::endl;
		std::cout << std::endl;
	}

	//枚举全局Extension
	uint32_t instance_extension_count = 0;
	err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);
	std::vector<VkExtensionProperties> global_ext(instance_extension_count);
	err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, global_ext.data());

	std::cout << "InstanceExtension:" << std::endl;
	for (VkExtensionProperties& prop : global_ext)
	{
		std::cout << "name:\t" << prop.extensionName << std::endl;
	}

	//验证我们需要使用的layer和extension是否都存在
	for (auto& to_be_validate : g_enabled_global_layers)
	{
		if (std::find_if(global_layers.begin(), global_layers.end(),
			[to_be_validate](VkLayerProperties layer) {return strcmp(layer.layerName, to_be_validate) == 0; }) == global_layers.end())
		{
			std::cout << "err: Can't find global layer" << to_be_validate << std::endl;
		}
	}
	for (auto& to_be_validate : g_enabled_global_extensions)
	{
		if (std::find_if(global_ext.begin(), global_ext.end(),
			[to_be_validate](VkExtensionProperties ext) {return strcmp(ext.extensionName, to_be_validate) == 0; }) == global_ext.end())
		{
			std::cout << "err: Can't find global extension" << to_be_validate << std::endl;
		}
	}
}

void CreateVkInstance()
{
	const VkApplicationInfo app = {
		/*.sType = */					VK_STRUCTURE_TYPE_APPLICATION_INFO,
		/*.pNext = */					NULL,
		/*.pApplicationName = */		"LearningVulkan",
		/*.applicationVersion = */		0,
		/*.pEngineName = */				"LearningVulkan",
		/*.engineVersion = */			0,
		/*.apiVersion = */				VK_API_VERSION_1_0,
	};
	VkInstanceCreateInfo inst_info = {
		/*.sType = */					VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		/*.pNext = */					NULL,
		/*.flags = */					0,
		/*.pApplicationInfo = */		&app,
		/*.enabledLayerCount = */		sizeof(g_enabled_global_layers) / sizeof(char*),
		/*.ppEnabledLayerNames = */		g_enabled_global_layers,
		/*.enabledExtensionCount = */	sizeof(g_enabled_global_extensions) / sizeof(char*),
		/*.ppEnabledExtensionNames = */	g_enabled_global_extensions,
	};
	vkCreateInstance(&inst_info, NULL, &g_instance);
}

void EnumeratePhysicalDevices()
{
	//获取所有GPU handle
	uint32_t gpu_count;
	vkEnumeratePhysicalDevices(g_instance, &gpu_count, NULL);
	std::vector<VkPhysicalDevice> gpu_list(gpu_count);
	vkEnumeratePhysicalDevices(g_instance, &gpu_count, gpu_list.data());

	//获取GPU属性
	std::cout << "\nGPU count:" << gpu_count << std::endl;
	for (VkPhysicalDevice gpu : gpu_list)
	{
		VkPhysicalDeviceProperties gpu_props;
		vkGetPhysicalDeviceProperties(gpu, &gpu_props);
		VkPhysicalDeviceFeatures gpu_features;
		vkGetPhysicalDeviceFeatures(gpu, &gpu_features);
		std::cout << gpu_props.deviceName << std::endl;
	}
	//通过上面的gpu_props和gpu_features选定一个gpu，这里就直接选0号了
	g_gpu = gpu_list[0];

	//查找gpu的extension
	uint32_t device_extension_count = 0;
	vkEnumerateDeviceExtensionProperties(g_gpu, NULL, &device_extension_count, NULL);
	std::vector<VkExtensionProperties> device_extensions(device_extension_count);
	vkEnumerateDeviceExtensionProperties(g_gpu, NULL, &device_extension_count, device_extensions.data());
	std::cout << "\nDevice extension count:" << device_extension_count << std::endl;
	for (auto& ext : device_extensions)
	{
		std::cout << ext.extensionName << std::endl;
	}

	//验证VK_KHR_swapchain可用
	for (auto& to_be_validate : g_enabled_device_extension)
	{
		if (std::find_if(device_extensions.begin(), device_extensions.end(),
			[to_be_validate](VkExtensionProperties ext) {return strcmp(ext.extensionName, to_be_validate) == 0; }) == device_extensions.end())
		{
			std::cout << "err: This graphic card is not support swapchain" << to_be_validate << std::endl;
		}
	}

	//获取queue family
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(g_gpu, &queue_family_count, NULL);
	g_queue_family_props.resize(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(g_gpu, &queue_family_count, g_queue_family_props.data());
	PrintQueueFamilyProperties(g_queue_family_props);
}

void GetPhysicalDeviceMemoryProperties()
{
	vkGetPhysicalDeviceMemoryProperties(g_gpu, &g_memory_properties);
	
	std::cout << "\nPhysicalDeviceMemoryProperties:" << std::endl;
	std::cout << "memoryHeapCount:" << g_memory_properties.memoryHeapCount << std::endl;
	for(int i = 0 ; i < g_memory_properties.memoryHeapCount; ++i)
	{
		std::cout << "heap "<< i << " size:" << g_memory_properties.memoryHeaps[i].size << " flag: " << VkMemoryHeapFlags2Str(g_memory_properties.memoryHeaps[i].flags).c_str() << std::endl;
	}
	std::cout << "memoryTypeCount:" << g_memory_properties.memoryTypeCount << std::endl;
	for (int i = 0; i < g_memory_properties.memoryTypeCount; ++i)
	{
		std::cout << "type " << i << " heapIndex:" << g_memory_properties.memoryTypes[i].heapIndex << " propertyFlags:" << VkMemoryPropertyFlags2Str(g_memory_properties.memoryTypes[i].propertyFlags).c_str() << std::endl;
	}
}

void PreparePhysicalDevice()
{
	EnumerateGlobalExtAndLayer();
	CreateVkInstance();
	EnumeratePhysicalDevices();
	GetPhysicalDeviceMemoryProperties();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	//case WM_GETMINMAXINFO:  // set window's minimum size
//		((MINMAXINFO *)lParam)->ptMinTrackSize = demo.minsize;
	//	return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_SIZE:
		// Resize the application to the new window size, except when
		// it was minimized. Vulkan doesn't support images or swapchains
		// with width=0 and height=0.
		if (wParam != SIZE_MINIMIZED) {
			g_window_width = lParam & 0xffff;
			g_window_height = (lParam & 0xffff0000) >> 16;
			//	demo_resize();
		}
		break;
	default:
		break;
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void CreateMainWindow()
{
	WNDCLASSEX win_class;

	win_class.cbSize = sizeof(WNDCLASSEX);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = WndProc;
	win_class.cbClsExtra = 0;
	win_class.cbWndExtra = 0;
	win_class.hInstance = g_hinstance;  // hInstance
	win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class.lpszMenuName = NULL;
	win_class.lpszClassName = "LearningVulkan";
	win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	// Register window class:
	if (!RegisterClassEx(&win_class)) {
		// It didn't work, so try to give a useful error:
		printf("Unexpected error trying to start the application!\n");
		fflush(stdout);
		exit(1);
	}
	// Create window with the registered class:
	RECT wr = { 0, 0, g_window_width, g_window_height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	g_hwnd = CreateWindowEx(0,
		"LearningVulkan",            // class name
		"LearningVulkan",            // app name
		WS_OVERLAPPEDWINDOW |  // window style
		WS_VISIBLE | WS_SYSMENU,
		100, 100,            // x/y coords
		wr.right - wr.left,  // width
		wr.bottom - wr.top,  // height
		NULL,                // handle to parent
		NULL,                // handle to menu
		g_hinstance,    // hInstance
		NULL);               // no extra parameters
}

void CreateSurface()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VkWin32SurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.hinstance = g_hinstance;
	createInfo.hwnd = g_hwnd;
	vkCreateWin32SurfaceKHR(g_instance, &createInfo, NULL, &g_surface);
#else
#error to be implemented
#endif
}

void SearchPresentQueueFamilyIndex()
{
	size_t queue_family_count = g_queue_family_props.size();
	std::vector<VkBool32> supports_present(queue_family_count);
	for (uint32_t i = 0; i < queue_family_count; i++) 
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(g_gpu, i, g_surface, &supports_present[i]);
	}

	std::cout << "\nQueue families support present:" << std::endl;
	for(auto& val : supports_present)
	{
		std::cout << val << std::endl;
	}

	//遍历每个family，看看是否既支持graphic，又支持present
	for(size_t i = 0 ; i < queue_family_count; ++i)
	{
		bool is_support_graphic = g_queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
		bool is_support_present = supports_present[i];
		if( is_support_graphic && is_support_present)
		{
			g_present_queue_family_index = i;
			break;
		}
	}
	//大部分显卡和设备都支持同一个queue既做graphic又做present以提高效率。但是需要注意的是，vulkan官方文档
	//并没有规定设备必须提供一个queue同时支持graphic和present，所以graphic queue和present queue有可能是两个不同的queue
	//这种情况我这里就没处理了，处理方案参考官方demo，大意就是在graphic queue提交command buffer，完成后用一个
	//ImageMemoryBarrier做一次ownership转换，把graphic queue的swapchain image的ownership转换到present queue上
	if(g_present_queue_family_index == UINT32_MAX)
	{
		std::cout << "err: can find a queue family that both supports graphic and present" << std::endl;
	}
	else
	{
		std::cout << "\nChoosen graphic and present queue family index:" << std::endl;
		std::cout << g_present_queue_family_index << std::endl;
	}
}

void CreateDevice()
{
	float queue_priorities[1] = { 0.0 };
	VkDeviceQueueCreateInfo queue_create_info;
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.pNext = NULL;
	queue_create_info.queueFamilyIndex = g_present_queue_family_index;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = queue_priorities;
	queue_create_info.flags = 0;

	VkDeviceCreateInfo device;
	device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device.pNext = NULL;
	device.flags = 0;
	device.queueCreateInfoCount = 1;
	device.pQueueCreateInfos = &queue_create_info;
	device.enabledLayerCount = 0;  //文档说这个参数被"deprecated and ignored"，然而实测(NV驱动456.38; 09/17/2020)这个参数必须是0。如果不是0，接下来就会去读ppEnabledLayerNames导致内存崩溃，妈卖批……
	device.ppEnabledLayerNames = NULL;
	device.enabledExtensionCount = sizeof(g_enabled_device_extension) / sizeof(char*);
	device.ppEnabledExtensionNames = g_enabled_device_extension;
	device.pEnabledFeatures = NULL;

	vkCreateDevice(g_gpu, &device, NULL, &g_device);
}

void CreatePresentQueue()
{
	//通过之前查找到的present_queue_family_index，正式创建VkQueue。
	//注意如果这里present queue和graphic queue不是同一个的话，需要创建不同的VkQueue
	vkGetDeviceQueue(g_device, g_present_queue_family_index, 0, &g_present_queue);
}

void ChooseSurfaceFormat()
{
	//列举这块gpu支持的所有格式
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(g_gpu, g_surface, &format_count, NULL);
	std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(g_gpu, g_surface, &format_count, surface_formats.data());

	std::cout << "\nSupport formats count: " << format_count << std::endl;
	for(auto& val : surface_formats)
	{
		std::cout << "format:" << VkFormat2Str(val.format) << "  color space:" << VkColorSpace2Str(val.colorSpace) << std::endl;
	}
	//挑一个我们想用的格式。我们就用R8G8B8A8好了
	for(VkSurfaceFormatKHR format : surface_formats)
	{
		if (format.format == VK_FORMAT_R8G8B8A8_UNORM || format.colorSpace == VK_FORMAT_B8G8R8A8_UNORM)
		{
			g_surface_format = format;
		}
	}
}

void CreateSemaphoreAndFence()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {
	/*.sType = */VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	/*.pNext = */NULL,
	/*.flags = */0,
	};
	VkFenceCreateInfo fence_ci = {
		/*.sType = */VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags = */VK_FENCE_CREATE_SIGNALED_BIT
	};
	for (uint32_t i = 0; i < FRAME_LAG; i++) 
	{
		vkCreateSemaphore(g_device, &semaphoreCreateInfo, NULL, &g_image_acquired_semaphores[i]);
		vkCreateSemaphore(g_device, &semaphoreCreateInfo, NULL, &g_draw_complete_semaphores[i]);
		vkCreateFence(g_device, &fence_ci, NULL, &g_fences[i]);
	}
}

void CreateCommandPool()
{
	const VkCommandPoolCreateInfo cmd_pool_info = {
	/*.sType = */VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	/*.pNext = */NULL,
	/*.queueFamilyIndex = */g_present_queue_family_index,
	/*.flags = */0,
	};
	vkCreateCommandPool(g_device, &cmd_pool_info, NULL, &g_cmd_pool);
}

void CreateCommandBufferForInit()
{
	const VkCommandBufferAllocateInfo cmd = {
	/*.sType = */VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	/*.pNext = */NULL,
	/*.commandPool = */g_cmd_pool,
	/*.level = */VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	/*.commandBufferCount = */1,
	};
	vkAllocateCommandBuffers(g_device, &cmd, &g_init_cmd);

	VkCommandBufferBeginInfo cmd_buf_info = {
	/*.sType = */VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	/*.pNext = */NULL,
	/*.flags = */0,
	/*.pInheritanceInfo = */NULL,
	};
	vkBeginCommandBuffer(g_init_cmd, &cmd_buf_info);
}

void GetSurfaceCapabilities()
{
	
	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_gpu, g_surface, &surface_capabilities);

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(g_gpu, g_surface, &present_mode_count, NULL);
	std::vector<VkPresentModeKHR> present_mode(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(g_gpu, g_surface, &present_mode_count, present_mode.data());
	std::cout << "\nSupport present mode:" << std::endl;
	for(auto& mode : present_mode)
	{
		std::cout << VkPresentModeKHR2Str(mode) << std::endl;
	}

}

void PrepareSwapchain()
{
	CreateMainWindow();
	CreateSurface();
	SearchPresentQueueFamilyIndex();
	CreateDevice();
	CreatePresentQueue();
	ChooseSurfaceFormat();
	CreateSemaphoreAndFence();
	CreateCommandPool();
	CreateCommandBufferForInit();
	GetSurfaceCapabilities();
}

int main()
{
	g_hinstance = g_hinstance;
	
	PreparePhysicalDevice();
	PrepareSwapchain();

	return 0;
} 