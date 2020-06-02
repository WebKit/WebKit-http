//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TransformFeedbackVk.cpp:
//    Implements the class methods for TransformFeedbackVk.
//

#include "libANGLE/renderer/vulkan/TransformFeedbackVk.h"

#include "libANGLE/Context.h"
#include "libANGLE/Query.h"
#include "libANGLE/renderer/glslang_wrapper_utils.h"
#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/ProgramVk.h"
#include "libANGLE/renderer/vulkan/QueryVk.h"

#include "common/debug.h"

namespace rx
{

TransformFeedbackVk::TransformFeedbackVk(const gl::TransformFeedbackState &state)
    : TransformFeedbackImpl(state),
      mRebindTransformFeedbackBuffer(false),
      mBufferHelpers{},
      mBufferHandles{},
      mBufferOffsets{},
      mBufferSizes{},
      mAlignedBufferOffsets{},
      mCounterBufferHandles{}
{}

TransformFeedbackVk::~TransformFeedbackVk() {}

void TransformFeedbackVk::onDestroy(const gl::Context *context)
{
    RendererVk *rendererVk = vk::GetImpl(context)->getRenderer();

    for (vk::BufferHelper &bufferHelper : mCounterBufferHelpers)
    {
        bufferHelper.release(rendererVk);
    }
}

angle::Result TransformFeedbackVk::begin(const gl::Context *context,
                                         gl::PrimitiveMode primitiveMode)
{
    ContextVk *contextVk = vk::GetImpl(context);

    contextVk->onTransformFeedbackStateChanged();

    const gl::ProgramExecutable *executable = contextVk->getState().getProgramExecutable();
    ASSERT(executable);
    size_t xfbBufferCount = executable->getTransformFeedbackBufferCount(contextVk->getState());

    for (size_t bufferIndex = 0; bufferIndex < xfbBufferCount; ++bufferIndex)
    {
        const gl::OffsetBindingPointer<gl::Buffer> &binding = mState.getIndexedBuffer(bufferIndex);
        ASSERT(binding.get());
        mBufferHelpers[bufferIndex] = &vk::GetImpl(binding.get())->getBuffer();
        mBufferHandles[bufferIndex] = mBufferHelpers[bufferIndex]->getBuffer().getHandle();
        mBufferOffsets[bufferIndex] = binding.getOffset();
        mBufferSizes[bufferIndex]   = gl::GetBoundBufferAvailableSize(binding);

        if (contextVk->getFeatures().supportsTransformFeedbackExtension.enabled)
        {
            if (mCounterBufferHandles[bufferIndex] == VK_NULL_HANDLE)
            {
                VkBufferCreateInfo createInfo = {};
                createInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                createInfo.size               = 16;
                createInfo.usage       = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT;
                createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                vk::BufferHelper &bufferHelper = mCounterBufferHelpers[bufferIndex];
                ANGLE_TRY(
                    bufferHelper.init(contextVk, createInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

                mCounterBufferHandles[bufferIndex] = bufferHelper.getBuffer().getHandle();
            }
        }
        else
        {
            ASSERT(contextVk->getFeatures().emulateTransformFeedback.enabled);
            RendererVk *rendererVk = contextVk->getRenderer();
            const VkDeviceSize offsetAlignment =
                rendererVk->getPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;

            // Make sure there's no possible under/overflow with binding size.
            static_assert(sizeof(VkDeviceSize) >= sizeof(binding.getSize()),
                          "VkDeviceSize too small");

            // Set the offset as close as possible to the requested offset while remaining aligned.
            mAlignedBufferOffsets[bufferIndex] =
                (mBufferOffsets[bufferIndex] / offsetAlignment) * offsetAlignment;
        }
    }

    if (contextVk->getFeatures().supportsTransformFeedbackExtension.enabled)
    {
        mRebindTransformFeedbackBuffer = true;
    }

    return onTransformFeedbackStateChanged(contextVk);
}

angle::Result TransformFeedbackVk::end(const gl::Context *context)
{
    // If there's an active transform feedback query, accumulate the primitives drawn.
    const gl::State &glState = context->getState();
    gl::Query *transformFeedbackQuery =
        glState.getActiveQuery(gl::QueryType::TransformFeedbackPrimitivesWritten);

    if (transformFeedbackQuery)
    {
        vk::GetImpl(transformFeedbackQuery)->onTransformFeedbackEnd(context);
    }

    ContextVk *contextVk = vk::GetImpl(context);
    contextVk->onTransformFeedbackStateChanged();

    return onTransformFeedbackStateChanged(contextVk);
}

angle::Result TransformFeedbackVk::pause(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);

    contextVk->onTransformFeedbackStateChanged();

    if (contextVk->getFeatures().supportsTransformFeedbackExtension.enabled)
    {
        // We need to end the RenderPass to perform transform feedback pause/resume
        // becasue vkCmdBegin/EndTransformFeedback can be placed once per RenderPass.
        ANGLE_TRY(onTransformFeedbackStateChanged(contextVk));
    }

    return angle::Result::Continue;
}

angle::Result TransformFeedbackVk::resume(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);

    contextVk->onTransformFeedbackStateChanged();

    if (contextVk->getFeatures().supportsTransformFeedbackExtension.enabled)
    {
        // We need to end the RenderPass to perform transform feedback pause/resume
        // becasue vkCmdBegin/EndTransformFeedback can be placed once per RenderPass.
        ANGLE_TRY(onTransformFeedbackStateChanged(contextVk));
    }

    return angle::Result::Continue;
}

angle::Result TransformFeedbackVk::bindIndexedBuffer(
    const gl::Context *context,
    size_t index,
    const gl::OffsetBindingPointer<gl::Buffer> &binding)
{
    ContextVk *contextVk = vk::GetImpl(context);

    // Make sure the transform feedback buffers are bound to the program descriptor sets.
    contextVk->invalidateCurrentTransformFeedbackBuffers();

    return angle::Result::Continue;
}

void TransformFeedbackVk::updateDescriptorSetLayout(
    ContextVk *contextVk,
    ShaderInterfaceVariableInfoMap &vsVariableInfoMap,
    size_t xfbBufferCount,
    vk::DescriptorSetLayoutDesc *descSetLayoutOut) const
{
    if (!contextVk->getFeatures().emulateTransformFeedback.enabled)
        return;

    for (uint32_t bufferIndex = 0; bufferIndex < xfbBufferCount; ++bufferIndex)
    {
        const std::string bufferName            = GetXfbBufferName(bufferIndex);
        const ShaderInterfaceVariableInfo &info = vsVariableInfoMap[bufferName];

        descSetLayoutOut->update(info.binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_VERTEX_BIT);
    }
}

void TransformFeedbackVk::initDescriptorSet(ContextVk *contextVk,
                                            size_t xfbBufferCount,
                                            vk::BufferHelper *emptyBuffer,
                                            VkDescriptorSet descSet) const
{
    if (!contextVk->getFeatures().emulateTransformFeedback.enabled)
        return;

    gl::TransformFeedbackBuffersArray<VkDescriptorBufferInfo> descriptorBufferInfo;

    for (size_t bufferIndex = 0; bufferIndex < xfbBufferCount; ++bufferIndex)
    {
        VkDescriptorBufferInfo &bufferInfo = descriptorBufferInfo[bufferIndex];
        bufferInfo.buffer                  = emptyBuffer->getBuffer().getHandle();
        bufferInfo.offset                  = 0;
        bufferInfo.range                   = VK_WHOLE_SIZE;
    }

    writeDescriptorSet(contextVk, xfbBufferCount, descriptorBufferInfo.data(), descSet);
}

void TransformFeedbackVk::updateDescriptorSet(ContextVk *contextVk,
                                              const gl::ProgramState &programState,
                                              VkDescriptorSet descSet) const
{
    if (!contextVk->getFeatures().emulateTransformFeedback.enabled)
        return;

    const gl::ProgramExecutable *executable = contextVk->getState().getProgramExecutable();
    ASSERT(executable);
    size_t xfbBufferCount = executable->getTransformFeedbackBufferCount(contextVk->getState());

    ASSERT(xfbBufferCount > 0);
    ASSERT(programState.getTransformFeedbackBufferMode() != GL_INTERLEAVED_ATTRIBS ||
           xfbBufferCount == 1);

    gl::TransformFeedbackBuffersArray<VkDescriptorBufferInfo> descriptorBufferInfo;

    // Update buffer descriptor binding info for output buffers
    for (size_t bufferIndex = 0; bufferIndex < xfbBufferCount; ++bufferIndex)
    {
        VkDescriptorBufferInfo &bufferInfo = descriptorBufferInfo[bufferIndex];

        bufferInfo.buffer = mBufferHandles[bufferIndex];
        bufferInfo.offset = mAlignedBufferOffsets[bufferIndex];
        bufferInfo.range  = mBufferSizes[bufferIndex] +
                           (mBufferOffsets[bufferIndex] - mAlignedBufferOffsets[bufferIndex]);
        ASSERT(bufferInfo.range != 0);
    }

    writeDescriptorSet(contextVk, xfbBufferCount, descriptorBufferInfo.data(), descSet);
}

void TransformFeedbackVk::getBufferOffsets(ContextVk *contextVk,
                                           GLint drawCallFirstVertex,
                                           int32_t *offsetsOut,
                                           size_t offsetsSize) const
{
    if (!contextVk->getFeatures().emulateTransformFeedback.enabled)
        return;

    GLsizeiptr verticesDrawn = mState.getVerticesDrawn();
    const std::vector<GLsizei> &bufferStrides =
        mState.getBoundProgram()->getTransformFeedbackStrides();
    const gl::ProgramExecutable *executable = contextVk->getState().getProgramExecutable();
    ASSERT(executable);
    size_t xfbBufferCount = executable->getTransformFeedbackBufferCount(contextVk->getState());

    ASSERT(xfbBufferCount > 0);

    // The caller should make sure the offsets array has enough space.  The maximum possible
    // number of outputs is gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS.
    ASSERT(offsetsSize >= xfbBufferCount);

    for (size_t bufferIndex = 0; bufferIndex < xfbBufferCount; ++bufferIndex)
    {
        int64_t offsetFromDescriptor =
            static_cast<int64_t>(mBufferOffsets[bufferIndex] - mAlignedBufferOffsets[bufferIndex]);
        int64_t drawCallVertexOffset = static_cast<int64_t>(verticesDrawn) - drawCallFirstVertex;

        int64_t writeOffset =
            (offsetFromDescriptor + drawCallVertexOffset * bufferStrides[bufferIndex]) /
            static_cast<int64_t>(sizeof(uint32_t));

        offsetsOut[bufferIndex] = static_cast<int32_t>(writeOffset);

        // Assert on overflow.  For now, support transform feedback up to 2GB.
        ASSERT(offsetsOut[bufferIndex] == writeOffset);
    }
}

angle::Result TransformFeedbackVk::onTransformFeedbackStateChanged(ContextVk *contextVk)
{
    // Currently, we don't handle resources switching from read-only to writable and back correctly.
    // In the case of transform feedback, the attached buffers can switch between being written by
    // transform feedback and being read as a vertex array buffer.  For now, we'll end the render
    // pass every time transform feedback is started or ended to work around this issue temporarily.
    //
    // TODO(syoussefi): detect changes to buffer usage (e.g. as transform feedback output, vertex
    // or index data etc) in the front end and notify the backend.  A new node should be created
    // only on such changes.  http://anglebug.com/3205
    ANGLE_TRY(contextVk->endRenderPass());
    return angle::Result::Continue;
}

void TransformFeedbackVk::writeDescriptorSet(ContextVk *contextVk,
                                             size_t xfbBufferCount,
                                             VkDescriptorBufferInfo *pBufferInfo,
                                             VkDescriptorSet descSet) const
{
    VkDevice device                   = contextVk->getDevice();
    ProgramExecutableVk *executableVk = contextVk->getExecutable();
    ShaderMapInterfaceVariableInfoMap variableInfoMap =
        executableVk->getShaderInterfaceVariableInfoMap();
    const std::string bufferName      = GetXfbBufferName(0);
    ShaderInterfaceVariableInfo &info = variableInfoMap[gl::ShaderType::Vertex][bufferName];

    VkWriteDescriptorSet writeDescriptorInfo = {};
    writeDescriptorInfo.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorInfo.dstSet               = descSet;
    writeDescriptorInfo.dstBinding           = info.binding;
    writeDescriptorInfo.dstArrayElement      = 0;
    writeDescriptorInfo.descriptorCount      = static_cast<uint32_t>(xfbBufferCount);
    writeDescriptorInfo.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorInfo.pImageInfo           = nullptr;
    writeDescriptorInfo.pBufferInfo          = pBufferInfo;
    writeDescriptorInfo.pTexelBufferView     = nullptr;

    vkUpdateDescriptorSets(device, 1, &writeDescriptorInfo, 0, nullptr);
}

}  // namespace rx
