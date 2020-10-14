#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#include "vulkan/vulkan.h"
#include "common.h"

#include "linmath.h"
#include "shaders.h"

#include "CubeTexture.h"

//准备启用的layer
const char* g_enabled_global_layers[] = {
	"VK_LAYER_KHRONOS_validation",
	//"VK_LAYER_LUNARG_api_dump"
};

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

//视口，矩阵，旋转相关
mat4x4 g_projection_matrix;
mat4x4 g_view_matrix;
mat4x4 g_model_matrix;
float g_spin_angle = 4;

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
int g_frame_index = 0;//当前要用的fence，信号量的index
uint32_t g_current_buffer = 0;//当前的swapchain buffer
VkSemaphore g_image_acquired_semaphores[FRAME_LAG]; //在vkAcquireNextImageKHR时需要带的semaphore，在vkQueueSubmit前等待
VkSemaphore g_draw_complete_semaphores[FRAME_LAG]; //在vkQueueSubmit时带的semaphore，在vkQueuePresentKHR前等待
VkFence g_fences[FRAME_LAG]; //在vkQueueSubmit时带的fence，在每一帧draw的开头等待，确保cpu不会领先gpu多于FRAME_LAG帧
VkCommandPool g_cmd_pool;
VkCommandBuffer g_init_cmd = VK_NULL_HANDLE; //这个cb只在初始化时，提供一些特殊cmd的buffer，例如set image layout
VkSwapchainKHR g_swapchain = VK_NULL_HANDLE;
VkDescriptorSetLayout g_desc_layout;
VkPipelineLayout g_pipeline_layout;
VkRenderPass g_render_pass;
VkPipelineCache g_pipeline_cache;
VkPipeline g_pipeline;
VkDescriptorPool g_desc_pool;

//每个swapchain对应一个SwapchainImageResources，里面储存了swapchain的image，cb，uniform buffer等
struct SwapchainImageResources
{
	VkImage image;
	VkImageView view;

	VkBuffer uniform_buffer;
	VkDeviceMemory uniform_memory;
	void *uniform_memory_ptr;
	VkFramebuffer framebuffer;

	VkCommandBuffer cmd;

