#include "RayTracingRenderer.hpp"
#include "RayTracingPipeline.hpp"
#include "Vulkan/RayTracing/DeviceProcedures.hpp"
#include "Vulkan/RayTracing/ShaderBindingTable.hpp"
#include "Vulkan/PipelineCommon/CommonComputePipeline.hpp"
#include "Utilities/Glm.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/BufferUtil.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageMemoryBarrier.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/SingleTimeCommands.hpp"
#include "Vulkan/SwapChain.hpp"
#include <chrono>
#include <iostream>
#include <numeric>



namespace Vulkan::RayTracing
{
    struct DenoiserPushConstantData
    {
        uint32_t pingpong;
        uint32_t stepsize;
    };

    namespace
    {
        template <class TAccelerationStructure>
        VkAccelerationStructureBuildSizesInfoKHR GetTotalRequirements(
            const std::vector<TAccelerationStructure>& accelerationStructures)
        {
            VkAccelerationStructureBuildSizesInfoKHR total{};

            for (const auto& accelerationStructure : accelerationStructures)
            {
                total.accelerationStructureSize += accelerationStructure.BuildSizes().accelerationStructureSize;
                total.buildScratchSize += accelerationStructure.BuildSizes().buildScratchSize;
                total.updateScratchSize += accelerationStructure.BuildSizes().updateScratchSize;
            }

            return total;
        }
    }

    RayTracingRenderer::RayTracingRenderer(const WindowConfig& windowConfig, const VkPresentModeKHR presentMode,
                             const bool enableValidationLayers) :
        RayTraceBaseRenderer(windowConfig, presentMode, enableValidationLayers)
    {
    }

    RayTracingRenderer::~RayTracingRenderer()
    {
        RayTracingRenderer::DeleteSwapChain();
        DeleteAccelerationStructures();
        rayTracingProperties_.reset();
        deviceProcedures_.reset();         
    }

    void RayTracingRenderer::SetPhysicalDeviceImpl(
        VkPhysicalDevice physicalDevice,
        std::vector<const char*>& requiredExtensions,
        VkPhysicalDeviceFeatures& deviceFeatures,
        void* nextDeviceFeatures)
    {
        supportRayTracing_ = true;
        
        // Required extensions.
        requiredExtensions.insert(requiredExtensions.end(),
                                  {
                                      VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
                                  });
        
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures = {};
        rayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rayTracingFeatures.pNext = nextDeviceFeatures;
        rayTracingFeatures.rayTracingPipeline = true;

        RayTraceBaseRenderer::SetPhysicalDeviceImpl(physicalDevice, requiredExtensions, deviceFeatures, &rayTracingFeatures);
    }

    void RayTracingRenderer::OnDeviceSet()
    {
        RayTraceBaseRenderer::OnDeviceSet();
    }

    void RayTracingRenderer::CreateSwapChain()
    {
        RayTraceBaseRenderer::CreateSwapChain();

        CreateOutputImage();

        rayTracingPipeline_.reset(new RayTracingPipeline(*deviceProcedures_, SwapChain(), topAs_[0],
                                                         *accumulationImageView_, *motionVectorImageView_,
                                                         *gbufferImageView_, *albedoImageView_, *visibilityBufferImageView_, *visibility1BufferImageView_,
                                                         UniformBuffers(), GetScene()));
        denoiserPipeline_.reset(new DenoiserPipeline(*deviceProcedures_, SwapChain(), topAs_[0], *pingpongImage0View_,
                                                     *pingpongImage1View_, *gbufferImageView_, *albedoImageView_,
                                                     UniformBuffers(), GetScene()));
        composePipeline_.reset(new ComposePipeline(*deviceProcedures_, SwapChain(), *pingpongImage0View_, *pingpongImage1View_,
                                                   *albedoImageView_, *outputImageView_, *motionVectorImageView_, UniformBuffers()));

        accumulatePipeline_.reset(new PipelineCommon::AccumulatePipeline(SwapChain(),
            *accumulationImageView_,
            *pingpongImage0View_,
            *pingpongImage1View_,
            *motionVectorImageView_,
            *visibilityBufferImageView_,
            *visibility1BufferImageView_,
            *outputImageView_,
            UniformBuffers(), GetScene()));
    
        
        const std::vector<ShaderBindingTable::Entry> rayGenPrograms = {{rayTracingPipeline_->RayGenShaderIndex(), {}}};
        const std::vector<ShaderBindingTable::Entry> missPrograms = {{rayTracingPipeline_->MissShaderIndex(), {}}};
        const std::vector<ShaderBindingTable::Entry> hitGroups = {
            {rayTracingPipeline_->TriangleHitGroupIndex(), {}}, {rayTracingPipeline_->ProceduralHitGroupIndex(), {}}
        };

        shaderBindingTable_.reset(new ShaderBindingTable(*deviceProcedures_, rayTracingPipeline_->Handle(),
                                                         *rayTracingProperties_, rayGenPrograms, missPrograms,
                                                         hitGroups));     
        
    }

