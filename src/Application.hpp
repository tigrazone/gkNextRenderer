#pragma once

#include "ModelViewController.hpp"
#include "SceneList.hpp"
#include "UserSettings.hpp"
#include "Assets/UniformBuffer.hpp"
#include "Assets/Model.hpp"
#include "Vulkan/FrameBuffer.hpp"
#include "Vulkan/WindowConfig.hpp"
#include "Vulkan/VulkanBaseRenderer.hpp"

#include <fstream>

namespace NextRenderer
{
	enum class EApplicationStatus
	{
		Starting,
		Running,
		Loading,
		AsyncPreparing,
	};

	std::string GetBuildVersion();
}

class NextRendererApplication final
{
public:

	VULKAN_NON_COPIABLE(NextRendererApplication)

	NextRendererApplication(uint32_t rendererType, const UserSettings& userSettings, Vulkan::Window* window, VkPresentModeKHR presentMode);
	~NextRendererApplication();

	Vulkan::VulkanBaseRenderer& GetRenderer() { return *renderer_; }

	void Start();
	bool Tick();
	void End();
protected:
	
	Assets::UniformBufferObject GetUniformBufferObject(const VkOffset2D offset, const VkExtent2D extent) const;
	void OnDeviceSet();
	void CreateSwapChain();
	void DeleteSwapChain();
	void DrawFrame();
	void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void RenderUI(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void BeforeNextFrame();

	const Assets::Scene& GetScene() const { return *scene_; }
	
	void OnKey(int key, int scancode, int action, int mods);
	void OnCursorPosition(double xpos, double ypos);
	void OnMouseButton(int button, int action, int mods);
	void OnScroll(double xoffset, double yoffset);
	void OnDropFile(int path_count, const char* paths[]);
	void OnTouch(bool down, double xpos, double ypos);
	void OnTouchMove(double xpos, double ypos);
	
	Vulkan::Window& GetWindow() {return *window_;}

	
private:

	void LoadScene(uint32_t sceneIndex);
	void CheckAndUpdateBenchmarkState(double prevTime);
	void CheckFramebufferSize();

	void Report(int fps, const std::string& sceneName, bool upload_screen, bool save_screen);

	Vulkan::Window* window_;
	std::unique_ptr<Vulkan::VulkanBaseRenderer> renderer_;

	uint32_t sceneIndex_{((uint32_t)~((uint32_t)0))};
	mutable UserSettings userSettings_{};
	UserSettings previousSettings_{};
	Assets::CameraInitialSate cameraInitialSate_{};

	mutable ModelViewController modelViewController_{};

	mutable Assets::UniformBufferObject prevUBO_ {};

	std::shared_ptr<Assets::Scene> scene_;
	std::unique_ptr<class UserInterface> userInterface_;

	double time_{};

	NextRenderer::EApplicationStatus status_{};

	uint32_t totalFrames_{};

	// Benchmark stats
	double sceneInitialTime_{};
	double periodInitialTime_{};
	uint32_t periodTotalFrames_{};
	uint32_t benchmarkTotalFrames_{};
	uint32_t benchmarkNumber_{0};
	std::ofstream benchmarkCsvReportFile;

	glm::vec2 mousePos_ {};
};
