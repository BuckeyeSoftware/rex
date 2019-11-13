INST_FUN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
INST_FUN(vkGetPhysicalDeviceSurfacePresentModesKHR)
INST_FUN(vkGetPhysicalDeviceSurfaceFormatsKHR)

DEV_FUN(vkCreateSwapchainKHR)
DEV_FUN(vkDestroySwapchainKHR)
DEV_FUN(vkGetSwapchainImagesKHR)
DEV_FUN(vkAcquireNextImageKHR)
DEV_FUN(vkQueuePresentKHR)

DEV_FUN(vkCreateBuffer)
DEV_FUN(vkDestroyBuffer)
DEV_FUN(vkCreateImage)
DEV_FUN(vkDestroyImage)
DEV_FUN(vkCreateImageView)
DEV_FUN(vkDestroyImageView)
DEV_FUN(vkGetBufferMemoryRequirements)
DEV_FUN(vkGetImageMemoryRequirements)
DEV_FUN(vkBindBufferMemory)
DEV_FUN(vkBindImageMemory)
DEV_FUN(vkAllocateMemory)
DEV_FUN(vkFreeMemory)
DEV_FUN(vkMapMemory)
DEV_FUN(vkUnmapMemory)

DEV_FUN(vkCreateCommandPool)
DEV_FUN(vkDestroyCommandPool)
DEV_FUN(vkAllocateCommandBuffers)
DEV_FUN(vkBeginCommandBuffer)
DEV_FUN(vkEndCommandBuffer)

DEV_FUN(vkCmdPipelineBarrier)
DEV_FUN(vkCmdCopyBuffer)
DEV_FUN(vkCmdCopyBufferToImage)

DEV_FUN(vkCreateDescriptorPool)
DEV_FUN(vkDestroyDescriptorPool)
DEV_FUN(vkCreateDescriptorSetLayout)
DEV_FUN(vkDestroyDescriptorSetLayout)
DEV_FUN(vkAllocateDescriptorSets)
DEV_FUN(vkUpdateDescriptorSets)

DEV_FUN(vkCreateFence)
DEV_FUN(vkDestroyFence)
DEV_FUN(vkWaitForFences)
DEV_FUN(vkResetFences)

DEV_FUN(vkCreateSemaphore)
DEV_FUN(vkDestroySemaphore)

DEV_FUN(vkQueueSubmit)

DEV_FUN(vkCreateFramebuffer)
DEV_FUN(vkDestroyFramebuffer)
DEV_FUN(vkCreateRenderPass)
DEV_FUN(vkDestroyRenderPass)

DEV_FUN(vkCmdBlitImage)
DEV_FUN(vkCmdDraw)

#if defined(RX_DEBUG)
DEV_FUN(vkSetDebugUtilsObjectNameEXT)
#endif
