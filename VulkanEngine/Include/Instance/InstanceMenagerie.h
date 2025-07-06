#pragma once
#include "../Core/Config.h"
#include "UnsupportedLayerAndExtensionException.h"
#include "../Core/Ref.h"
#include "GLFW/glfw3.h"

VK_BEGIN

#define DELETE_TOKEN    "$(vkEngine.DeleteMessenger)"

struct Version
{
	uint32_t Major = 1;
	uint32_t Minor = 0;
	uint32_t Patch = 0;
};

struct InstanceCreateInfo
{
	std::string EngineName;
	Version EngineVersion;

	std::string AppName;
	Version AppVersion;
};

struct DebugMessengerCreateInfo
{
	vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity = {};
	vk::DebugUtilsMessageTypeFlagsEXT     messageType = {};
};

class InstanceMenagerie
{
public:
	InstanceMenagerie(const std::vector<const char*>& extensions, const std::vector<const char*>& layers)
		: mExtensions(extensions), mLayers(layers), mAPI_Version(vk::enumerateInstanceVersion())
	{
		auto SupportedExtensions = vk::enumerateInstanceExtensionProperties();
		auto SupportedLayers = vk::enumerateInstanceLayerProperties();

		TryAddingWindowSurfaceExtensions(SupportedExtensions);

		auto UnsupportedExtensions = GetCompliment(SupportedExtensions, mExtensions, 
			[](const vk::ExtensionProperties& extension)
		{
			return extension.extensionName;
		});

		auto UnsupportedLayers = GetCompliment(SupportedLayers, mLayers,
			[](const vk::LayerProperties& extension)
		{
			return extension.layerName;
		});

		if (!UnsupportedLayers.empty() || !UnsupportedExtensions.empty())
			throw UnsupportedLayersAndExtensions(UnsupportedExtensions, UnsupportedLayers);
	}

	Core::Ref<vk::Instance> Create(const InstanceCreateInfo& info) const
	{
		vk::ApplicationInfo appInfo{};

		appInfo.apiVersion = mAPI_Version;
		appInfo.applicationVersion = VK_MAKE_VERSION(
			info.AppVersion.Major, info.AppVersion.Minor, info.AppVersion.Patch);
		appInfo.engineVersion = VK_MAKE_VERSION(
			info.EngineVersion.Major, info.EngineVersion.Minor, info.EngineVersion.Patch);
		appInfo.pApplicationName = info.AppName.c_str();
		appInfo.pEngineName = info.EngineName.c_str();

		vk::InstanceCreateInfo createInfo{};

		createInfo.pApplicationInfo = &appInfo;
		createInfo.ppEnabledExtensionNames = mExtensions.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(mExtensions.size());
		createInfo.ppEnabledLayerNames = mLayers.data();
		createInfo.enabledLayerCount = static_cast<uint32_t>(mLayers.size());

		return { vk::createInstance(createInfo), [](vk::Instance handle)
		{
			handle.destroy();
		} };
	}

	static std::vector<std::string> GetAllLayerNames()
	{
		auto SupportedLayers = vk::enumerateInstanceLayerProperties();

		std::vector<std::string> LayerNames;

		for (auto& prop : SupportedLayers)
			LayerNames.emplace_back(prop.layerName.data());

		return LayerNames;
	}

	static std::vector<std::string> GetAllExtensionNames()
	{
		auto SupportedExtensions = vk::enumerateInstanceExtensionProperties();

		std::vector<std::string> ExtensionNames;

		for (auto& prop : SupportedExtensions)
			ExtensionNames.emplace_back(prop.extensionName.data());

		return ExtensionNames;
	}

	template <typename Callback> static Core::Ref<vk::DebugUtilsMessengerEXT> CreateDebugMessenger(
		Core::Ref<vk::Instance> instance,
		const DebugMessengerCreateInfo& createInfo,
		Callback callback);

