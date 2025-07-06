#pragma once
#include "MemoryConfig.h"
#include "Buffer.h"

VK_BEGIN

class Image : public RecordableResource
{
public:
	Image() = default;

	Core::Ref<vk::ImageView> CreateImageView(const Core::ImageViewInfo& info) const;

	template <typename T>
	void CopyBufferData(const Buffer<T>& buffer);

	void Blit(const Image& src, const ImageBlitInfo blitInfo);

	void TransitionLayout(vk::ImageLayout newLayout, vk::PipelineStageFlags usageStage) const;

	/* -------------------- Scoped operations ---------------------- */

	virtual void BeginCommands(vk::CommandBuffer commandBuffer) const override;

	void RecordBlit(const Image& src, const ImageBlitInfo& blitInfo, bool restoreOriginalLayout = true) const;
	void RecordTransitionLayout(vk::ImageLayout newLayout, vk::PipelineStageFlags usageStage) const;

	virtual void EndCommands() const override;

	/* ------------------------------------------------------------- */

	void TransferOwnership(vk::QueueFlagBits Cap) const;
	void TransferOwnership(uint32_t queueFamilyIndex) const;
	Core::Ref<vk::Sampler> GetSampler() const { return mChunk->Sampler; }

	void SetSampler(Core::Ref<vk::Sampler> sampler) { mChunk->Sampler = sampler; }

	// Only base mipmap level is supported right now!
	std::vector<vk::ImageSubresourceRange> GetSubresourceRanges() const;
	std::vector<vk::ImageSubresourceLayers> GetSubresourceLayers() const;

	vk::ImageView GetIdentityImageView() const { return mChunk->ImageHandles.IdentityView; }

	explicit operator bool() const { return static_cast<bool>(mChunk); }

private:
	Core::Ref<Core::ImageChunk> mChunk;

	explicit Image(const Core::Ref<Core::ImageChunk>& chunk)
		: mChunk(chunk) {}

	friend class ResourcePool;
	friend class RenderTargetContext;
	friend class Framebuffer;

	friend class Swapchain;

	friend void RecordBlitImages(vk::CommandBuffer commandBuffer, Image& Dst,
		vk::ImageLayout dstLayout, const Image& Src,
		vk::ImageLayout srcLayout, const ImageBlitInfo& blitInfo);
private:
	// Helper methods...

	void RecordTransitionLayoutInternal(vk::ImageLayout NewLayout, vk::PipelineStageFlags usageStage, 
		vk::ImageLayout oldLayout, vk::PipelineStageFlags oldStages,
		vk::CommandBuffer CmdBuffer, vk::QueueFlags QueueCaps) const;

	void RecordBlitInternal(const ImageBlitInfo& imageBlitInfo, vk::ImageLayout dstLayout, 
		const Image& src, vk::ImageLayout srcLayout, vk::CommandBuffer CmdBuffer) const;

	void PerformImageCopyTaskGPU(Core::Image& DstImage, const Core::Image& SrcImage,
		const vk::ArrayProxy<vk::ImageCopy>& CopyRegions) const;
	void PerformBufferToImageCopyTaskGPU(Core::Image& DstImage, const Core::Buffer& SrcBuffer,
		const vk::ArrayProxy<vk::BufferImageCopy>& CopyRegions) const;

	void ReleaseImage(uint32_t dstQueueFamily) const;
	void AcquireImage(uint32_t dstQueueFamily) const;

	// Make hollow instance...
	void MakeHollow();
};

template <typename T>
void VK_NAMESPACE::Image::CopyBufferData(const Buffer<T>& buffer)
{
	vk::BufferImageCopy CopyRegion{};
	CopyRegion.setBufferOffset(0);
	CopyRegion.setBufferImageHeight(0);
	CopyRegion.setBufferRowLength(0);

	CopyRegion.setImageOffset({ 0, 0, 0 });
	CopyRegion.setImageExtent(mChunk->ImageHandles.Config.Extent);
	CopyRegion.setImageSubresource(GetSubresourceLayers().front());

	PerformBufferToImageCopyTaskGPU(mChunk->ImageHandles, buffer.GetNativeHandles(), CopyRegion);
}

void RecordBlitImages(vk::CommandBuffer commandBuffer, Image& Dst,
	vk::ImageLayout dstLayout, const Image& Src, 
	vk::ImageLayout srcLayout, const ImageBlitInfo& blitInfo);

VK_END