	VkDescriptorSet descriptor_set;
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

//vs的uniform buffer
struct VsUniformStruct {
	// Must start with MVP
	float mvp[4][4];
	float position[12 * 3][4];
	float attr[12 * 3][4];
};
VsUniformStruct g_vs_uniform_struct;


//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
static const float g_vertex_buffer_data[] = {
	-1.0f,-1.0f,-1.0f,  // -X side
	-1.0f,-1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,

	-1.0f,-1.0f,-1.0f,  // -Z side
	 1.0f, 1.0f,-1.0f,
	 1.0f,-1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f,-1.0f,
	 1.0f, 1.0f,-1.0f,

	-1.0f,-1.0f,-1.0f,  // -Y side
	 1.0f,-1.0f,-1.0f,
	 1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,
	 1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,

	-1.0f, 1.0f,-1.0f,  // +Y side
	-1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f,-1.0f,

	 1.0f, 1.0f,-1.0f,  // +X side
	 1.0f, 1.0f, 1.0f,
	 1.0f,-1.0f, 1.0f,
	 1.0f,-1.0f, 1.0f,
	 1.0f,-1.0f,-1.0f,
	 1.0f, 1.0f,-1.0f,

	-1.0f, 1.0f, 1.0f,  // +Z side
	-1.0f,-1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,
	 1.0f,-1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = {
	0.0f, 1.0f,  // -X side
	1.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,

	1.0f, 1.0f,  // -Z side
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,

	1.0f, 0.0f,  // -Y side
	1.0f, 1.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	0.0f, 0.0f,

	1.0f, 0.0f,  // +Y side
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,

	1.0f, 0.0f,  // +X side
	0.0f, 0.0f,
	0.0f, 1.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,

	0.0f, 0.0f,  // +Z side
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
};

void EnumerateGlobalExtAndLayer()
{
	//枚举全局Layer
	uint32_t instance_layer_count = 0;
	VkResult err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
	assert(err == VK_SUCCESS);
	std::vector<VkLayerProperties> global_layers(instance_layer_count);
	err = vkEnumerateInstanceLayerProperties(&instance_layer_count, global_layers.data());
	assert(err == VK_SUCCESS);

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
	assert(err == VK_SUCCESS);
	std::vector<VkExtensionProperties> global_ext(instance_extension_count);
	err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, global_ext.data());
	assert(err == VK_SUCCESS);

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
	VkResult err;
	err = vkCreateInstance(&inst_info, NULL, &g_instance);
	assert(err == VK_SUCCESS);
}

void EnumeratePhysicalDevices()
{
	VkResult err;
	//获取所有GPU handle
	uint32_t gpu_count;
	err = vkEnumeratePhysicalDevices(g_instance, &gpu_count, NULL);
	assert(err == VK_SUCCESS);
	std::vector<VkPhysicalDevice> gpu_list(gpu_count);
	err = vkEnumeratePhysicalDevices(g_instance, &gpu_count, gpu_list.data());
	assert(err == VK_SUCCESS);

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
	err = vkEnumerateDeviceExtensionProperties(g_gpu, NULL, &device_extension_count, NULL);
	assert(err == VK_SUCCESS);
	std::vector<VkExtensionProperties> device_extensions(device_extension_count);
	err = vkEnumerateDeviceExtensionProperties(g_gpu, NULL, &device_extension_count, device_extensions.data());
	assert(err == VK_SUCCESS);
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
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
		//case WM_GETMINMAXINFO:  // set window's minimum size
		//		((MINMAXINFO *)lParam)->ptMinTrackSize = demo.minsize;
		//	return 0;
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
	VkResult err = vkCreateWin32SurfaceKHR(g_instance, &createInfo, NULL, &g_surface);
	assert(err == VK_SUCCESS);
#else
#error to be implemented
#endif
}

void SearchPresentQueueFamilyIndex()
{
	VkResult err;
	size_t queue_family_count = g_queue_family_props.size();
	std::vector<VkBool32> supports_present(queue_family_count);
	for (uint32_t i = 0; i < queue_family_count; i++)
	{
		err = vkGetPhysicalDeviceSurfaceSupportKHR(g_gpu, i, g_surface, &supports_present[i]);
		assert(err == VK_SUCCESS);
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

	VkResult err;
	err = vkCreateDevice(g_gpu, &device, NULL, &g_device);
	assert(err == VK_SUCCESS);
}

void CreatePresentQueue()
{
	//通过之前查找到的present_queue_family_index，正式创建VkQueue。
	//注意如果这里present queue和graphic queue不是同一个的话，需要创建不同的VkQueue
	vkGetDeviceQueue(g_device, g_present_queue_family_index, 0, &g_present_queue);
}

void ChooseSurfaceFormat()
{
	VkResult err;
	//列举这块gpu支持的所有格式
	uint32_t format_count;
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(g_gpu, g_surface, &format_count, NULL);
	assert(err == VK_SUCCESS);
	std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(g_gpu, g_surface, &format_count, surface_formats.data());
	assert(err == VK_SUCCESS);

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
	VkResult err;
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
		err = vkCreateSemaphore(g_device, &semaphoreCreateInfo, NULL, &g_image_acquired_semaphores[i]);
		assert(err == VK_SUCCESS);
		err = vkCreateSemaphore(g_device, &semaphoreCreateInfo, NULL, &g_draw_complete_semaphores[i]);
		assert(err == VK_SUCCESS);
		err = vkCreateFence(g_device, &fence_ci, NULL, &g_fences[i]);
		assert(err == VK_SUCCESS);
	}
}

void CreateCommandPool()
{
	VkResult err;
	const VkCommandPoolCreateInfo cmd_pool_info = {
		/*.sType = */VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		/*.pNext = */NULL,
		/*.queueFamilyIndex = */g_present_queue_family_index,
		/*.flags = */0,
	};
	err = vkCreateCommandPool(g_device, &cmd_pool_info, NULL, &g_cmd_pool);
	assert(err == VK_SUCCESS);
}

void CreateCommandBufferForInit()
{
	VkResult err;
	const VkCommandBufferAllocateInfo cmd = {
		/*.sType = */VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		/*.pNext = */NULL,
		/*.commandPool = */g_cmd_pool,
		/*.level = */VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		/*.commandBufferCount = */1,
	};
	err = vkAllocateCommandBuffers(g_device, &cmd, &g_init_cmd);
	assert(err == VK_SUCCESS);

	VkCommandBufferBeginInfo cmd_buf_info = {
		/*.sType = */VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		/*.pNext = */NULL,
		/*.flags = */0,
		/*.pInheritanceInfo = */NULL,
	};
	err = vkBeginCommandBuffer(g_init_cmd, &cmd_buf_info);
	assert(err == VK_SUCCESS);
}

void PrepareCommandBuffer()
{
	CreateCommandPool();
	CreateCommandBufferForInit();
}

VkSurfaceCapabilitiesKHR GetSurfaceCapabilities()
{
	VkResult err;
	VkSurfaceCapabilitiesKHR surface_capabilities;
	err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_gpu, g_surface, &surface_capabilities);
	assert(err == VK_SUCCESS);

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
	VkResult err;
	//选一个present mode
	uint32_t present_mode_count;
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(g_gpu, g_surface, &present_mode_count, NULL);
	assert(err == VK_SUCCESS);
	std::vector<VkPresentModeKHR> present_mode(present_mode_count);
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(g_gpu, g_surface, &present_mode_count, present_mode.data());
	assert(err == VK_SUCCESS);
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
	VkResult err;
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
	err = vkCreateSwapchainKHR(g_device, &swapchain_ci, NULL, &g_swapchain);