	vk::ResultValue<Core::Ref<vk::SurfaceKHR>> CreateSurface(Core::Ref<vk::Instance> instance, GLFWwindow* context)
	{
		if (!mSurfaceSupported)
			return { vk::Result::eErrorExtensionNotPresent, {vk::SurfaceKHR(), [](vk::SurfaceKHR) {} } };

		VkSurfaceKHR c_surface;

		vk::Result result = vk::Result(glfwCreateWindowSurface(*instance, context, nullptr, &c_surface));

		return { result, {c_surface, [instance](vk::SurfaceKHR surface)
		{ instance->destroySurfaceKHR(surface); }}};
	}

	~InstanceMenagerie()
	{}

	InstanceMenagerie(const InstanceMenagerie&) = delete;
	InstanceMenagerie& operator=(const InstanceMenagerie&) = delete;

	InstanceMenagerie(InstanceMenagerie&& Other) noexcept
		: mExtensions(Other.mExtensions), 
		mLayers(Other.mLayers),
		mAPI_Version(Other.mAPI_Version)
	{}

	const std::vector<const char*>& GetExtensions() const { return mExtensions; }
	const std::vector<const char*>& GetLayers() const { return mLayers; }

private:
	std::vector<const char*> mExtensions;
	std::vector<const char*> mLayers;

	bool mSurfaceSupported = false;

	const uint32_t mAPI_Version = -1;

	void TryAddingWindowSurfaceExtensions(const std::vector<vk::ExtensionProperties>& AllExtensions)
	{
		if (!glfwVulkanSupported())
			return;

		uint32_t ExtCount;
		auto SurfaceExts = glfwGetRequiredInstanceExtensions(&ExtCount);

		auto NoSupport = GetCompliment(
			AllExtensions, 
			std::vector<const char*>(SurfaceExts, SurfaceExts + ExtCount),
			[](const vk::ExtensionProperties& props) { return props.extensionName; 
		});

		mSurfaceSupported = NoSupport.empty();

		if (!mSurfaceSupported)
			return;

		mExtensions.insert(mExtensions.end(), SurfaceExts, SurfaceExts + ExtCount);
	}
};

template <typename Callback>
Core::Ref<vk::DebugUtilsMessengerEXT> VK_NAMESPACE::InstanceMenagerie::CreateDebugMessenger(
	Core::Ref<vk::Instance> instance, const DebugMessengerCreateInfo& createInfo, Callback callback)
{
	vk::detail::DispatchLoaderDynamic dispatcher(*instance, vkGetInstanceProcAddr);

	vk::DebugUtilsMessengerCreateInfoEXT rawCreateInfo{};

	rawCreateInfo.messageSeverity = createInfo.messageSeverity;
	rawCreateInfo.messageType = createInfo.messageType;
	rawCreateInfo.pfnUserCallback = [](vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
		const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)->vk::Bool32
	{
		Callback& UserCallback = *(Callback*) pUserData;
		std::string DeleteToken = DELETE_TOKEN;

		if (DeleteToken == pCallbackData->pMessage)
		{
			Callback* pUserCallback = &UserCallback;
			delete pUserCallback;
			return VK_TRUE;
		}

		return UserCallback(messageSeverity, messageTypes, pCallbackData->pMessage);
	};

	rawCreateInfo.pUserData = (void*) new Callback(callback);

	return {instance->createDebugUtilsMessengerEXT(rawCreateInfo, nullptr, dispatcher),
	[instance](vk::DebugUtilsMessengerEXT handle)
	{
		vk::detail::DispatchLoaderDynamic dispatcher(*instance, vkGetInstanceProcAddr);

		// Submitting the a callback to the messenger to delete the allocated user memory
		vk::DebugUtilsMessengerCallbackDataEXT callbackData;
		callbackData.pMessage = DELETE_TOKEN;

		instance->submitDebugUtilsMessageEXT(
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
			callbackData, dispatcher);

		instance->destroyDebugUtilsMessengerEXT(handle, nullptr, dispatcher);
	} };
}

VK_END
