#include "Application.h"

Application::Application(const ApplicationCreateInfo& info) : mWindow(std::make_unique<WindowsWindow>(info.WindowInfo))
{
	_STL_ASSERT(!sApplicationInstance, "Application has already been created!");
	sApplicationInstance = this;

	std::vector<const char*> extensions =
#if _DEBUG
	{ "VK_EXT_debug_utils" };
	//{ "VK_EXT_debug_report", "VK_EXT_debug_utils" };
#else
	{};
#endif
	std::vector<const char*> layers =
#if _DEBUG
	{ "VK_LAYER_KHRONOS_validation" /*"VK_LAYER_NV_GPU_Trace_release_public_2024_1_1"*/
	/*"VK_LAYER_NV_nomad_release_public_2024_1_1"*/ };
#else
	{};
#endif;

	mInstanceMenagerie = std::make_shared<vkEngine::InstanceMenagerie>(extensions, layers);

	vkEngine::InstanceCreateInfo instanceInfo{};
	instanceInfo.AppName = info.AppName;
	instanceInfo.EngineName = info.EngineName;
	instanceInfo.AppVersion = { 1, 0, 0 };
	instanceInfo.EngineVersion = { 1, 0, 0 };

	mInstance = mInstanceMenagerie->Create(instanceInfo);
	auto [Result, Surface] = mInstanceMenagerie->CreateSurface(mInstance, mWindow->GetNativeHandle());

	_STL_ASSERT(Result == vk::Result::eSuccess, "Could not create a surface!");

	mSurface = Surface;

#if _DEBUG
	vkEngine::DebugMessengerCreateInfo messengerInfo{};

	messengerInfo.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

	messengerInfo.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

	mMessenger =
		vkEngine::InstanceMenagerie::CreateDebugMessenger(mInstance, messengerInfo, DebugCallback);
#endif

	mPhysicalDevices = std::make_shared<vkEngine::PhysicalDeviceMenagerie>(mInstance);

	vkEngine::DeviceCreateInfo deviceInfo{};

	deviceInfo.DeviceCapabilities =
		vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute |
		vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eSparseBinding;

#if _DEBUG
	deviceInfo.Layers =
	{ "VK_LAYER_KHRONOS_validation" };
	deviceInfo.Extensions =
	{ VK_EXT_TOOLING_INFO_EXTENSION_NAME };
#else
	deviceInfo.Layers =
	{};
#endif;

	deviceInfo.MaxQueueCount = info.WorkerCount;
	deviceInfo.PhysicalDevice = (*mPhysicalDevices)[0];
	deviceInfo.RequiredFeatures = deviceInfo.PhysicalDevice.Features;

	deviceInfo.SwapchainInfo.Width = mWindow->GetWindowSize().x;
	deviceInfo.SwapchainInfo.Height = mWindow->GetWindowSize().y;
	deviceInfo.SwapchainInfo.PresentMode = vk::PresentModeKHR::eMailbox;
	deviceInfo.SwapchainInfo.Surface = mSurface;

	try
	{
		mDevice = std::make_shared<vkEngine::Device>(deviceInfo);
	}
	catch (vk::SystemError& e)
	{
		std::cout << e.what() << std::endl;
		std::cin.get();
		__debugbreak();
	}

	mSwapchain = mDevice->GetSwapchain();
}

void Application::Run()
{
	std::chrono::time_point<std::chrono::high_resolution_clock> Previous;

	std::chrono::high_resolution_clock clock;

	Previous = clock.now();

	auto Duration = clock.now() - Previous;

	OnStart();

	while (mRunning.load() && !mWindow->IsWindowClosed())
	{
		OnUpdate(static_cast<float>(Duration.count()) / static_cast<float>(1e9));
		mWindow->PollUserEvents();

		Duration = clock.now() - Previous;
		Previous = clock.now();
	}
}
