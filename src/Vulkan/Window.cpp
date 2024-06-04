#include "Window.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/StbImage.hpp"
#include <iostream>

namespace Vulkan {

namespace
{
#if !ANDROID
	void GlfwErrorCallback(const int error, const char* const description)
	{
		std::cerr << "ERROR: GLFW: " << description << " (code: " << error << ")" << std::endl;
	}

	void GlfwKeyCallback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods)
	{
		auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (this_->OnKey)
		{
			this_->OnKey(key, scancode, action, mods);
		}
	}

	void GlfwCursorPositionCallback(GLFWwindow* window, const double xpos, const double ypos)
	{
		auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (this_->OnCursorPosition)
		{
			this_->OnCursorPosition(xpos, ypos);
		}
	}

	void GlfwMouseButtonCallback(GLFWwindow* window, const int button, const int action, const int mods)
	{
		auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (this_->OnMouseButton)
		{
			this_->OnMouseButton(button, action, mods);
		}
	}

	void GlfwScrollCallback(GLFWwindow* window, const double xoffset, const double yoffset)
	{
		auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (this_->OnScroll)
		{
			this_->OnScroll(xoffset, yoffset);
		}
	}
#endif
}

Window::Window(const WindowConfig& config) :
	config_(config)
{
#if !ANDROID
	glfwSetErrorCallback(GlfwErrorCallback);

	if (!glfwInit())
	{
		Throw(std::runtime_error("glfwInit() failed"));
	}

	if (!glfwVulkanSupported())
	{
		Throw(std::runtime_error("glfwVulkanSupported() failed"));
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, config.Resizable ? GLFW_TRUE : GLFW_FALSE);

	auto* const monitor = config.Fullscreen ? glfwGetPrimaryMonitor() : nullptr;

	window_ = glfwCreateWindow(config.Width, config.Height, config.Title.c_str(), monitor, nullptr);
	if (window_ == nullptr)
	{
		Throw(std::runtime_error("failed to create window"));
	}

	GLFWimage icon;
	icon.pixels = stbi_load("../assets/textures/Vulkan.png", &icon.width, &icon.height, nullptr, 4);
	if (icon.pixels == nullptr)
	{
		Throw(std::runtime_error("failed to load icon"));
	}

	glfwSetWindowIcon(window_, 1, &icon);
	stbi_image_free(icon.pixels);

	if (config.CursorDisabled)
	{
		glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	glfwSetWindowUserPointer(window_, this);
	glfwSetKeyCallback(window_, GlfwKeyCallback);
	glfwSetCursorPosCallback(window_, GlfwCursorPositionCallback);
	glfwSetMouseButtonCallback(window_, GlfwMouseButtonCallback);
	glfwSetScrollCallback(window_, GlfwScrollCallback);
#endif
}

Window::~Window()
{
#if !ANDROID
	if (window_ != nullptr)
	{
		glfwDestroyWindow(window_);
		window_ = nullptr;
	}

	glfwTerminate();
	glfwSetErrorCallback(nullptr);
#endif
}

float Window::ContentScale() const
{
#if !ANDROID
	float xscale;
	float yscale;
	glfwGetWindowContentScale(window_, &xscale, &yscale);
#else
	float xscale = 1;
#endif
	return xscale;
}

VkExtent2D Window::FramebufferSize() const
{
#if !ANDROID
	int width, height;
	glfwGetFramebufferSize(window_, &width, &height);
	return VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
#else
	return VkExtent2D{ 1920, 1080 };
#endif
}

VkExtent2D Window::WindowSize() const
{
#if !ANDROID
	int width, height;
	glfwGetWindowSize(window_, &width, &height);
	return VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
#else
	return VkExtent2D{ 1920, 1080 };
#endif
}

const char* Window::GetKeyName(const int key, const int scancode) const
{
#if !ANDROID
	return glfwGetKeyName(key, scancode);
#else
	return "A";
#endif
}

std::vector<const char*> Window::GetRequiredInstanceExtensions() const
{
#if !ANDROID
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
#else
	return std::vector<const char*>();
#endif
}

double Window::GetTime() const
{
#if !ANDROID
	return glfwGetTime();
#else
	return 0;
#endif
}

void Window::Close()
{
#if !ANDROID
	glfwSetWindowShouldClose(window_, 1);
#endif
}

bool Window::IsMinimized() const
{
	const auto size = FramebufferSize();
	return size.height == 0 && size.width == 0;
}

void Window::Run()
{
#if !ANDROID
	glfwSetTime(0.0);

	while (!glfwWindowShouldClose(window_))
	{
		glfwPollEvents();

		if (DrawFrame)
		{
			DrawFrame();
		}
	}
#endif
}

void Window::WaitForEvents() const
{
#if !ANDROID
	glfwWaitEvents();
#endif
}

}
