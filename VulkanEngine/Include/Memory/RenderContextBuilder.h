#pragma once
#include "MemoryConfig.h"
#include "RenderTargetContext.h"

VK_BEGIN

// Change occured: DepthStencil attachment comes at the end of the RenderContextCreateInfo::Attachments

class RenderContextBuilder
{
public:
	RenderContextBuilder() = default;

	RenderTargetContext MakeContext(const RenderContextCreateInfo& createInfo) const
	{
		std::vector<Core::ImageAttachmentConfig> configs;
		AttachmentTypeFlags attachmentFlags = AttachmentTypeFlags();

		for (const auto& attachmentInfo : createInfo.Attachments)
		{
			attachmentFlags |= AttachmentTypeFlagBits::eColor;
			auto& Recent = configs.emplace_back();
			Recent.Layout = attachmentInfo.Layout;
			Recent.Usage = attachmentInfo.Usage;
			Recent.AttachmentDesc.setFormat(attachmentInfo.Format);
			Recent.AttachmentDesc.setLoadOp(attachmentInfo.LoadOp);
			Recent.AttachmentDesc.setSamples(attachmentInfo.Samples);
			Recent.AttachmentDesc.setStoreOp(attachmentInfo.StoreOp);
			Recent.AttachmentDesc.setStencilLoadOp(attachmentInfo.StencilLoadOp);
			Recent.AttachmentDesc.setStencilStoreOp(attachmentInfo.StencilStoreOp);
			Recent.AttachmentDesc.setInitialLayout(attachmentInfo.Layout);
			Recent.AttachmentDesc.setFinalLayout(attachmentInfo.Layout);
		}

		if (createInfo.UsingDepthAttachment || createInfo.UsingStencilAttachment)
		{
			attachmentFlags |= (AttachmentTypeFlags)(
				AttachmentTypeFlagBits::eDepth * createInfo.UsingDepthAttachment |
				AttachmentTypeFlagBits::eStencil * createInfo.UsingStencilAttachment);
		}

		auto Handle = Core::Utils::CreateRenderPass(*mDevice, configs, 
			createInfo.UsingDepthAttachment || createInfo.UsingStencilAttachment, mBindPoint);

		auto Device = mDevice;

		RenderTargetContext context;

		RenderContextData Data{};
		Data.RenderPass = Handle;
		Data.Info = createInfo;

		context.mData = Core::CreateRef(Data, [Device](const RenderContextData renderPass)
		{ Device->destroyRenderPass(renderPass.RenderPass); });

		context.mDevice = Device;
		context.mPhysicalDevice = mPhysicalDevice;
		context.mQueueManager = GetQueueManager();
		context.mCommandPools = mCommandPools;
		context.mAttachmentFlags = attachmentFlags;

		return context;
	}

	QueueManagerRef GetQueueManager() const { return mQueueManager; }

	explicit operator bool() const { return static_cast<bool>(mDevice); }

private:
	Core::Ref<vk::Device> mDevice;
	vk::PhysicalDevice mPhysicalDevice{};

	QueueManagerRef mQueueManager;
	CommandPools mCommandPools;

	vk::PipelineBindPoint mBindPoint = vk::PipelineBindPoint::eGraphics;

	friend class Context;
};

VK_END
