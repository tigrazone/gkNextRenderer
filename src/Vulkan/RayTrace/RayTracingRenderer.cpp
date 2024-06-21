#include "RayTracingRenderer.hpp"
#include "RayTracingPipeline.hpp"
#include "Vulkan/RayTracing/DeviceProcedures.hpp"
#include "Vulkan/RayTracing/ShaderBindingTable.hpp"
#include "Vulkan/PipelineCommon/CommonComputePipeline.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageMemoryBarrier.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/SingleTimeCommands.hpp"
#include "Vulkan/SwapChain.hpp"
#include <chrono>
#include <iostream>
#include <numeric>

#include "Vulkan/RenderImage.hpp"

#ifdef WIN32
#	include <aclapi.h>
#	include <dxgi1_2.h>
#endif

#if WITH_OIDN
#include "ThirdParty/oidn/include/oidn.hpp"
#endif

namespace Vulkan::RayTracing
{
#if WITH_OIDN
    oidn::DeviceRef device;
    oidn::BufferRef colorBuf;
    oidn::FilterRef filter[2];
#endif
    
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
        // try use amd gpu as denoise device
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
#if WIN32
        requiredExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
        requiredExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
#endif
        
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures = {};
        rayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rayTracingFeatures.pNext = nextDeviceFeatures;
        rayTracingFeatures.rayTracingPipeline = true;

