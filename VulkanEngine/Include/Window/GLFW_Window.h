#pragma once
#include "GLFW_Core.h"

struct GLFWwindow;

struct WindowProps
{
	std::string name;
	int width = 1280;
	int height = 800;
	bool vSync = true;
};

class WindowsWindow
{
public:
	WindowsWindow(const WindowProps& props);
	~WindowsWindow();

	struct UserData
	{
		WindowProps props;
		std::string APIversion;
		bool state;

		bool cursorHidden;

		void* mUserBuffer = nullptr;
	};

	WindowsWindow(const WindowsWindow&) = delete;
	WindowsWindow& operator=(const WindowsWindow&) = delete;

	void SetVSync(bool val);
	void ShutDown();
	//virtual void SetEventCallback(const EventCallback& eventCallback) override;

	bool IsWindowClosed() const;
	void OnUpdate() const;

	void SetTitle(const std::string& title);

	void PollUserEvents() const;

	void HideCursor();
	void RetrieveCursor();
	bool IsCursorHidden() const;

	void SwapFramebuffers();
	glm::vec2 GetCursorPosition() const;

	void HideCursor() const;
	void RetrieveCursor() const;

	void SetWindowDefaultStyle();

	glm::ivec2 GetWindowSize() const { return { mData.props.width, mData.props.height }; }
	inline GLFWwindow* GetNativeHandle() { return mWindow; }

private:
	GLFWwindow* mWindow;
	UserData mData;

	void SetWindowCallBacks();

	friend class WindowCallbacks;
};
