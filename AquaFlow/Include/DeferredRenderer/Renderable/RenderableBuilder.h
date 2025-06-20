#pragma once
#include <type_traits>
#include <functional>

#include "../../Core/AqCore.h"
#include "DeferredRenderable.h"

AQUA_BEGIN

class RenderableBuilder
{
public:
	using CopyFn = std::function<void(vkEngine::GenericBuffer&, const RenderableInfo&)>;
	using CopyFnList = std::vector<CopyFn>;

public:

	template <typename IdxFn, typename ...VertFn>
	RenderableBuilder(vkEngine::Device ctx, IdxFn&& idxFn, VertFn&&... vertfn)
		: mCtx(ctx) 
	{
		mVertexCopyFuncs = CopyFnList({ vertfn, ... });
		mResourceManager = mCtx.MakeMemoryResourceManager();
	}

	~RenderableBuilder() = default;

	template <size_t Idx, typename Func>
	void SetCopyFunctions(Func&& func)
	{
		std::get<Idx>(mVertexCopyFuncs) = func;
	}

	std::shared_ptr<Renderable> CreateRenderable(const RenderableInfo& renderableInfo)
	{
		std::shared_ptr<Renderable> renderable = std::make_shared<Renderable>();
		renderable->Info = renderableInfo;

		vkEngine::BufferCreateInfo createInfo{};
		createInfo.Size = 1;
		createInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
		createInfo.Usage = renderableInfo.Usage | vk::BufferUsageFlagBits::eVertexBuffer;

		renderable->mVertexBuffers.resize(mVertexCopyFuncs.size());

		for (size_t i = 0; i < mVertexCopyFuncs.size(); i++)
		{
			renderable->mVertexBuffers[i] = mResourceManager.CreateGenericBuffer(createInfo);
			mVertexCopyFuncs[i](renderable->mVertexBuffers[i], renderableInfo);
		}

		createInfo.Usage = renderableInfo.Usage | vk::BufferUsageFlagBits::eIndexBuffer;

		renderable->mIndexBuffer = mResourceManager.CreateGenericBuffer(createInfo);
		mIndexCopyFunc(renderable->mIndexBuffer, renderableInfo);
		
		return renderable;
	}

private:
	vkEngine::Device mCtx;
	vkEngine::MemoryResourceManager mResourceManager;

	CopyFn mIndexCopyFunc;
	CopyFnList mVertexCopyFuncs;
};

AQUA_END