	if (oldSwapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(g_device, oldSwapchain, NULL);
	}
}

void CreateSwapchainImageView()
{
	VkResult err;
	//首先把所有swapchain的VkImage获取出来
	uint32_t swapchain_image_count;
	err = vkGetSwapchainImagesKHR(g_device, g_swapchain, &swapchain_image_count, NULL);
	assert(err == VK_SUCCESS);
	std::vector<VkImage> swapchain_images(swapchain_image_count);
	err = vkGetSwapchainImagesKHR(g_device, g_swapchain, &swapchain_image_count, swapchain_images.data());
	assert(err == VK_SUCCESS);

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
		err = vkCreateImageView(g_device, &color_image_view, NULL, &g_swapchain_image_resources[i].view);
		assert(err == VK_SUCCESS);
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
	VkResult err;
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
	err = vkCreateImage(g_device, &ci, NULL, &g_depth.image);
	assert(err == VK_SUCCESS);

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
	err = vkAllocateMemory(g_device, &mem_alloc_info, NULL, &g_depth.mem);
	assert(err == VK_SUCCESS);

	//绑定内存到VkImage
	err = vkBindImageMemory(g_device, g_depth.image, g_depth.mem, 0);
	assert(err == VK_SUCCESS);

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
	err = vkCreateImageView(g_device, &view, NULL, &g_depth.view);
	assert(err == VK_SUCCESS);
}

void PrepareCubeTexture()
{
	VkResult err;
	//我们的texture是R8G8B8A8格式
	const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageTiling tiling = VK_IMAGE_TILING_LINEAR;
	const VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	//获取对应格式的FormatProperties
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(g_gpu, tex_format, &props);
	PrintPhysicalDeviceFormatProperties(tex_format, props);
	//验证这个gpu是可以处理我们这个格式的
	if (!(props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ))
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
	err = vkCreateImage(g_device, &image_create_info, NULL, &g_texture.image);
	assert(err == VK_SUCCESS);

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
	err = vkAllocateMemory(g_device, &mem_alloc_info, NULL, &g_texture.mem);
	assert(err == VK_SUCCESS);

	//绑定内存到VkImage
	err = vkBindImageMemory(g_device, g_texture.image, g_texture.mem, 0);
	assert(err == VK_SUCCESS);

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
	err = vkMapMemory(g_device, g_texture.mem, 0, mem_alloc_info.allocationSize, 0, &data);
	assert(err == VK_SUCCESS);
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
	err = vkCreateSampler(g_device, &sampler_ci, NULL, &g_texture.sampler);
	assert(err == VK_SUCCESS);

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
	err = vkCreateImageView(g_device, &view, NULL, &g_texture.view);
	assert(err == VK_SUCCESS);
}

