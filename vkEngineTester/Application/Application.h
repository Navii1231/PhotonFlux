#pragma once
#include "Device/Device.h"
#include "Window/GLFW_Window.h"
#include "ShaderCompiler/ShaderCompiler.h"

#include <chrono>

struct ApplicationCreateInfo
{
	WindowProps WindowInfo;
	std::string AppName;
	std::string EngineName;

	uint32_t WorkerCount = 8;
};

inline VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT c_severity,
	VkDebugUtilsMessageTypeFlagsEXT c_type, const std::string& message)
{
	std::cout << "Vulkan API Core ";

	vk::DebugUtilsMessageSeverityFlagBitsEXT severity = (vk::DebugUtilsMessageSeverityFlagBitsEXT) c_severity;
	vk::DebugUtilsMessageTypeFlagBitsEXT type = (vk::DebugUtilsMessageTypeFlagBitsEXT) c_type;

	switch (severity)
	{
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
			std::cout << "[Verbose] ";
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
			std::cout << "[Info] ";
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
			std::cout << "[Warning] ";
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
			std::cout << "[Error] ";
			std::cout << message << '\n';

			char buffer[2048];
			sprintf_s(buffer, "%s", message.c_str());

			_STL_ASSERT(false, buffer);

			return VK_FALSE;
		default:
			break;
	}

	std::cout << message << std::endl;

	return VK_TRUE;
}

class Application
{
public:
	Application(const ApplicationCreateInfo& info);

	virtual bool OnStart() = 0;
	virtual bool OnUpdate(float ElaspedTime) = 0;

	void Run();

	virtual ~Application() = default;

protected:
	std::unique_ptr<WindowsWindow> mWindow;
	std::shared_ptr<vkEngine::Device> mDevice;
	std::shared_ptr<vkEngine::Swapchain> mSwapchain;

	vkEngine::Core::Ref<vk::Instance> mInstance;
	vkEngine::Core::Ref<vk::SurfaceKHR> mSurface;
	vkEngine::Core::Ref<vk::DebugUtilsMessengerEXT> mMessenger;

	std::shared_ptr<vkEngine::InstanceMenagerie> mInstanceMenagerie;
	std::shared_ptr<vkEngine::PhysicalDeviceMenagerie> mPhysicalDevices;

	std::atomic_bool mRunning = true;

	static Application* sApplicationInstance;

private:
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
};

__declspec(selectany) Application* Application::sApplicationInstance = nullptr;
