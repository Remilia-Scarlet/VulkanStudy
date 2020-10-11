#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#include "vulkan/vulkan.h"
#include "common.h"

#include "CubeTexture.h"

//准备启用的layer
const char* g_enabled_global_layers[] = {"VK_LAYER_KHRONOS_validation"};

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
constexpr int FRAME_LAG = 2; //2帧做双缓冲
VkSemaphore g_image_acquired_semaphores[FRAME_LAG]; //在vkAcquireNextImageKHR时需要带的semaphore，在vkQueueSubmit前等待
VkSemaphore g_draw_complete_semaphores[FRAME_LAG]; //在vkQueueSubmit时带的semaphore，在vkQueuePresentKHR前等待
VkFence g_fences[FRAME_LAG]; //在vkQueueSubmit时带的fence，在每一帧draw的开头等待，确保cpu不会领先gpu多于FRAME_LAG帧
VkCommandPool g_cmd_pool;
VkCommandBuffer g_init_cmd = VK_NULL_HANDLE; //这个cb只在初始化时，提供一些特殊cmd的buffer，例如set image layout
VkSwapchainKHR g_swapchain = VK_NULL_HANDLE;

//每个swapchain对应一个SwapchainImageResources，里面储存了swapchain的image，cb，uniform buffer等
struct SwapchainImageResources
{
	VkImage image;
	VkImageView view;
};
std::vector<SwapchainImageResources> g_swapchain_image_resources;

//深度缓冲相关
struct Depth
{
	VkFormat format;
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
};
Depth g_depth;

