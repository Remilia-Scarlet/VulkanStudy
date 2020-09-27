
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
int window_width = 100;
int window_height = 100;

//Vulkan
VkInstance g_instance = VK_NULL_HANDLE;
VkPhysicalDevice g_gpu = VK_NULL_HANDLE;
std::vector<VkQueueFamilyProperties> g_queue_family_props;

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

void Prepare()
{
	EnumerateGlobalExtAndLayer();
	CreateVkInstance();
	EnumeratePhysicalDevices();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_GETMINMAXINFO:  // set window's minimum size
//		((MINMAXINFO *)lParam)->ptMinTrackSize = demo.minsize;
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_SIZE:
		// Resize the application to the new window size, except when
		// it was minimized. Vulkan doesn't support images or swapchains
		// with width=0 and height=0.
		if (wParam != SIZE_MINIMIZED) {
			window_width = lParam & 0xffff;
			window_height = (lParam & 0xffff0000) >> 16;
			//	demo_resize();
		}
		break;
		//case WM_KEYDOWN:
		//	switch (wParam) {
		//	case VK_LEFT:
		//		demo.spin_angle -= demo.spin_increment;
		//		break;
		//	case VK_RIGHT:
		//		demo.spin_angle += demo.spin_increment;
		//		break;
		//	case VK_SPACE:
		//		demo.pause = !demo.pause;
		//		break;
		//	}
		//	return 0;
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
	win_class.hInstance = GetModuleHandle(NULL);  // hInstance
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
	RECT wr = { 0, 0, window_width, window_height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	HWND hwindow = CreateWindowEx(0,
		"LearningVulkan",            // class name
		"LearningVulkan",            // app name
		WS_OVERLAPPEDWINDOW |  // window style
		WS_VISIBLE | WS_SYSMENU,
		100, 100,            // x/y coords
		wr.right - wr.left,  // width
		wr.bottom - wr.top,  // height
		NULL,                // handle to parent
		NULL,                // handle to menu
		GetModuleHandle(NULL),    // hInstance
		NULL);               // no extra parameters
}

void PrepareSwapchain()
{
	CreateMainWindow();
}

int main()
{
	Prepare();
	PrepareSwapchain();

	return 0;
}