    void RayTracingRenderer::DeleteSwapChain()
    {
        shaderBindingTable_.reset();
        accumulatePipeline_.reset();
        rayTracingPipeline_.reset();
        denoiserPipeline_.reset();
        composePipeline_.reset();
        outputImageView_.reset();
        outputImage_.reset();
        pingpongImage0_.reset();
        pingpongImage1_.reset();
        outputImageMemory_.reset();
        accumulationImageView_.reset();
        accumulationImage_.reset();
        accumulationImageMemory_.reset();
        gbufferImage_.reset();
        gbufferImageMemory_.reset();
        gbufferImageView_.reset();
        albedoImage_.reset();
        albedoImageView_.reset();
        albedoImageMemory_.reset();

        visibilityBufferImage_.reset();
        visibilityBufferImageMemory_.reset();
        visibilityBufferImageView_.reset();

        visibility1BufferImage_.reset();
        visibility1BufferImageMemory_.reset();
        visibility1BufferImageView_.reset();

        motionVectorImage_.reset();
        motionVectorImageView_.reset();
        motionVectorImageMemory_.reset();
        
        RayTraceBaseRenderer::DeleteSwapChain();
    }

    void RayTracingRenderer::Render(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
    {
        const auto extent = SwapChain().Extent();

        VkDescriptorSet descriptorSets[] = {rayTracingPipeline_->DescriptorSet(imageIndex)};

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        // Acquire destination images for rendering.
        ImageMemoryBarrier::Insert(commandBuffer, accumulationImage_->Handle(), subresourceRange, 0,
                                   VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        ImageMemoryBarrier::Insert(commandBuffer, pingpongImage0_->Handle(), subresourceRange, 0,
                                   VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        ImageMemoryBarrier::Insert(commandBuffer, gbufferImage_->Handle(), subresourceRange, 0,
                                   VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        ImageMemoryBarrier::Insert(commandBuffer, albedoImage_->Handle(), subresourceRange, 0,
                                   VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        ImageMemoryBarrier::Insert(commandBuffer, outputImage_->Handle(), subresourceRange, 0,
                                   VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        ImageMemoryBarrier::Insert(commandBuffer, pingpongImage1_->Handle(), subresourceRange, 0,
                                   VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        ImageMemoryBarrier::Insert(commandBuffer, motionVectorImage_->Handle(), subresourceRange,
               0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL);

        ImageMemoryBarrier::Insert(commandBuffer, visibilityBufferImage_->Handle(), subresourceRange,
                0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL);
        ImageMemoryBarrier::Insert(commandBuffer, visibility1BufferImage_->Handle(), subresourceRange,
                0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL);
        
        // Bind ray tracing pipeline.
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline_->Handle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                                rayTracingPipeline_->PipelineLayout().Handle(), 0, 1, descriptorSets, 0, nullptr);

        // Describe the shader binding table.
        VkStridedDeviceAddressRegionKHR raygenShaderBindingTable = {};
        raygenShaderBindingTable.deviceAddress = shaderBindingTable_->RayGenDeviceAddress();
        raygenShaderBindingTable.stride = shaderBindingTable_->RayGenEntrySize();
        raygenShaderBindingTable.size = shaderBindingTable_->RayGenSize();

        VkStridedDeviceAddressRegionKHR missShaderBindingTable = {};
        missShaderBindingTable.deviceAddress = shaderBindingTable_->MissDeviceAddress();
        missShaderBindingTable.stride = shaderBindingTable_->MissEntrySize();
        missShaderBindingTable.size = shaderBindingTable_->MissSize();

        VkStridedDeviceAddressRegionKHR hitShaderBindingTable = {};
        hitShaderBindingTable.deviceAddress = shaderBindingTable_->HitGroupDeviceAddress();
        hitShaderBindingTable.stride = shaderBindingTable_->HitGroupEntrySize();
        hitShaderBindingTable.size = shaderBindingTable_->HitGroupSize();

        VkStridedDeviceAddressRegionKHR callableShaderBindingTable = {};

        // Execute ray tracing shaders.
        {
            SCOPED_GPU_TIMER("rt pass");
            deviceProcedures_->vkCmdTraceRaysKHR(commandBuffer,
                                                &raygenShaderBindingTable, &missShaderBindingTable, &hitShaderBindingTable,
                                                &callableShaderBindingTable,
                                                CheckerboxRendering() ? extent.width / 2 : extent.width, extent.height, 1);

            ImageMemoryBarrier::Insert(commandBuffer, pingpongImage0_->Handle(), subresourceRange, 0,
                                    VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

            ImageMemoryBarrier::Insert(commandBuffer, gbufferImage_->Handle(), subresourceRange,
                                    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                                    VK_IMAGE_LAYOUT_GENERAL);

            ImageMemoryBarrier::Insert(commandBuffer, albedoImage_->Handle(), subresourceRange,
                                    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                                    VK_IMAGE_LAYOUT_GENERAL);

            // accumulate with reproject
            ImageMemoryBarrier::Insert(commandBuffer, motionVectorImage_->Handle(), subresourceRange,
                VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_GENERAL);
        }

        // accumulate with reproject
        // frame0: new + image 0 -> image 1
        // frame1: new + image 1 -> image 0
        {
            SCOPED_GPU_TIMER("reproject pass");
            VkDescriptorSet DescriptorSets[] = {accumulatePipeline_->DescriptorSet(imageIndex)};
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, accumulatePipeline_->Handle());
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    accumulatePipeline_->PipelineLayout().Handle(), 0, 1, DescriptorSets, 0, nullptr);
            vkCmdDispatch(commandBuffer, SwapChain().Extent().width / 8, SwapChain().Extent().height / 4, 1);
        }

        // ping & pong denoise
        // frame0: image 1 -> image 0 -> image 1 -> image 0 -> image 1
        // frame1: image 0 -> image 1 -> image 0 -> image 1 -> image 0
        for (int i = 0; i < denoiseIteration_ * 2; i++)
        {
            // pingpong & stepsize via push constants
            DenoiserPushConstantData pushData;
            pushData.pingpong = (frameCount_ + i) % 2;
            pushData.stepsize = i;

            vkCmdPushConstants(commandBuffer, denoiserPipeline_->PipelineLayout().Handle(), VK_SHADER_STAGE_COMPUTE_BIT,
                               0, sizeof(DenoiserPushConstantData), &pushData);

            // Execute Filter Kernel
            VkDescriptorSet denoiserDescriptorSets[] = {denoiserPipeline_->DescriptorSet(imageIndex)};
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, denoiserPipeline_->Handle());
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    denoiserPipeline_->PipelineLayout().Handle(), 0, 1, denoiserDescriptorSets, 0,
                                    nullptr);
            vkCmdDispatch(commandBuffer, extent.width / 8, extent.height / 4, 1);

            // make sure output image is ready
            ImageMemoryBarrier::Insert(commandBuffer, pingpongImage0_->Handle(), subresourceRange,
                                       VK_ACCESS_SHADER_WRITE_BIT,
                                       VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

            ImageMemoryBarrier::Insert(commandBuffer, pingpongImage1_->Handle(), subresourceRange,
                                       VK_ACCESS_SHADER_WRITE_BIT,
                                       VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
        }

        // compose with first bounce
        {
            SCOPED_GPU_TIMER("compose pass");

            // DenoiserPushConstantData pushData;
            // pushData.pingpong = frameCount_ % 2;
            // pushData.stepsize = 1;
            //
            // vkCmdPushConstants(commandBuffer, composePipeline_->PipelineLayout().Handle(), VK_SHADER_STAGE_COMPUTE_BIT,
            //                    0, sizeof(DenoiserPushConstantData), &pushData);
            //
            // VkDescriptorSet denoiserDescriptorSets[] = {composePipeline_->DescriptorSet(imageIndex)};
            // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, composePipeline_->Handle());
            // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            //                         composePipeline_->PipelineLayout().Handle(), 0, 1, denoiserDescriptorSets, 0, nullptr);
            // vkCmdDispatch(commandBuffer, extent.width / 8, extent.height / 4, 1);
        

            // Acquire output image and swap-chain image for copying.
            ImageMemoryBarrier::Insert(commandBuffer, outputImage_->Handle(), subresourceRange,
                                    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

            ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange, 0,
                                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        }

        // Copy output image into swap-chain image.
        VkImageCopy copyRegion;
        copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.srcOffset = {0, 0, 0};
        copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.dstOffset = {0, 0, 0};
        copyRegion.extent = {extent.width, extent.height, 1};

        vkCmdCopyImage(commandBuffer,
                       outputImage_->Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       SwapChain().Images()[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &copyRegion);

        ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange,
                                   VK_ACCESS_TRANSFER_WRITE_BIT,
                                   0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
    
    void RayTracingRenderer::CreateOutputImage()
    {
        const auto extent = SwapChain().Extent();
        const auto format = SwapChain().Format();
        const auto tiling = VK_IMAGE_TILING_OPTIMAL;

        accumulationImage_.reset(new Image(Device(), extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                           VK_IMAGE_USAGE_STORAGE_BIT));
        accumulationImageMemory_.reset(
            new DeviceMemory(accumulationImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        accumulationImageView_.reset(new ImageView(Device(), accumulationImage_->Handle(),
                                                   VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT));

        outputImage_.reset(new Image(Device(), extent, format, tiling,
                                     VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
        outputImageMemory_.reset(new DeviceMemory(outputImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        outputImageView_.reset(new ImageView(Device(), outputImage_->Handle(), format, VK_IMAGE_ASPECT_COLOR_BIT));

        pingpongImage0_.reset(new Image(Device(), extent, VK_FORMAT_R16G16B16A16_SFLOAT, tiling,
                                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
        pingpongImage0Memory_.reset(
            new DeviceMemory(pingpongImage0_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        pingpongImage0View_.reset(new ImageView(Device(), pingpongImage0_->Handle(), VK_FORMAT_R16G16B16A16_SFLOAT,
                                                VK_IMAGE_ASPECT_COLOR_BIT));

        pingpongImage1_.reset(new Image(Device(), extent, VK_FORMAT_R16G16B16A16_SFLOAT, tiling,
                                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
        pingpongImage1Memory_.reset(
            new DeviceMemory(pingpongImage1_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        pingpongImage1View_.reset(new ImageView(Device(), pingpongImage1_->Handle(), VK_FORMAT_R16G16B16A16_SFLOAT,
                                                VK_IMAGE_ASPECT_COLOR_BIT));

        gbufferImage_.reset(new Image(Device(), extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
        gbufferImageMemory_.reset(new DeviceMemory(gbufferImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        gbufferImageView_.reset(new ImageView(Device(), gbufferImage_->Handle(), VK_FORMAT_R32G32B32A32_SFLOAT,
                                              VK_IMAGE_ASPECT_COLOR_BIT));

        albedoImage_.reset(new Image(Device(), extent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                     VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
        albedoImageMemory_.reset(new DeviceMemory(albedoImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        albedoImageView_.reset(new ImageView(Device(), albedoImage_->Handle(), VK_FORMAT_R16G16B16A16_SFLOAT,
                                             VK_IMAGE_ASPECT_COLOR_BIT));

        motionVectorImage_.reset(new Image(Device(), extent, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                           VK_IMAGE_USAGE_STORAGE_BIT));
        motionVectorImageMemory_.reset(new DeviceMemory(motionVectorImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        motionVectorImageView_.reset( new ImageView(Device(), motionVectorImage_->Handle(), VK_FORMAT_R16G16_SFLOAT,
                                                   VK_IMAGE_ASPECT_COLOR_BIT) );

        visibilityBufferImage_.reset(new Image(Device(), extent,
        VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
        visibilityBufferImageMemory_.reset(
            new DeviceMemory(visibilityBufferImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        visibilityBufferImageView_.reset(new ImageView(Device(), visibilityBufferImage_->Handle(),
            VK_FORMAT_R32_UINT,
            VK_IMAGE_ASPECT_COLOR_BIT));

        visibility1BufferImage_.reset(new Image(Device(), extent,
        VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
        visibility1BufferImageMemory_.reset(
            new DeviceMemory(visibility1BufferImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        visibility1BufferImageView_.reset(new ImageView(Device(), visibility1BufferImage_->Handle(),
            VK_FORMAT_R32_UINT,
            VK_IMAGE_ASPECT_COLOR_BIT));
        
        const auto& debugUtils = Device().DebugUtils();

        debugUtils.SetObjectName(accumulationImage_->Handle(), "Accumulation Image");
        debugUtils.SetObjectName(accumulationImageMemory_->Handle(), "Accumulation Image Memory");
        debugUtils.SetObjectName(accumulationImageView_->Handle(), "Accumulation ImageView");

        debugUtils.SetObjectName(outputImage_->Handle(), "Output Image");
        debugUtils.SetObjectName(outputImageMemory_->Handle(), "Output Image Memory");
        debugUtils.SetObjectName(outputImageView_->Handle(), "Output ImageView");

        debugUtils.SetObjectName(gbufferImage_->Handle(), "Gbuffer Image");
        debugUtils.SetObjectName(gbufferImageMemory_->Handle(), "Gbuffer Image Memory");
        debugUtils.SetObjectName(gbufferImageView_->Handle(), "Gbuffer ImageView");
    }
}
