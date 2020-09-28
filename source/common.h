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

static const char* VkFormat2Str(VkFormat format)
{
#define VKFORMAT_2_STR(FORMAT) case FORMAT: return #FORMAT
	switch (format)
	{
		VKFORMAT_2_STR(VK_FORMAT_UNDEFINED);
		VKFORMAT_2_STR(VK_FORMAT_R4G4_UNORM_PACK8);
		VKFORMAT_2_STR(VK_FORMAT_R4G4B4A4_UNORM_PACK16);
		VKFORMAT_2_STR(VK_FORMAT_B4G4R4A4_UNORM_PACK16);
		VKFORMAT_2_STR(VK_FORMAT_R5G6B5_UNORM_PACK16);
		VKFORMAT_2_STR(VK_FORMAT_B5G6R5_UNORM_PACK16);
		VKFORMAT_2_STR(VK_FORMAT_R5G5B5A1_UNORM_PACK16);
		VKFORMAT_2_STR(VK_FORMAT_B5G5R5A1_UNORM_PACK16);
		VKFORMAT_2_STR(VK_FORMAT_A1R5G5B5_UNORM_PACK16);
		VKFORMAT_2_STR(VK_FORMAT_R8_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_R8_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_R8_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_R8_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_R8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R8_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R8_SRGB);
		VKFORMAT_2_STR(VK_FORMAT_R8G8_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_R8G8_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_R8G8_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_R8G8_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_R8G8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R8G8_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R8G8_SRGB);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8_SRGB);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8_SINT);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8_SRGB);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8A8_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8A8_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8A8_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8A8_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8A8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8A8_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R8G8B8A8_SRGB);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8A8_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8A8_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8A8_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8A8_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8A8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8A8_SINT);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8A8_SRGB);
		VKFORMAT_2_STR(VK_FORMAT_A8B8G8R8_UNORM_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A8B8G8R8_SNORM_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A8B8G8R8_USCALED_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A8B8G8R8_SSCALED_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A8B8G8R8_UINT_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A8B8G8R8_SINT_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A8B8G8R8_SRGB_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2R10G10B10_UNORM_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2R10G10B10_SNORM_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2R10G10B10_USCALED_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2R10G10B10_SSCALED_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2R10G10B10_UINT_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2R10G10B10_SINT_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2B10G10R10_SNORM_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2B10G10R10_USCALED_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2B10G10R10_SSCALED_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2B10G10R10_UINT_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_A2B10G10R10_SINT_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_R16_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_R16_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_R16_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_R16_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_R16_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R16_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R16_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R16G16_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_R16G16_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_R16G16_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_R16G16_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_R16G16_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R16G16_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R16G16_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16A16_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16A16_SNORM);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16A16_USCALED);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16A16_SSCALED);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16A16_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16A16_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R16G16B16A16_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R32_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R32_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R32_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R32G32_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R32G32_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R32G32_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R32G32B32_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R32G32B32_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R32G32B32_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R32G32B32A32_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R32G32B32A32_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R32G32B32A32_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R64_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R64_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R64_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R64G64_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R64G64_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R64G64_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R64G64B64_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R64G64B64_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R64G64B64_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_R64G64B64A64_UINT);
		VKFORMAT_2_STR(VK_FORMAT_R64G64B64A64_SINT);
		VKFORMAT_2_STR(VK_FORMAT_R64G64B64A64_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_D16_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_X8_D24_UNORM_PACK32);
		VKFORMAT_2_STR(VK_FORMAT_D32_SFLOAT);
		VKFORMAT_2_STR(VK_FORMAT_S8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_D16_UNORM_S8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_D24_UNORM_S8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_D32_SFLOAT_S8_UINT);
		VKFORMAT_2_STR(VK_FORMAT_BC1_RGB_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC1_RGB_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC2_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC2_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC3_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC3_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC4_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC4_SNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC5_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC5_SNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC6H_UFLOAT_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC6H_SFLOAT_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC7_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_BC7_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_EAC_R11_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_EAC_R11_SNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
		VKFORMAT_2_STR(VK_FORMAT_G8B8G8R8_422_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_B8G8R8G8_422_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_R10X6_UNORM_PACK16);
		VKFORMAT_2_STR(VK_FORMAT_R10X6G10X6_UNORM_2PACK16);
		VKFORMAT_2_STR(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16);
		VKFORMAT_2_STR(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_R12X4_UNORM_PACK16);
		VKFORMAT_2_STR(VK_FORMAT_R12X4G12X4_UNORM_2PACK16);
		VKFORMAT_2_STR(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16);
		VKFORMAT_2_STR(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16);
		VKFORMAT_2_STR(VK_FORMAT_G16B16G16R16_422_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_B16G16R16G16_422_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM);
		VKFORMAT_2_STR(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG);
		VKFORMAT_2_STR(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG);
		VKFORMAT_2_STR(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG);
		VKFORMAT_2_STR(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG);
		VKFORMAT_2_STR(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG);
		VKFORMAT_2_STR(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG);
		VKFORMAT_2_STR(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG);
		VKFORMAT_2_STR(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT);
		VKFORMAT_2_STR(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT);
		default: return "wtf";
	}