        RayTraceBaseRenderer::SetPhysicalDeviceImpl(physicalDevice, requiredExtensions, deviceFeatures, &rayTracingFeatures);
    }

    void RayTracingRenderer::OnDeviceSet()
    {
#if WITH_OIDN
        // Query the UUID of the Vulkan physical device
        VkPhysicalDeviceIDProperties id_properties{};
        id_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

        VkPhysicalDeviceProperties2 properties{};
        properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties.pNext = &id_properties;
        vkGetPhysicalDeviceProperties2(Device().PhysicalDevice(), &properties);

        oidn::UUID uuid;
        std::memcpy(uuid.bytes, id_properties.deviceUUID, sizeof(uuid.bytes));
        
        device = oidn::newDevice(uuid); // CPU or GPU if available
        device.commit();
#endif
        
        RayTraceBaseRenderer::OnDeviceSet();
    }

    void RayTracingRenderer::CreateSwapChain()
    {
        RayTraceBaseRenderer::CreateSwapChain();

        CreateOutputImage();

        rayTracingPipeline_.reset(new RayTracingPipeline(*deviceProcedures_, SwapChain(), topAs_[0],
                                                         rtAccumulation_->GetImageView(), rtMotionVector_->GetImageView(),
                                                         rtVisibility0_->GetImageView(), rtVisibility1_->GetImageView(),
                                                         UniformBuffers(), GetScene()));

        accumulatePipeline_.reset(new PipelineCommon::AccumulatePipeline(SwapChain(),
            rtAccumulation_->GetImageView(),
            rtPingPong0->GetImageView(),
            rtPingPong1->GetImageView(),
            rtMotionVector_->GetImageView(),
            rtVisibility0_->GetImageView(),
            rtVisibility1_->GetImageView(),
            rtOutputForOIDN_->GetImageView(),
            UniformBuffers(), GetScene()));

        composePipeline_.reset(new PipelineCommon::FinalComposePipeline(SwapChain(), *oidnImageView_, *oidnImage1View_, UniformBuffers()));
    
        
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
        composePipeline_.reset();

        rtAccumulation_.reset();
        rtOutput_.reset();
        rtMotionVector_.reset();
        rtVisibility0_.reset();
        rtVisibility1_.reset();

        rtPingPong0.reset();
        rtPingPong1.reset();

        oidnImage_.reset();
        oidnImageMemory_.reset();
        oidnImageView_.reset();

        oidnImage1_.reset();
        oidnImage1Memory_.reset();
        oidnImage1View_.reset();
        
        RayTraceBaseRenderer::DeleteSwapChain();
    }

    void RayTracingRenderer::BeforeNextFrame()
    {
#if WITH_OIDN
        
#endif
    }

    void RayTracingRenderer::AfterPresent()
    {
#if WITH_OIDN
        
#endif
    }

    void RayTracingRenderer::AfterQuery()
    {
#if WITH_OIDN
        filter[ (frameCount_ + 1) % 2 ].executeAsync();
        device.sync();
#endif
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
        rtAccumulation_->InsertBarrier(commandBuffer, 0,VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        rtOutput_->InsertBarrier(commandBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        rtOutputForOIDN_->InsertBarrier(commandBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        rtMotionVector_->InsertBarrier(commandBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        rtVisibility0_->InsertBarrier(commandBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        rtVisibility1_->InsertBarrier(commandBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        rtPingPong0->InsertBarrier(commandBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        rtPingPong1->InsertBarrier(commandBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        
        // Bind ray tracing pipeline.
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline_->Handle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,rayTracingPipeline_->PipelineLayout().Handle(), 0, 1, descriptorSets, 0, nullptr);

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

            rtAccumulation_->InsertBarrier(commandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
            rtVisibility0_->InsertBarrier(commandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
            rtMotionVector_->InsertBarrier(commandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
        }

        // accumulate with reproject
        {
            SCOPED_GPU_TIMER("reproject pass");
            VkDescriptorSet DescriptorSets[] = {accumulatePipeline_->DescriptorSet(imageIndex)};
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, accumulatePipeline_->Handle());
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    accumulatePipeline_->PipelineLayout().Handle(), 0, 1, DescriptorSets, 0, nullptr);
            vkCmdDispatch(commandBuffer, SwapChain().Extent().width / 8, SwapChain().Extent().height / 4, 1);
        }
        
        {
            SCOPED_GPU_TIMER("compose");

            VkImage targetImage;
            if( frameCount_ % 2 == 0 )
            {
                targetImage = oidnImage_->Handle();
            }
            else
            {
                targetImage = oidnImage1_->Handle();
            }
            ImageMemoryBarrier::Insert(commandBuffer, targetImage, subresourceRange, 0,
                               VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_GENERAL);

            ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange, 0,
                                       VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_GENERAL);
            
            VkDescriptorSet DescriptorSets[] = {composePipeline_->DescriptorSet(imageIndex)};
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, composePipeline_->Handle());
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    composePipeline_->PipelineLayout().Handle(), 0, 1, DescriptorSets, 0, nullptr);
            vkCmdDispatch(commandBuffer, SwapChain().Extent().width / 8, SwapChain().Extent().height / 4, 1);

            ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange,
                                       VK_ACCESS_TRANSFER_WRITE_BIT,
                                       0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        }

        {
#if WITH_OIDN

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;
	           
            ImageMemoryBarrier::Insert(commandBuffer, rtOutputForOIDN_->GetImage().Handle(), subresourceRange,
                           VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

            VkImage targetImage;
            if( frameCount_ % 2 == 0 )
            {
                targetImage = oidnImage1_->Handle();
            }
            else
            {
                targetImage = oidnImage_->Handle();
            }

            ImageMemoryBarrier::Insert(commandBuffer, targetImage, subresourceRange, 0,
   VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            
            // Copy output image into swap-chain image.
            VkImageCopy copyRegion;
            copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copyRegion.srcOffset = {0, 0, 0};
            copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copyRegion.dstOffset = {0, 0, 0};
            copyRegion.extent = {SwapChain().Extent().width, SwapChain().Extent().height, 1};
            
            vkCmdCopyImage(commandBuffer,
                           rtOutputForOIDN_->GetImage().Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           targetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &copyRegion);

            ImageMemoryBarrier::Insert(commandBuffer, rtOutputForOIDN_->GetImage().Handle(), subresourceRange,
               VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
               VK_IMAGE_LAYOUT_GENERAL);
            // after cmdbuffer commit, we can readback the image
            #endif
        }
    }

    void RayTracingRenderer::CreateOutputImage()
    {
        const auto extent = SwapChain().Extent();
        const auto format = SwapChain().Format();
        
        rtAccumulation_.reset(new RenderImage(Device(), extent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,  VK_IMAGE_USAGE_STORAGE_BIT));
        rtOutput_.reset(new RenderImage(Device(), extent, format, VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
        rtOutputForOIDN_.reset(new RenderImage(Device(), extent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
        rtPingPong0.reset(new RenderImage(Device(), extent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT));
        rtPingPong1.reset(new RenderImage(Device(), extent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT));
        rtMotionVector_.reset(new RenderImage(Device(), extent, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_STORAGE_BIT));
        rtVisibility0_.reset(new RenderImage(Device(), extent, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT));
        rtVisibility1_.reset(new RenderImage(Device(), extent, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_STORAGE_BIT));

        oidnImage_.reset(new Image(Device(), extent, true));
        oidnImageMemory_.reset(new DeviceMemory(oidnImage_->AllocateExternalMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        oidnImageView_.reset(new ImageView(Device(), oidnImage_->Handle(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT));

        oidnImage1_.reset(new Image(Device(), extent, true));
        oidnImage1Memory_.reset(new DeviceMemory(oidnImage1_->AllocateExternalMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
        oidnImage1View_.reset(new ImageView(Device(), oidnImage1_->Handle(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT));

        {
            auto Extent = SwapChain().Extent();

            HANDLE extHandle;
            VkMemoryGetWin32HandleInfoKHR handleInfo = { VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR };
            handleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
            handleInfo.memory = oidnImageMemory_->Handle();
            if ( deviceProcedures_->vkGetMemoryWin32HandleKHR(Device().Handle(), &handleInfo, &extHandle) != VK_SUCCESS)
            {
                return;
            }

            size_t SrcImageSize = Extent.width * Extent.height * 4 * 2;
            colorBuf = device.newBuffer(oidn::ExternalMemoryTypeFlag::OpaqueWin32, extHandle, nullptr, SrcImageSize );
       
            // Create a filter for denoising a beauty (color) image using optional auxiliary images too
            // This can be an expensive operation, so try no to create a new filter for every image!
            filter[0] = device.newFilter("RT"); // generic ray tracing filter
            filter[0].setImage("color",  colorBuf,  oidn::Format::Half3, Extent.width, Extent.height, 0, 4 * 2, 4 * 2 * Extent.width); // beauty
            //filter.setImage("albedo", albedoBuf, oidn::Format::Float3, width, height); // auxiliary
            //filter.setImage("normal", normalBuf, oidn::Format::Float3, width, height); // auxiliary
            filter[0].setImage("output", colorBuf,  oidn::Format::Half3, Extent.width, Extent.height, 0, 4 * 2, 4 * 2 * Extent.width); // denoised beauty
            filter[0].set("hdr", true); // beauty image is HDR
            filter[0].set("quality", oidn::Quality::Fast); // beauty image is HDR
            filter[0].set("quality", oidn::Quality::Fast); // beauty image is HDR
            filter[0].commit();
        }

        {
            auto Extent = SwapChain().Extent();

            HANDLE extHandle;
            VkMemoryGetWin32HandleInfoKHR handleInfo = { VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR };
            handleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
            handleInfo.memory = oidnImage1Memory_->Handle();
            if ( deviceProcedures_->vkGetMemoryWin32HandleKHR(Device().Handle(), &handleInfo, &extHandle) != VK_SUCCESS)
            {
                return;
            }

            size_t SrcImageSize = Extent.width * Extent.height * 4 * 2;
            colorBuf = device.newBuffer(oidn::ExternalMemoryTypeFlag::OpaqueWin32, extHandle, nullptr, SrcImageSize );
       
            // Create a filter for denoising a beauty (color) image using optional auxiliary images too
            // This can be an expensive operation, so try no to create a new filter for every image!
            filter[1] = device.newFilter("RT"); // generic ray tracing filter
            filter[1].setImage("color",  colorBuf,  oidn::Format::Half3, Extent.width, Extent.height, 0, 4 * 2, 4 * 2 * Extent.width); // beauty
            //filter.setImage("albedo", albedoBuf, oidn::Format::Float3, width, height); // auxiliary
            //filter.setImage("normal", normalBuf, oidn::Format::Float3, width, height); // auxiliary
            filter[1].setImage("output", colorBuf,  oidn::Format::Half3, Extent.width, Extent.height, 0, 4 * 2, 4 * 2 * Extent.width); // denoised beauty
            filter[1].set("hdr", true); // beauty image is HDR
            filter[1].set("quality", oidn::Quality::Fast); // beauty image is HDR
            filter[1].commit();
        }
    }
}
