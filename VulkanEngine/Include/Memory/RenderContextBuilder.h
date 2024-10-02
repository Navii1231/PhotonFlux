#pragma once
#include "MemoryConfig.h"
#include "RenderTargetContext.h"

VK_BEGIN

class RenderContextBuilder
{
public:
	RenderContextBuilder() = default;

	RenderTargetContext MakeContext(const RenderContextCreateInfo& createInfo) const
	{
		std::vector<Core::ImageAttachmentConfig> configs;
		AttachmentTypeFlags attachmentFlags = AttachmentTypeFlags();

		for (const auto& attachmentInfo : createInfo.ColorAttachments)
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

			auto& Recent = configs.emplace_back();
			Recent.Layout = createInfo.DepthStencilAttachment.Layout;
			Recent.Usage = createInfo.DepthStencilAttachment.Usage;
			Recent.AttachmentDesc.setFormat(createInfo.DepthStencilAttachment.Format);
			Recent.AttachmentDesc.setSamples(createInfo.DepthStencilAttachment.Samples);
			Recent.AttachmentDesc.setLoadOp(createInfo.DepthStencilAttachment.LoadOp);
			Recent.AttachmentDesc.setStoreOp(createInfo.DepthStencilAttachment.StoreOp);
			Recent.AttachmentDesc.setStencilLoadOp(createInfo.DepthStencilAttachment.StencilLoadOp);
			Recent.AttachmentDesc.setStencilStoreOp(createInfo.DepthStencilAttachment.StencilStoreOp);
			Recent.AttachmentDesc.setInitialLayout(createInfo.DepthStencilAttachment.Layout);
			Recent.AttachmentDesc.setFinalLayout(createInfo.DepthStencilAttachment.Layout);
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
		context.mProcessHandler = mProcessHandler;
		context.mAttachmentFlags = attachmentFlags;

		return context;
	}

	explicit operator bool() const { return static_cast<bool>(mDevice); }

private:
	Core::Ref<vk::Device> mDevice;
	ProcessManager mProcessHandler;
	vk::PhysicalDevice mPhysicalDevice{};

	vk::PipelineBindPoint mBindPoint = vk::PipelineBindPoint::eGraphics;

	friend class Device;
};

VK_END