//盒子的贴图
struct Texture
{
	int32_t tex_width, tex_height;
	VkSampler sampler;
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
};
Texture g_texture;

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
		/*.sType = */ VK_STRUCTURE_TYPE_APPLICATION_INFO,
		/*.pNext = */ NULL,
		/*.pApplicationName = */ "LearningVulkan",
		/*.applicationVersion = */ 0,
		/*.pEngineName = */ "LearningVulkan",
		/*.engineVersion = */ 0,
		/*.apiVersion = */ VK_API_VERSION_1_0,
	};
	VkInstanceCreateInfo inst_info = {
		/*.sType = */ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		/*.pNext = */ NULL,
		/*.flags = */ 0,
		/*.pApplicationInfo = */ &app,
		/*.enabledLayerCount = */ ARRAY_SIZE(g_enabled_global_layers),
		/*.ppEnabledLayerNames = */ g_enabled_global_layers,
		/*.enabledExtensionCount = */ ARRAY_SIZE(g_enabled_global_extensions),
		/*.ppEnabledExtensionNames = */ g_enabled_global_extensions,
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		//case WM_GETMINMAXINFO:  // set window's minimum size
		//		((MINMAXINFO *)lParam)->ptMinTrackSize = demo.minsize;
		//	return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_SIZE:
		// Resize the application to the new window size, except when
		// it was minimized. Vulkan doesn't support images or swapchains
		// with width=0 and height=0.
		if (wParam != SIZE_MINIMIZED)
		{
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
	win_class.hInstance = g_hinstance; // hInstance
	win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class.lpszMenuName = NULL;
	win_class.lpszClassName = "LearningVulkan";
	win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	// Register window class:
	if (!RegisterClassEx(&win_class))
	{
		// It didn't work, so try to give a useful error:
		printf("Unexpected error trying to start the application!\n");
		fflush(stdout);
		exit(1);
	}
	// Create window with the registered class:
	RECT wr = {0, 0, g_window_width, g_window_height};
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	g_hwnd = CreateWindowEx(0,
	                        "LearningVulkan", // class name
	                        "LearningVulkan", // app name
	                        WS_OVERLAPPEDWINDOW | // window style
	                        WS_VISIBLE | WS_SYSMENU,
	                        100, 100, // x/y coords
	                        wr.right - wr.left, // width
	                        wr.bottom - wr.top, // height
	                        NULL, // handle to parent
	                        NULL, // handle to menu
	                        g_hinstance, // hInstance
	                        NULL); // no extra parameters
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
	for (auto& val : supports_present)
	{
		std::cout << val << std::endl;
	}

	//遍历每个family，看看是否既支持graphic，又支持present
	for (size_t i = 0; i < queue_family_count; ++i)
	{
		bool is_support_graphic = g_queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
		bool is_support_present = supports_present[i];
		if (is_support_graphic && is_support_present)
		{
			g_present_queue_family_index = i;
			break;
		}
	}
	//大部分显卡和设备都支持同一个queue既做graphic又做present以提高效率。但是需要注意的是，vulkan官方文档
	//并没有规定设备必须提供一个queue同时支持graphic和present，所以graphic queue和present queue有可能是两个不同的queue
	//这种情况我这里就没处理了，处理方案参考官方demo，大意就是在graphic queue提交command buffer，完成后用一个
	//ImageMemoryBarrier做一次ownership转换，把graphic queue的swapchain image的ownership转换到present queue上
	if (g_present_queue_family_index == UINT32_MAX)
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
	float queue_priorities[1] = {0.0};
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
	device.enabledLayerCount = 0;
	//文档说这个参数被"deprecated and ignored"，然而实测(NV驱动456.38; 09/17/2020)这个参数必须是0。如果不是0，接下来就会去读ppEnabledLayerNames导致内存崩溃，妈卖批……
	device.ppEnabledLayerNames = NULL;
	device.enabledExtensionCount = ARRAY_SIZE(g_enabled_device_extension);
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
	//挑一个我们想用的格式。我们就用R8G8B8A8或者B8G8R8A8好了
	for(VkSurfaceFormatKHR format : surface_formats)
	{
		if (format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
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

void PrepareCommandBuffer()
{
	CreateCommandPool();
	CreateCommandBufferForInit();
}

VkSurfaceCapabilitiesKHR GetSurfaceCapabilities()
{
	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_gpu, g_surface, &surface_capabilities);

	//获取到了surface的currentExtent，min/maxImageExtent 等信息，这里需要确定等会我们要创建的swapchain的extent大小
	std::cout << "\nsurface_capabilities:" << std::endl;
	std::cout << "minImageCount:" << surface_capabilities.minImageCount << std::endl;
	std::cout << "maxImageCount:" << surface_capabilities.maxImageCount << std::endl;
	std::cout << "currentExtent: w:" << surface_capabilities.currentExtent.width << " h:" << surface_capabilities.currentExtent.height << std::endl;
	std::cout << "minImageExtent: w:" << surface_capabilities.minImageExtent.width << " h:" << surface_capabilities.minImageExtent.height << std::endl;
	std::cout << "maxImageExtent: w:" << surface_capabilities.maxImageExtent.width << " h:" << surface_capabilities.maxImageExtent.height << std::endl;
	std::cout << "supportedTransforms:" << VkSurfaceTransformFlagKHR2Str(surface_capabilities.supportedTransforms).c_str() << std::endl;
	std::cout << "supportedCompositeAlpha:" << VkCompositeAlphaFlagBitsKHR2Str(surface_capabilities.supportedCompositeAlpha).c_str() << std::endl;
	return surface_capabilities;
}

VkExtent2D GetSwapchainExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
	VkExtent2D swapchain_extent; //等会创建swapchain的大小
	if (surface_capabilities.currentExtent.width == 0xFFFFFFFF)
	{
		//如果这里surface_capabilities获取出来是0xffffffff，那就说明surface大小会取决于swapchain大小，我们在这里自行根据min/max自行决定swapchain大小。
		swapchain_extent.width = g_window_width;
		swapchain_extent.height = g_window_height;

		if (swapchain_extent.width < surface_capabilities.minImageExtent.width)
		{
			swapchain_extent.width = surface_capabilities.minImageExtent.width;
		}
		else if (swapchain_extent.width > surface_capabilities.maxImageExtent.width)
		{
			swapchain_extent.width = surface_capabilities.maxImageExtent.width;
		}

		if (swapchain_extent.height < surface_capabilities.minImageExtent.height)
		{
			swapchain_extent.height = surface_capabilities.minImageExtent.height;
		}
		else if (swapchain_extent.height > surface_capabilities.maxImageExtent.height)
		{
			swapchain_extent.height = surface_capabilities.maxImageExtent.height;
		}
	}
	else
	{
		// 如果surface的大小是确定的，那么swapchain的大小必须一致
		swapchain_extent = surface_capabilities.currentExtent;
		g_window_width = surface_capabilities.currentExtent.width;
		g_window_height = surface_capabilities.currentExtent.height;
	}
	return swapchain_extent;
}

int GetDesiredNumOfSwapchainImages(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
	//双缓冲
	uint32_t desired_num_of_swapchain_images = 2;
	if (desired_num_of_swapchain_images < surface_capabilities.minImageCount)
	{
		desired_num_of_swapchain_images = surface_capabilities.minImageCount;
	}
	// 如果maxImageCount是0，那就没有最大image数限制，否则我们不能超过最大值。
	if ((surface_capabilities.maxImageCount > 0) && (desired_num_of_swapchain_images > surface_capabilities.
		maxImageCount))
	{
		desired_num_of_swapchain_images = surface_capabilities.maxImageCount;
	}
	return desired_num_of_swapchain_images;
}

VkSurfaceTransformFlagBitsKHR GetSurfaceTransformFlags(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
	//pretransform就是surface现在是不是90度旋转的，还是180度旋转的，还是镜像的这些
	VkSurfaceTransformFlagBitsKHR preTransform;
	if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surface_capabilities.currentTransform;
	}
	return preTransform;
}

VkCompositeAlphaFlagBitsKHR GetCompositeAlphaFlag(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
	// Alpha与颜色的预乘关系
	VkCompositeAlphaFlagBitsKHR desired_composite_alpha = 
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;				//不乘以alpha，直接使用color值，如果有alpha通道，忽略它并把它视作1.0
		//VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;	//合成器假设非alpha通道已经被程序预乘了alpha
		//VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;	//合成器在合成过程中会乘以alpha
		//VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;			//由本机的操作系统进行合成，vulkan引擎未知alpha合成方式

	if(surface_capabilities.supportedCompositeAlpha != desired_composite_alpha)
	{
		//error!
	}
	
	return desired_composite_alpha;
}

VkPresentModeKHR SelectPresentMode()
{
	//选一个present mode
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(g_gpu, g_surface, &present_mode_count, NULL);
	std::vector<VkPresentModeKHR> present_mode(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(g_gpu, g_surface, &present_mode_count, present_mode.data());
	std::cout << "\nSupport present mode:" << std::endl;
	for (auto& mode : present_mode)
	{
		std::cout << VkPresentModeKHR2Str(mode) << std::endl;
	}
	//FIFO是spec保证必须要有的模式，咱们不如就选这个模式了
	VkPresentModeKHR desired_mode = VK_PRESENT_MODE_FIFO_KHR;
	if (std::find(present_mode.begin(), present_mode.end(), desired_mode) == present_mode.end())
	{
		std::cout << "err: How dare you don't support VK_PRESENT_MODE_FIFO_KHR?" << std::endl;
	}
	return desired_mode;
}

void CreateSwapchain(VkExtent2D swapchain_extent, int desired_num_of_swapchain_images, VkSurfaceTransformFlagBitsKHR pre_transform, VkCompositeAlphaFlagBitsKHR composite_alpha, VkPresentModeKHR present_mode)
{
	VkSwapchainKHR oldSwapchain = g_swapchain;
	VkSwapchainCreateInfoKHR swapchain_ci = {
		/*.sType = */VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		/*.pNext = */NULL,
		/*.flags = */0,
		/*.surface = */g_surface,
		/*.minImageCount = */desired_num_of_swapchain_images,
		/*.imageFormat = */g_surface_format.format,
		/*.imageColorSpace = */g_surface_format.colorSpace,
		/*.imageExtent = */swapchain_extent,
		/*.imageArrayLayers = */1,
		/*.imageUsage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		/*.imageSharingMode = */VK_SHARING_MODE_EXCLUSIVE,
		/*.queueFamilyIndexCount = */0,
		/*.pQueueFamilyIndices = */NULL,
		/*.preTransform = */pre_transform,
		/*.compositeAlpha = */composite_alpha,
		/*.presentMode = */present_mode,
		/*.clipped = */true,
		/*.oldSwapchain = */oldSwapchain,
	};
	vkCreateSwapchainKHR(g_device, &swapchain_ci, NULL, &g_swapchain);

	if (oldSwapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(g_device, oldSwapchain, NULL);
	}
}

void CreateSwapchainImageView()
{
	//首先把所有swapchain的VkImage获取出来
	uint32_t swapchain_image_count;
	vkGetSwapchainImagesKHR(g_device, g_swapchain, &swapchain_image_count, NULL);
	std::vector<VkImage> swapchain_images(swapchain_image_count);
	vkGetSwapchainImagesKHR(g_device, g_swapchain, &swapchain_image_count, swapchain_images.data());

	//储存到我们的swapchain_image_resources结构体中
	g_swapchain_image_resources.resize(swapchain_image_count);
	for(uint32_t i = 0 ; i < g_swapchain_image_resources.size(); ++i)
	{
		g_swapchain_image_resources[i].image = swapchain_images[i];
	}
	//对每个image创建一个view
	for (int i = 0; i < g_swapchain_image_resources.size(); i++) {
		VkImageViewCreateInfo color_image_view = {
			/*.sType = */VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			/*.pNext = */NULL,
			/*.flags = */0,
			/*.image = */g_swapchain_image_resources[i].image,
			/*.viewType = */VK_IMAGE_VIEW_TYPE_2D,
			/*.format = */g_surface_format.format,
			/*.components = */{
					/*.r = */VK_COMPONENT_SWIZZLE_R,
					/*.g = */VK_COMPONENT_SWIZZLE_G,
					/*.b = */VK_COMPONENT_SWIZZLE_B,
					/*.a = */VK_COMPONENT_SWIZZLE_A,
			},
			/*.subresourceRange =*/{
					/*.aspectMask = */VK_IMAGE_ASPECT_COLOR_BIT,
					/*.baseMipLevel = */0,
					/*.levelCount = */1,
					/*.baseArrayLayer = */0,
					/*.layerCount = */1
			},
		};
		vkCreateImageView(g_device, &color_image_view, NULL, &g_swapchain_image_resources[i].view);
	}
}

void InitSwapchain()
{
	VkSurfaceCapabilitiesKHR capabilities = GetSurfaceCapabilities();
	VkExtent2D swapchain_extent = GetSwapchainExtent(capabilities);
	int desired_num_of_swapchain_images = GetDesiredNumOfSwapchainImages(capabilities);
	VkSurfaceTransformFlagBitsKHR pre_transform = GetSurfaceTransformFlags(capabilities);
	VkCompositeAlphaFlagBitsKHR composite_alpha = GetCompositeAlphaFlag(capabilities);
	VkPresentModeKHR present_mode = SelectPresentMode();
	CreateSwapchain(swapchain_extent, desired_num_of_swapchain_images, pre_transform, composite_alpha, present_mode);
	CreateSwapchainImageView();
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
	PrepareCommandBuffer();
	InitSwapchain();
}

int GetMemoryTypeFromProperties(uint32_t typeBits, VkFlags requirements_mask)
{
	//找到第一个满足所有requirement的type
	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
		if ((typeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if ((g_memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
				return i;
			}
		}
		typeBits >>= 1;
	}
	//这里需要错误处理
	std::cout << "err: can't find any memory type that matches flag: " << requirements_mask << std::endl;
	return -1;
}

void PrepareDepth()
{
	//创建VkImage
	g_depth.format = VK_FORMAT_D16_UNORM;
	const VkImageCreateInfo ci = {
		/*.sType = */VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags = */0,
		/*.imageType = */VK_IMAGE_TYPE_2D,
		/*.format = */g_depth.format,
		/*.extent = */{g_window_width, g_window_height, 1},
		/*.mipLevels = */1,
		/*.arrayLayers = */1,
		/*.samples = */VK_SAMPLE_COUNT_1_BIT,
		/*.tiling = */VK_IMAGE_TILING_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		/*.sharingMode = */VK_SHARING_MODE_EXCLUSIVE,
		/*.queueFamilyIndexCount = */0,
		/*.pQueueFamilyIndices = */NULL,
		/*.initialLayout = */VK_IMAGE_LAYOUT_UNDEFINED
	};
	vkCreateImage(g_device, &ci, NULL, &g_depth.image);

	//获取内存需求
	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(g_device, g_depth.image, &mem_reqs);

	//分配内存
	uint32_t memory_type_index = GetMemoryTypeFromProperties(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkMemoryAllocateInfo mem_alloc_info = {
		/*sType*/ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		/*pNext*/ NULL,
		/*allocationSize*/ mem_reqs.size,
		/*memoryTypeIndex*/ memory_type_index
	};
	vkAllocateMemory(g_device, &mem_alloc_info, NULL, &g_depth.mem);

	//绑定内存到VkImage
	vkBindImageMemory(g_device, g_depth.image, g_depth.mem, 0);

	//创建ImageView
	VkImageViewCreateInfo view = {
		/*.sType = */VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags =*/ 0,
		/*.image = */g_depth.image,
		/*.viewType = */VK_IMAGE_VIEW_TYPE_2D,
		/*.format = */g_depth.format,
		/*.components =*/
		{
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
		},
		/*.subresourceRange =*/
		{
			/*.aspectMask = */VK_IMAGE_ASPECT_DEPTH_BIT,
			/*.baseMipLevel = */0,
			/*.levelCount = */1,
			/*.baseArrayLayer = */0,
			/*.layerCount = */1
		}
	};
	vkCreateImageView(g_device, &view, NULL, &g_depth.view);
}

void PrepareCubeTexture()
{
	//我们的texture是R8G8B8A8格式
	const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageTiling tiling = VK_IMAGE_TILING_LINEAR;
	const VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	//获取对应格式的FormatProperties
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(g_gpu, tex_format, &props);
	PrintPhysicalDeviceFormatProperties(tex_format, props);
	//验证这个gpu是可以处理我们这个格式的
	if (props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
	{
		std::cout << "err: No support for R8G8B8A8_UNORM as texture image format" << std::endl;
	}

	int32_t tex_width;
	int32_t tex_height;
	//首先把其他参数填为NULL，只获取图片长宽
	if (!loadTexture(NULL, 0, &tex_width, &tex_height)) 
	{
		std::cout << "err: Failed to load textures" << std::endl;;
	}
	g_texture.tex_width = tex_width;
	g_texture.tex_height = tex_height;

	//先把VkImage创建出来
	const VkImageCreateInfo image_create_info = {
		/*.sType = */VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags = */0u,
		/*.imageType = */VK_IMAGE_TYPE_2D,
		/*.format = */tex_format,
		/*.extent = */{tex_width, tex_height, 1},
		/*.mipLevels = */1,
		/*.arrayLayers = */1,
		/*.samples = */VK_SAMPLE_COUNT_1_BIT,
		/*.tiling = */tiling,
		/*.usage = */usage,
		/*.sharingMode = */VK_SHARING_MODE_EXCLUSIVE,
		/*.queueFamilyIndexCount = */ 0u,
		/*.pQueueFamilyIndices = */ NULL,
		/*.initialLayout = */VK_IMAGE_LAYOUT_PREINITIALIZED,
	};
	vkCreateImage(g_device, &image_create_info, NULL, &g_texture.image);

	//获取内存需求
	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(g_device, g_texture.image, &mem_reqs);

	//分配内存
	const VkFlags required_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32_t memory_type_index = GetMemoryTypeFromProperties(mem_reqs.memoryTypeBits, required_props);
	VkMemoryAllocateInfo mem_alloc_info = {
		/*sType*/ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		/*pNext*/ NULL,
		/*allocationSize*/ mem_reqs.size,
		/*memoryTypeIndex*/ memory_type_index
	};
	vkAllocateMemory(g_device, &mem_alloc_info, NULL, &g_texture.mem);

	//绑定内存到VkImage
	vkBindImageMemory(g_device, g_texture.image, g_texture.mem, 0);

	//读取图像内容，map到texture memory中
	assert(required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); //注意这里必须是host visible的，才可以map/unmap写入
	const VkImageSubresource subres = {
		/*.aspectMask = */VK_IMAGE_ASPECT_COLOR_BIT,
		/*.mipLevel = */0,
		/*.arrayLayer = */0,
	};
	VkSubresourceLayout layout;
	vkGetImageSubresourceLayout(g_device, g_texture.image, &subres, &layout); //获取layout是为了得到rowPitch

	void* data;
	vkMapMemory(g_device, g_texture.mem, 0, mem_alloc_info.allocationSize, 0, &data);
	if (!loadTexture((uint8_t*)data, layout.rowPitch, &tex_width, &tex_height)) 
	{
		std::cout << "err: Failed to load texture" << std::endl;;
	}
	vkUnmapMemory(g_device, g_texture.mem);

	//下面开始设置image layout
	const VkImageLayout old_image_layout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	const VkImageLayout new_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkImageMemoryBarrier image_memory_barrier = {
		/*.sType = */VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		/*.pNext = */NULL,
		/*.srcAccessMask = */0,
		/*.dstAccessMask = */VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
		/*.oldLayout = */old_image_layout,
		/*.newLayout = */new_image_layout,
		/*.srcQueueFamilyIndex = */0,
		/*.dstQueueFamilyIndex = */0,
		/*.image = */g_texture.image,
		/*.subresourceRange = */{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};
	//由于涉及到texture transition，我们必须设置image memory barrier
	vkCmdPipelineBarrier(g_init_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);

	//开始创建sampler
	const VkSamplerCreateInfo sampler_ci = {
		/*.sType = */VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags = */0,
		/*.magFilter = */VK_FILTER_NEAREST,
		/*.minFilter = */VK_FILTER_NEAREST,
		/*.mipmapMode = */VK_SAMPLER_MIPMAP_MODE_NEAREST,
		/*.addressModeU = */VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/*.addressModeV = */VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/*.addressModeW = */VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/*.mipLodBias = */0.0f,
		/*.anisotropyEnable = */VK_FALSE,
		/*.maxAnisotropy = */1,
		/*.compareEnable = */VK_FALSE,
		/*.compareOp = */VK_COMPARE_OP_NEVER,
		/*.minLod = */0.0f,
		/*.maxLod = */0.0f,
		/*.borderColor = */VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		/*.unnormalizedCoordinates = */VK_FALSE,
	};
	vkCreateSampler(g_device, &sampler_ci, NULL, &g_texture.sampler);

	//创建image view
	VkImageViewCreateInfo view = {
		/*.sType = */VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags = */0,
		/*.image = */g_texture.image,
		/*.viewType = */VK_IMAGE_VIEW_TYPE_2D,
		/*.format = */tex_format,
		/*.components =*/
			{
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A,
			},
		/*.subresourceRange = */{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
	};
	vkCreateImageView(g_device, &view, NULL, &g_texture.view);
}

void PrepareDataBuffer()
{
	
}

void PrepareRenderResource()
{
	PrepareDepth();
	PrepareCubeTexture();
	PrepareDataBuffer();
}

int main()
{
	g_hinstance = g_hinstance;

	PreparePhysicalDevice();
	PrepareSwapchain();
	PrepareRenderResource();

	return 0;
}
