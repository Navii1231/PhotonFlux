#pragma once
#include "MemoryConfig.h"

VK_BEGIN

class ImageView
{
public:
	
private:
	Core::Ref<vk::ImageView> mHandle;

	ImageView() = default;
	friend class Image;
};

VK_END