void PrepareDataBuffer()
{
	VkResult err;
	vec3 eye = { 0.0f, 3.0f, 5.0f };
	vec3 origin = { 0, 0, 0 };
	vec3 up = { 0.0f, 1.0f, 0.0 };
	//准备MVP矩阵
	mat4x4_perspective(g_projection_matrix, (float)degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
	mat4x4_look_at(g_view_matrix, eye, origin, up);
	mat4x4_identity(g_model_matrix);
	g_projection_matrix[1][1] *= -1;  // Flip projection matrix from GL to Vulkan orientation.
	mat4x4 MVP, VP;
	mat4x4_mul(VP, g_projection_matrix, g_view_matrix);
	mat4x4_mul(MVP, VP, g_model_matrix);
	memcpy(g_vs_uniform_struct.mvp, MVP, sizeof(MVP));
	for (unsigned int i = 0; i < 12 * 3; i++) {
		g_vs_uniform_struct.position[i][0] = g_vertex_buffer_data[i * 3];
		g_vs_uniform_struct.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
		g_vs_uniform_struct.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
		g_vs_uniform_struct.position[i][3] = 1.0f;
		g_vs_uniform_struct.attr[i][0] = g_uv_buffer_data[2 * i];
		g_vs_uniform_struct.attr[i][1] = g_uv_buffer_data[2 * i + 1];
		g_vs_uniform_struct.attr[i][2] = 0;
		g_vs_uniform_struct.attr[i][3] = 0;
	}
	
	//准备好了g_vs_uniform_struct，现在开始创建buffer
	VkBufferCreateInfo buf_info;
	memset(&buf_info, 0, sizeof(buf_info));
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = sizeof(g_vs_uniform_struct);

	for (size_t i = 0; i < g_swapchain_image_resources.size(); i++) 
	{
		//创建VkBuffer
		err = vkCreateBuffer(g_device, &buf_info, NULL, &g_swapchain_image_resources[i].uniform_buffer);
		assert(err == VK_SUCCESS);

		//跟之前的VkImage一样，需要获取它的MemoryRequirements
		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(g_device, g_swapchain_image_resources[i].uniform_buffer, &mem_reqs);

		//分配内存
		uint32_t memory_type_index = GetMemoryTypeFromProperties(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VkMemoryAllocateInfo mem_alloc{
			/*.sType = */VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			/*.pNext = */NULL,
			/*.allocationSize = */mem_reqs.size,
			/*.memoryTypeIndex = */memory_type_index
		};
		err = vkAllocateMemory(g_device, &mem_alloc, NULL, &g_swapchain_image_resources[i].uniform_memory);
		assert(err == VK_SUCCESS);

		//开始Map/Unmap设置内存数据,注意这里我们只进行了map，而没有unmap，因为uniform_memory_ptr后面会用
		err = vkMapMemory(g_device, g_swapchain_image_resources[i].uniform_memory, 0, VK_WHOLE_SIZE, 0,
			&g_swapchain_image_resources[i].uniform_memory_ptr);
		assert(err == VK_SUCCESS);

		memcpy(g_swapchain_image_resources[i].uniform_memory_ptr, &g_vs_uniform_struct, sizeof g_vs_uniform_struct);

		//bind memory到buffer
		err = vkBindBufferMemory(g_device, g_swapchain_image_resources[i].uniform_buffer,
			g_swapchain_image_resources[i].uniform_memory, 0);
		assert(err == VK_SUCCESS);
	}
}

void PrepareDescriptorSetLayout()
{
	VkResult err;
	const VkDescriptorSetLayoutBinding layout_bindings[2] = {
		{
			/*.binding = */0,
			/*.descriptorType = */VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			/*.descriptorCount = */1,
			/*.stageFlags = */VK_SHADER_STAGE_VERTEX_BIT,
			/*.pImmutableSamplers = */NULL,
		},
		{
			/*.binding = */1,
			/*.descriptorType = */VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			/*.descriptorCount = */1,
			/*.stageFlags = */VK_SHADER_STAGE_FRAGMENT_BIT,
			/*.pImmutableSamplers = */NULL,
		},
	};
	const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
		/*.sType = */VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags = */0,
		/*.bindingCount = */2,
		/*.pBindings = */layout_bindings
	};
	err = vkCreateDescriptorSetLayout(g_device, &descriptor_layout, NULL, &g_desc_layout);
	assert(err == VK_SUCCESS);

	const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
		/*.sType = */VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags = */0,
		/*.setLayoutCount = */1,
		/*.pSetLayouts = */&g_desc_layout,
		/*.pushConstantRangeCount = */0,
		/*.pPushConstantRanges = */NULL
	};
	err = vkCreatePipelineLayout(g_device, &pipeline_layout_create_info, NULL, &g_pipeline_layout);
	assert(err == VK_SUCCESS);
}

