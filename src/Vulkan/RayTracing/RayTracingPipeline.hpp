#pragma once

#include "Vulkan/Vulkan.hpp"
#include <memory>
#include <vector>

namespace Assets
{
	class Scene;
	class UniformBuffer;
}

namespace Vulkan
{
	class DescriptorSetManager;
	class ImageView;
	class PipelineLayout;
	class SwapChain;
}

namespace Vulkan::RayTracing
{
	class DeviceProcedures;
	class TopLevelAccelerationStructure;

	class RayTracingPipeline final
	{
	public:

		VULKAN_NON_COPIABLE(RayTracingPipeline)

		RayTracingPipeline(
			const DeviceProcedures& deviceProcedures,
			const SwapChain& swapChain,
			const TopLevelAccelerationStructure& accelerationStructure,
			const ImageView& accumulationImageView,
			const ImageView& outputImageView,
			const ImageView& gbufferImageView,
			const std::vector<Assets::UniformBuffer>& uniformBuffers,
			const Assets::Scene& scene);
		~RayTracingPipeline();

		uint32_t RayGenShaderIndex() const { return rayGenIndex_; }
		uint32_t MissShaderIndex() const { return missIndex_; }
		uint32_t TriangleHitGroupIndex() const { return triangleHitGroupIndex_; }
		uint32_t ProceduralHitGroupIndex() const { return proceduralHitGroupIndex_; }

		VkDescriptorSet DescriptorSet(uint32_t index) const;
		const class PipelineLayout& PipelineLayout() const { return *rayTracePipelineLayout_; }
	public:
		VkPipeline denoiserPipeline_{};
		std::unique_ptr<DescriptorSetManager> denoiserDescriptorSetManager_;
		std::unique_ptr<class PipelineLayout> denoisePipelineLayout_;
	private:

		const SwapChain& swapChain_;

		VULKAN_HANDLE(VkPipeline, pipeline_)

		std::unique_ptr<DescriptorSetManager> descriptorSetManager_;
		std::unique_ptr<class PipelineLayout> rayTracePipelineLayout_;
		
		
		


		uint32_t rayGenIndex_;
		uint32_t missIndex_;
		uint32_t triangleHitGroupIndex_;
		uint32_t proceduralHitGroupIndex_;
	};

}
