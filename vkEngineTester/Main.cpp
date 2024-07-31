#include "GraphicsTester/GraphicsPipelineTester.h"
#include "ComputeTester/ComputePipelineTester.h"

void TestGraphicsPipeline()
{
	WindowProps props{};
	props.width = 1600;
	props.height = 900;
	props.name = "Learning it the hard way";
	props.vSync = false;

	ApplicationCreateInfo appInfo{};
	appInfo.AppName = "Vulkan Engine Tester";
	appInfo.EngineName = "vkEngine";
	appInfo.WindowInfo = props;

	{
		std::unique_ptr<Application> App = std::make_unique<GraphicsPipelineTester>(appInfo);
		App->Run();
	}
}

void TestComputePipeline()
{
	WindowProps props{};
	props.width = 1600;
	props.height = 900;
	props.name = "Learning it the hard way";
	props.vSync = false;

	ApplicationCreateInfo appInfo{};
	appInfo.AppName = "Vulkan Engine Tester";
	appInfo.EngineName = "vkEngine";
	appInfo.WindowInfo = props;

	{
		std::unique_ptr<Application> App = std::make_unique<ComputePipelineTester>(appInfo);
		App->Run();
	}
}

int main()
{
	{
		std::thread thread[1];

		thread[0] = std::thread(TestComputePipeline);

		thread[0].join();
	}

	return 0;
}