void PrepareRenderPass()
{
	//color和depth的初始layout是LAYOUT_UNDEFINED
	//因为在renderpass一开始我们并不关心它里面的内容
	//在subpass开始的时候，color attachment会被transition到LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	//depth会被transition到LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	//在renderpass结束时，color attachment会被transition到LAYOUT_PRESENT_SRC_KHR，然后拿去present
	//这些都是作为renderpass的一部分来完成的，无需barriers
	const VkAttachmentDescription attachments[2] = {
		{
			/*.flags = */0,
			/*.format = */g_surface_format.format,
			/*.samples = */VK_SAMPLE_COUNT_1_BIT,
			/*.loadOp = */VK_ATTACHMENT_LOAD_OP_CLEAR,
			/*.storeOp = */VK_ATTACHMENT_STORE_OP_STORE,
			/*.stencilLoadOp = */VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			/*.stencilStoreOp = */VK_ATTACHMENT_STORE_OP_DONT_CARE,
			/*.initialLayout = */VK_IMAGE_LAYOUT_UNDEFINED,
			/*.finalLayout = */VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
		{
			/*.flags = */0,
			/*.format = */g_depth.format,
			/*.samples = */VK_SAMPLE_COUNT_1_BIT,
			/*.loadOp = */VK_ATTACHMENT_LOAD_OP_CLEAR,
			/*.storeOp = */VK_ATTACHMENT_STORE_OP_DONT_CARE,
			/*.stencilLoadOp = */VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			/*.stencilStoreOp = */VK_ATTACHMENT_STORE_OP_DONT_CARE,
			/*.initialLayout = */VK_IMAGE_LAYOUT_UNDEFINED,
			/*.finalLayout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
	};
	const VkAttachmentReference color_reference = {
		/*.attachment = */0,
		/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	const VkAttachmentReference depth_reference = {
		/*.attachment = */1,
		/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
	const VkSubpassDescription subpass = {
		/*.flags = */0,
		/*.pipelineBindPoint = */VK_PIPELINE_BIND_POINT_GRAPHICS,
		/*.inputAttachmentCount = */0,
		/*.pInputAttachments = */NULL,
		/*.colorAttachmentCount = */1,
		/*.pColorAttachments = */&color_reference,
		/*.pResolveAttachments = */NULL,
		/*.pDepthStencilAttachment = */&depth_reference,
		/*.preserveAttachmentCount = */0,
		/*.pPreserveAttachments = */NULL,
	};

	VkSubpassDependency attachmentDependencies[2] = {
		{
			// Depth buffer is shared between swapchain images
			/*.srcSubpass = */VK_SUBPASS_EXTERNAL,
			/*.dstSubpass = */0,
			/*.srcStageMask = */VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			/*.dstStageMask = */VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			/*.srcAccessMask = */VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			/*.dstAccessMask = */VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			/*.dependencyFlags = */0,
		},
		{
			// Image Layout Transition
			/*.srcSubpass = */VK_SUBPASS_EXTERNAL,
			/*.dstSubpass = */0,
			/*.srcStageMask = */VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			/*.dstStageMask = */VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			/*.srcAccessMask = */0,
			/*.dstAccessMask = */VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
			/*.dependencyFlags = */0,
		},
	};

	const VkRenderPassCreateInfo rp_info = {
		/*.sType = */VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags = */0,
		/*.attachmentCount = */2,
		/*.pAttachments = */attachments,
		/*.subpassCount = */1,
		/*.pSubpasses = */&subpass,
		/*.dependencyCount = */2,
		/*.pDependencies = */attachmentDependencies,
	};
	VkResult err = vkCreateRenderPass(g_device, &rp_info, NULL, &g_render_pass);
	assert(err == VK_SUCCESS);
}

VkShaderModule PrepareShaderModule(const uint32_t* shader_code, size_t code_size)
{
	VkResult err;
	VkShaderModule module;
	VkShaderModuleCreateInfo moduleCreateInfo;

	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pNext = NULL;
	moduleCreateInfo.flags = 0;
	moduleCreateInfo.codeSize = code_size;
	moduleCreateInfo.pCode = shader_code;

	err = vkCreateShaderModule(g_device, &moduleCreateInfo, NULL, &module);
	assert(err == VK_SUCCESS);

	return module;
}

void PreparePipeline()
{
	VkResult err;
	//首先创建pipeline cache，所有的pipeline都从cache中分配出来
	VkPipelineCacheCreateInfo pipeline_cache_ci;
	memset(&pipeline_cache_ci, 0, sizeof(pipeline_cache_ci));
	pipeline_cache_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	err = vkCreatePipelineCache(g_device, &pipeline_cache_ci, NULL, &g_pipeline_cache);
	assert(err == VK_SUCCESS);

	//下面开始创建pipeline
	constexpr int NUM_DYNAMIC_STATES = 2; /*Viewport + Scissor*/
	VkPipelineCacheCreateInfo pipelineCache;
	VkPipelineVertexInputStateCreateInfo vi;
	VkPipelineInputAssemblyStateCreateInfo ia;
	VkPipelineRasterizationStateCreateInfo rs;
	VkPipelineColorBlendStateCreateInfo cb;
	VkPipelineDepthStencilStateCreateInfo ds;
	VkPipelineViewportStateCreateInfo vp;
	VkPipelineMultisampleStateCreateInfo ms;
	VkDynamicState dynamicStateEnables[NUM_DYNAMIC_STATES];
	VkPipelineDynamicStateCreateInfo dynamicState;

	memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
	memset(&dynamicState, 0, sizeof dynamicState);
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables;

	memset(&vi, 0, sizeof(vi));
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	memset(&ia, 0, sizeof(ia));
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	memset(&rs, 0, sizeof(rs));
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_BACK_BIT;
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;
	rs.lineWidth = 1.0f;

	memset(&cb, 0, sizeof(cb));
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState att_state[1];
	memset(att_state, 0, sizeof(att_state));
	att_state[0].colorWriteMask = 0xf;
	att_state[0].blendEnable = VK_FALSE;
	cb.attachmentCount = 1;
	cb.pAttachments = att_state;

	memset(&vp, 0, sizeof(vp));
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.viewportCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
	vp.scissorCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

	memset(&ds, 0, sizeof(ds));
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.depthTestEnable = VK_TRUE;
	ds.depthWriteEnable = VK_TRUE;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
	ds.stencilTestEnable = VK_FALSE;
	ds.front = ds.back;

	memset(&ms, 0, sizeof(ms));
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pSampleMask = NULL;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	//这里的shader code是提前编好的SPRI-V的shader代码
	VkShaderModule vert_shader_module = PrepareShaderModule(g_vs_code, sizeof(g_vs_code));
	VkShaderModule frag_shader_module = PrepareShaderModule(g_fs_code, sizeof(g_fs_code));
	
	// Two stages: vs and fs
	VkPipelineShaderStageCreateInfo shaderStages[2];
	memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vert_shader_module;
	shaderStages[0].pName = "main";

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = frag_shader_module;
	shaderStages[1].pName = "main";

	VkGraphicsPipelineCreateInfo pipeline;
	memset(&pipeline, 0, sizeof(pipeline));
	pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline.layout = g_pipeline_layout;
	pipeline.pVertexInputState = &vi;
	pipeline.pInputAssemblyState = &ia;
	pipeline.pRasterizationState = &rs;
	pipeline.pColorBlendState = &cb;
	pipeline.pMultisampleState = &ms;
	pipeline.pViewportState = &vp;
	pipeline.pDepthStencilState = &ds;
	pipeline.stageCount = ARRAY_SIZE(shaderStages);
	pipeline.pStages = shaderStages;
	pipeline.renderPass = g_render_pass;
	pipeline.pDynamicState = &dynamicState;
	err = vkCreateGraphicsPipelines(g_device, g_pipeline_cache, 1, &pipeline, NULL, &g_pipeline);
	assert(err == VK_SUCCESS);

	vkDestroyShaderModule(g_device, frag_shader_module, NULL);
	vkDestroyShaderModule(g_device, vert_shader_module, NULL);
}

void PrepareDescriptorPool()
{
	const VkDescriptorPoolSize type_counts[2] = {
		{
		   /*.type = */VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		   /*.descriptorCount = */g_swapchain_image_resources.size(),
		},
		{
		   /*.type = */VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		   /*.descriptorCount = */g_swapchain_image_resources.size(),
		},
	};
	const VkDescriptorPoolCreateInfo descriptor_pool = {
		/*.sType = */VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flag = */0,
		/*.maxSets = */g_swapchain_image_resources.size(),
		/*.poolSizeCount = */2,
		/*.pPoolSizes = */type_counts,
	};
	VkResult err = vkCreateDescriptorPool(g_device, &descriptor_pool, NULL, &g_desc_pool);
	assert(err == VK_SUCCESS);
}

void PrepareDescriptorSet()
{
	VkResult err;
	VkDescriptorSetAllocateInfo alloc_info = { /*.sType = */VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
											  /*.pNext = */NULL,
											  /*.descriptorPool = */g_desc_pool,
											  /*.descriptorSetCount = */1,
											  /*.pSetLayouts = */&g_desc_layout };

	VkDescriptorBufferInfo buffer_info;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(VsUniformStruct);

	VkDescriptorImageInfo tex_descs;
	memset(&tex_descs, 0, sizeof(tex_descs));
	tex_descs.sampler = g_texture.sampler;
	tex_descs.imageView = g_texture.view;
	tex_descs.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet writes[2];
	memset(&writes, 0, sizeof(writes));
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &buffer_info;

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstBinding = 1;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[1].pImageInfo = &tex_descs;

	for (unsigned int i = 0; i < g_swapchain_image_resources.size(); i++) {
		err = vkAllocateDescriptorSets(g_device, &alloc_info, &g_swapchain_image_resources[i].descriptor_set);
		assert(err == VK_SUCCESS);
		buffer_info.buffer = g_swapchain_image_resources[i].uniform_buffer;
		writes[0].dstSet = g_swapchain_image_resources[i].descriptor_set;
		writes[1].dstSet = g_swapchain_image_resources[i].descriptor_set;
		vkUpdateDescriptorSets(g_device, 2, writes, 0, NULL);
	}
}

void PrepareFrameBuffer()
{
	VkResult err;
	uint32_t i;
	for (i = 0; i < g_swapchain_image_resources.size(); i++) {
		VkImageView attachments[2];
		attachments[1] = g_depth.view;
		attachments[0] = g_swapchain_image_resources[i].view;
		const VkFramebufferCreateInfo fb_info = {
			/*.sType = */VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			/*.pNext = */NULL,
			/*.flag = */0,
			/*.renderPass = */g_render_pass,
			/*.attachmentCount = */2,
			/*.pAttachments = */attachments,
			/*.width = */g_window_width,
			/*.height = */g_window_height,
			/*.layers = */1,
		};
		err = vkCreateFramebuffer(g_device, &fb_info, NULL, &g_swapchain_image_resources[i].framebuffer);
		assert(err == VK_SUCCESS);
	}
}

void FlushInitCommandBuffer()
{
	VkResult err;
	err = vkEndCommandBuffer(g_init_cmd);
	assert(err == VK_SUCCESS);

	VkFence fence;
	VkFenceCreateInfo fence_ci = {
		/*.sType = */VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		/*.pNext = */NULL,
		/*.flags = */0
	};
	err = vkCreateFence(g_device, &fence_ci, NULL, &fence);

	const VkCommandBuffer cmd_bufs[] = { g_init_cmd };
	VkSubmitInfo submit_info = { /*.sType = */VK_STRUCTURE_TYPE_SUBMIT_INFO,
								/*.pNext = */NULL,
								/*.waitSemaphoreCount = */0,
								/*.pWaitSemaphores = */NULL,
								/*.pWaitDstStageMask = */NULL,
								/*.commandBufferCount = */1,
								/*.pCommandBuffers = */cmd_bufs,
								/*.signalSemaphoreCount = */0,
								/*.pSignalSemaphores = */NULL };
	err = vkQueueSubmit(g_present_queue, 1, &submit_info, fence);
	assert(err == VK_SUCCESS);

	err = vkWaitForFences(g_device, 1, &fence, VK_TRUE, UINT64_MAX);
	assert(err == VK_SUCCESS);

	vkFreeCommandBuffers(g_device, g_cmd_pool, 1, cmd_bufs);
	vkDestroyFence(g_device, fence, NULL);
	g_init_cmd = VK_NULL_HANDLE;
}

void PrepareRenderResource()
{
	PrepareDepth();
	PrepareCubeTexture();
	PrepareDataBuffer();
	PrepareDescriptorSetLayout();
	PrepareRenderPass();
	PreparePipeline();
	PrepareDescriptorPool();
	PrepareDescriptorSet();
	PrepareFrameBuffer();
	FlushInitCommandBuffer();
}

void PrepareRenderCommand()
{
	VkResult err;
	//每个frame buffer都需要一个cmd
	for (uint32_t i = 0; i < g_swapchain_image_resources.size(); i++)
	{
		//首先allocate cb，每个swapchain image都要一个
		const VkCommandBufferAllocateInfo cmd_alloc = {
			/*.sType = */VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			/*.pNext = */NULL,
			/*.commandPool = */g_cmd_pool,
			/*.level = */VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			/*.commandBufferCount = */1,
		};
		VkCommandBuffer cmd_buf;
		err = vkAllocateCommandBuffers(g_device, &cmd_alloc, &cmd_buf);
		assert(err == VK_SUCCESS);
		g_swapchain_image_resources[i].cmd = cmd_buf;

		//现在开始BeginCommandBuffer，之后在EndCommandBuffer之前的所有cmd都会被录制到这个cb中
		const VkCommandBufferBeginInfo cmd_buf_info = {
			/*.sType = */VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			/*.pNext = */NULL,
			/*.flags = */VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
			/*.pInheritanceInfo = */NULL,
		};
		err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);
		assert(err == VK_SUCCESS);

		//开始renderpass
		VkClearValue clear_values[2];
		memset(&clear_values, 0, sizeof(clear_values));
		clear_values[0].color = { 0.2f, 0.2f, 0.2f, 0.2f };
		clear_values[1].depthStencil = { 1.0f, 0 };

		const VkRenderPassBeginInfo rp_begin = {
			/*.sType = */VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			/*.pNext = */NULL,
			/*.renderPass = */g_render_pass,
			/*.framebuffer = */g_swapchain_image_resources[i].framebuffer,
			/*.renderArea.offset.x = */0,
			/*.renderArea.offset.y = */0,
			/*.renderArea.extent.width = */g_window_width,
			/*.renderArea.extent.height = */g_window_height,
			/*.clearValueCount = */2,
			/*.pClearValues = */clear_values,
		};
		vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

		//绑定pipeline
		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);

		//绑定descriptor set
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline_layout, 0, 1,
			&g_swapchain_image_resources[i].descriptor_set, 0, NULL);

		//设置viewport
		VkViewport viewport;
		memset(&viewport, 0, sizeof(viewport));
		float viewport_dimension;
		if (g_window_width < g_window_height) {
			viewport_dimension = (float)g_window_width;
			viewport.y = (g_window_height - g_window_width) / 2.0f;
		}
		else {
			viewport_dimension = (float)g_window_height;
			viewport.x = (g_window_width - g_window_height) / 2.0f;
		}
		viewport.height = viewport_dimension;
		viewport.width = viewport_dimension;
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

		//设置裁切器
		VkRect2D scissor;
		memset(&scissor, 0, sizeof(scissor));
		scissor.extent.width = g_window_width;
		scissor.extent.height = g_window_height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

		//draw
		vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);

		//结束renderpass，注意这里会把图片的image layout从COLOR_ATTACHMENT_OPTIMAL改为PRESENT_SRC_KHR
		vkCmdEndRenderPass(cmd_buf);

		//结束commandbuffer
		err = vkEndCommandBuffer(cmd_buf);
		assert(err == VK_SUCCESS);
	}

}