#undef VKFORMAT_2_STR
}

static const char* VkColorSpace2Str(VkColorSpaceKHR color_space)
{
#define VKCOLORSPACE_2_STR(SPACE) case SPACE: return #SPACE
	switch (color_space)
	{
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_BT709_LINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_BT709_NONLINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_BT2020_LINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_HDR10_ST2084_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_DOLBYVISION_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_HDR10_HLG_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_PASS_THROUGH_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT);
		VKCOLORSPACE_2_STR(VK_COLOR_SPACE_DISPLAY_NATIVE_AMD);
		default:return "wtf";
	}
#undef VKCOLORSPACE_2_STR
}

static std::string VkMemoryHeapFlags2Str(VkMemoryHeapFlags flags)
{
#define MemoryHeapFlags2Str(FLAG) if(flags & FLAG) str += std::string(str.empty()?"":" | ") + #FLAG
	std::string str;
	MemoryHeapFlags2Str(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
	MemoryHeapFlags2Str(VK_MEMORY_HEAP_MULTI_INSTANCE_BIT);
#undef MemoryHeapFlags2Str
	return str.empty() ? std::string("0") : str;
}

static std::string VkMemoryPropertyFlags2Str(VkMemoryPropertyFlags flags)
{
	std::string str;
#define MemoryPropertyFlags2Str(FLAG) if(flags & FLAG) str += std::string(str.empty() ? "" : " | ") + #FLAG;
	MemoryPropertyFlags2Str(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	MemoryPropertyFlags2Str(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	MemoryPropertyFlags2Str(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
	MemoryPropertyFlags2Str(VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
	MemoryPropertyFlags2Str(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
	MemoryPropertyFlags2Str(VK_MEMORY_PROPERTY_PROTECTED_BIT)
	MemoryPropertyFlags2Str(VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
	MemoryPropertyFlags2Str(VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
#undef MemoryPropertyFlags2Str
	return str.empty() ? std::string("0") : str;
}

static const char* VkPresentModeKHR2Str(VkPresentModeKHR present_mode)
{
#define VKENUM_2_STR(ENUM) case ENUM: return #ENUM
	switch (present_mode)
	{
		VKENUM_2_STR(VK_PRESENT_MODE_IMMEDIATE_KHR);
		VKENUM_2_STR(VK_PRESENT_MODE_MAILBOX_KHR);
		VKENUM_2_STR(VK_PRESENT_MODE_FIFO_KHR);
		VKENUM_2_STR(VK_PRESENT_MODE_FIFO_RELAXED_KHR);
		VKENUM_2_STR(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR);
		VKENUM_2_STR(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR);
		default: return "wtf";
	}
#undef VKENUM_2_STR
}