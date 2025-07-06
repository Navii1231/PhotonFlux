#include "Core/vkpch.h" // This cpp file should exist in the src folder
#include "GLFW_Window.h"

static uint32_t sWindowCount = 0;
static std::mutex sWindowLock;

class WindowCallbacks
{
public:
	static void framebuffer_size_callback(GLFWwindow*, int, int);
	static void key_callback(GLFWwindow*, int, int, int, int);
	static void char_callback(GLFWwindow* window, unsigned int keyCode);
	static void mouse_button_callback(GLFWwindow*, int, int, int);
	static void window_close_callback(GLFWwindow* window);
	static void scroll_callback(GLFWwindow* window, double x, double y);
	static void cursor_pos_callback(GLFWwindow* window, double x, double y);
	static void window_pos_callback(GLFWwindow* window, int x, int y);
	static void window_size_callback(GLFWwindow* window, int x, int y);
	static void error_callback(int errCode, const char* errString);
};

WindowsWindow::WindowsWindow(const WindowProps& props)
	: mWindow(), mData({ props, props.name, true, false }) 
{
	std::scoped_lock locker(sWindowLock);

	if (!glfwInit() && sWindowCount == 0)
	{
		mData.state = false;
		return;
	}

	//glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	mWindow = glfwCreateWindow(props.width, props.height, props.name.c_str(), nullptr, nullptr);

	if (!mWindow)
	{
		mData.state = false;
		glfwTerminate();

		_STL_ASSERT(mData.state, "Failed to create a Window!");

		return;
	}

	sWindowCount++;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	glfwMakeContextCurrent(mWindow);

	mData.APIversion = "No API";//(const char*)glGetString(GL_VERSION);
	SetVSync(props.vSync);

	SetWindowCallBacks();

	SetWindowDefaultStyle();
}

WindowsWindow::~WindowsWindow()
{
	std::scoped_lock locker(sWindowLock);
	sWindowCount--;

	ShutDown();

	if(sWindowCount == 0)
		glfwTerminate();

	mWindow = nullptr;
}

void WindowsWindow::SetVSync(bool val)
{
	mData.props.vSync = val;
	glfwSwapInterval(val);
}

void WindowsWindow::ShutDown()
{
	glfwDestroyWindow((GLFWwindow*)mWindow);
}

bool WindowsWindow::IsWindowClosed() const
{
	return glfwWindowShouldClose((GLFWwindow*)mWindow);
}

void WindowsWindow::OnUpdate() const
{
	//glfwSwapBuffers(mWindow);
	glfwPollEvents();
}

void WindowsWindow::SetTitle(const std::string& title)
{
	mData.props.name = title;
	glfwSetWindowTitle(mWindow, mData.props.name.c_str());
}

void WindowsWindow::PollUserEvents() const
{
	glfwPollEvents();
}

void WindowsWindow::HideCursor()
{
	mData.cursorHidden = true;
	glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void WindowsWindow::RetrieveCursor()
{
	mData.cursorHidden = false;
	glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

bool WindowsWindow::IsCursorHidden() const
{
	return mData.cursorHidden;
}

void WindowsWindow::SwapFramebuffers()
{
	glfwSwapBuffers(mWindow);
	glfwPollEvents();
}

glm::vec2 WindowsWindow::GetCursorPosition() const
{
	double x, y;
	glfwGetCursorPos(mWindow, &x, &y);
	return { (float) x, (float) y };
}

void WindowsWindow::HideCursor() const
{
	glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void WindowsWindow::RetrieveCursor() const
{
	glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void WindowsWindow::SetWindowDefaultStyle()
{
}

void WindowsWindow::SetWindowCallBacks()
{
	glfwSetWindowUserPointer(mWindow, &mData);

	glfwSetFramebufferSizeCallback(mWindow, WindowCallbacks::framebuffer_size_callback);
	glfwSetKeyCallback(mWindow, WindowCallbacks::key_callback);
	glfwSetCharCallback(mWindow, WindowCallbacks::char_callback);
	glfwSetMouseButtonCallback(mWindow, WindowCallbacks::mouse_button_callback);
	glfwSetWindowPosCallback(mWindow, WindowCallbacks::window_pos_callback);
	glfwSetWindowSizeCallback(mWindow, WindowCallbacks::window_size_callback);
	glfwSetWindowCloseCallback(mWindow, WindowCallbacks::window_close_callback);
	glfwSetScrollCallback(mWindow, WindowCallbacks::scroll_callback);
	glfwSetCursorPosCallback(mWindow, WindowCallbacks::cursor_pos_callback);
	glfwSetErrorCallback(WindowCallbacks::error_callback);
}

void WindowCallbacks::framebuffer_size_callback(GLFWwindow* window, int x, int y)
{
	WindowsWindow::UserData* me = (WindowsWindow::UserData*)glfwGetWindowUserPointer(window);

	me->props.width = x;
	me->props.height = y;

	//VulkanEngine::InvalidateSwapchain();
}

void WindowCallbacks::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	WindowsWindow::UserData* me = (WindowsWindow::UserData*)glfwGetWindowUserPointer(window);
}

void WindowCallbacks::char_callback(GLFWwindow* window, unsigned int keyCode)
{
	WindowsWindow::UserData* me = (WindowsWindow::UserData*)glfwGetWindowUserPointer(window);

}

void WindowCallbacks::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	WindowsWindow::UserData* me = (WindowsWindow::UserData*)glfwGetWindowUserPointer(window);
}

void WindowCallbacks::window_pos_callback(GLFWwindow* window, int x, int y)
{
	WindowsWindow::UserData* me = (WindowsWindow::UserData*)glfwGetWindowUserPointer(window);
}

void WindowCallbacks::window_size_callback(GLFWwindow* window, int x, int y)
{
	WindowsWindow::UserData* me = (WindowsWindow::UserData*)glfwGetWindowUserPointer(window);

	me->props.width = x;
	me->props.height = y;
}

void WindowCallbacks::window_close_callback(GLFWwindow* window)
{
	WindowsWindow::UserData* me = (WindowsWindow::UserData*)glfwGetWindowUserPointer(window);

}

void WindowCallbacks::scroll_callback(GLFWwindow* window, double x, double y)
{
	WindowsWindow::UserData* me = (WindowsWindow::UserData*)glfwGetWindowUserPointer(window);
}

void WindowCallbacks::cursor_pos_callback(GLFWwindow* window, double x, double y)
{
	WindowsWindow::UserData* me = (WindowsWindow::UserData*)glfwGetWindowUserPointer(window);
}

void WindowCallbacks::error_callback(int errCode, const char* errString)
{
	char buffer[1024];
	sprintf_s(buffer, "OpenGL window ran into an error: \nError Code: %x\nInfo: %s", errCode, errString);

	_STL_ASSERT(false, buffer);
}