void UpdateDataBuffer()
{
	mat4x4 MVP, Model, VP;
	int matrixSize = sizeof(MVP);

	mat4x4_mul(VP, g_projection_matrix, g_view_matrix);

	// Rotate around the Y axis
	mat4x4_dup(Model, g_model_matrix);
	mat4x4_rotate(g_model_matrix, Model, 0.0f, 1.0f, 0.0f, (float)degreesToRadians(g_spin_angle));
	mat4x4_mul(MVP, VP, g_model_matrix);

	memcpy(g_swapchain_image_resources[g_current_buffer].uniform_memory_ptr, (const void *)&MVP[0][0], matrixSize);
}

void Update()
{
	VkResult err;

	// 等待Fence
	vkWaitForFences(g_device, 1, &g_fences[g_frame_index], VK_TRUE, UINT64_MAX);
	vkResetFences(g_device, 1, &g_fences[g_frame_index]);

	do {
		// 获取下一个可用的swapchain image index
		err =
			vkAcquireNextImageKHR(g_device, g_swapchain, UINT64_MAX,
				g_image_acquired_semaphores[g_frame_index], VK_NULL_HANDLE, &g_current_buffer);

		if (err == VK_ERROR_OUT_OF_DATE_KHR) {
			// TODO:
			// demo->swapchain is out of date (e.g. the window was resized) and
			// must be recreated:
			//Resize();
		}
		else if (err == VK_SUBOPTIMAL_KHR) {
			// demo->swapchain is not as optimal as it could be, but the platform's
			// presentation engine will still present the image correctly.
			break;
		}
		else if (err == VK_ERROR_SURFACE_LOST_KHR) {
			// Surface丢失，需要destroy surface后重新创建
			vkDestroySurfaceKHR(g_instance, g_surface, NULL);
			CreateSurface();
			// TODO :Resize(); //需要重新创建，走一遍流程
		}
		else {
			assert(!err);
		}
	} while (err != VK_SUCCESS);

	UpdateDataBuffer();

	// 等待当前image的信号量，确保在vulkan引擎释放ownership之前我们不会往这张图上写东西。
	VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.pWaitDstStageMask = &pipe_stage_flags;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &g_image_acquired_semaphores[g_frame_index];
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &g_swapchain_image_resources[g_current_buffer].cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &g_draw_complete_semaphores[g_frame_index];
	err = vkQueueSubmit(g_present_queue, 1, &submit_info, g_fences[g_frame_index]);
	assert(!err);

	// 准备present
	VkPresentInfoKHR present = {
		/*.sType = */VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		/*.pNext = */NULL,
		/*.waitSemaphoreCount = */1,
		/*.pWaitSemaphores = */&g_draw_complete_semaphores[g_frame_index], // 等待画完
		/*.swapchainCount = */1,
		/*.pSwapchains = */&g_swapchain,
		/*.pImageIndices = */&g_current_buffer,
		/*.pResults* = */NULL
	};
	err = vkQueuePresentKHR(g_present_queue, &present);
	g_frame_index += 1;
	g_frame_index %= FRAME_LAG;

	if (err == VK_ERROR_OUT_OF_DATE_KHR) {
		// TODO:
		// demo->swapchain is out of date (e.g. the window was resized) and
		// must be recreated:
		//Resize();
	}
	else if (err == VK_SUBOPTIMAL_KHR) {
		// demo->swapchain is not as optimal as it could be, but the platform's
		// presentation engine will still present the image correctly.
	}
	else if (err == VK_ERROR_SURFACE_LOST_KHR) {
		// Surface丢失，需要destroy surface后重新创建
		vkDestroySurfaceKHR(g_instance, g_surface, NULL);
		CreateSurface();
		// TODO :Resize(); //需要重新创建，走一遍流程
	}
	else {
		assert(!err);
	}
}

int MainLoop()
{
	MSG msg;    // message
	msg.wParam = 0;
	
	bool done = false;
	while (!done) {
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		if (msg.message == WM_QUIT)  // check for a quit message
		{
			done = true;  // if found, quit app
		}
		else {
			/* Translate and dispatch to event queue*/
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			Update();
		}
	}

	return (int)msg.wParam;
}

void CleanUp()
{
	//略
}

int main()
{
	g_hinstance = GetModuleHandle(NULL);

	PreparePhysicalDevice();
	PrepareSwapchain();
	PrepareRenderResource();
	PrepareRenderCommand();
	int ret = MainLoop();
	CleanUp();
	return ret;
}
