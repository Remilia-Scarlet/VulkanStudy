#pragma once


static void PrintQueueFamilyProperties(const std::vector<VkQueueFamilyProperties>& queue_family_props)
{
	auto flag_to_str = [](VkQueueFlags flag)
	{
		std::string ret;
		if(flag & VK_QUEUE_GRAPHICS_BIT)ret += (ret.empty() ? "" : " | ") + std::string("VK_QUEUE_GRAPHICS_BIT");
		if(flag & VK_QUEUE_COMPUTE_BIT) ret += (ret.empty() ? "" : " | ") + std::string("VK_QUEUE_COMPUTE_BIT");
		if(flag & VK_QUEUE_TRANSFER_BIT) ret += (ret.empty() ? "" : " | ") + std::string("VK_QUEUE_TRANSFER_BIT");
		if(flag & VK_QUEUE_SPARSE_BINDING_BIT) ret += (ret.empty() ? "" : " | ") + std::string("VK_QUEUE_SPARSE_BINDING_BIT");
		if(flag & VK_QUEUE_PROTECTED_BIT) ret += (ret.empty() ? "" : " | ") + std::string("VK_QUEUE_PROTECTED_BIT");
		return ret;
	};
	std::cout << "\nQueue family count:" << queue_family_props.size() << std::endl;
	for (size_t i = 0 ; i < queue_family_props.size(); ++i)
	{
		std::cout << "family " << i << " queue count:\t" << queue_family_props[i].queueCount << std::endl;
		std::cout << "family " << i << " queue flags:\t" << flag_to_str(queue_family_props[i].queueFlags).c_str() << std::endl;
	}
}