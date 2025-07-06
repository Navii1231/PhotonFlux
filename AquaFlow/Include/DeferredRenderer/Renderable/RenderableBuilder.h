#pragma once
#include "../../Core/AqCore.h"
#include "DeferredRenderable.h"

AQUA_BEGIN

class RenderableBuilder
{
public:
	using CopyVertFn = std::function<void(vkEngine::GenericBuffer&, const RenderableInfo&)>;
	using CopyIdxFn = std::function<void(vkEngine::GenericBuffer&, const RenderableInfo&)>;
	using CopyVertFnMap = std::unordered_map<std::string, CopyVertFn>;

public:

	template <typename IdxFn>
	RenderableBuilder(vkEngine::Context ctx, IdxFn&& idxFn)
		: mCtx(ctx)
	{
		mIndexCopyFunc = idxFn;
		mResourcePool = mCtx.CreateResourcePool();
	}

	~RenderableBuilder() = default;

	CopyVertFn& operator[](const std::string& name) { return mVertexCopyFuncs[name]; }

	std::shared_ptr<Renderable> CreateRenderable(const RenderableInfo& renderableInfo)
	{
		std::shared_ptr<Renderable> renderable = std::make_shared<Renderable>();
		renderable->Info = renderableInfo;

		renderable->mVertexBuffers.reserve(mVertexCopyFuncs.size());

		for (auto& [name, func] : mVertexCopyFuncs)
		{
			renderable->mVertexBuffers[name] = mResourcePool.CreateGenericBuffer(
				renderableInfo.Usage | vk::BufferUsageFlagBits::eVertexBuffer, 
				vk::MemoryPropertyFlagBits::eHostCoherent);
			func(renderable->mVertexBuffers[name], renderableInfo);
		}

		renderable->mIndexBuffer = mResourcePool.CreateGenericBuffer(
			renderableInfo.Usage | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eHostCoherent);

		mIndexCopyFunc(renderable->mIndexBuffer, renderableInfo);
		
		return renderable;
	}

private:
	vkEngine::Context mCtx;
	vkEngine::ResourcePool mResourcePool;

	CopyIdxFn mIndexCopyFunc;
	CopyVertFnMap mVertexCopyFuncs;
};

AQUA_END
