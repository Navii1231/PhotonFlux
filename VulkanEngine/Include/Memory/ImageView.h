#pragma once
#include "MemoryConfig.h"

VK_BEGIN

// TODO: Class is incomplete
class ImageView
{
public:
	
private:
	Core::Ref<vk::ImageView> mHandle;

	ImageView() = default;
	friend class Image;
};

VK_END
