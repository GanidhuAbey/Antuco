#include <environment/prefilter_map.hpp>
#include <antuco.hpp>
#include <api_graphics.hpp>

#include <cubemap.hpp>

using namespace tuco;

void PrefilterMap::override_pipeline(PipelineConfig& config)
{
	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(glm::vec4);

	config.push_ranges.push_back(pushRange);
}

void PrefilterMap::record_command_buffer(uint32_t face, VkCommandBuffer command_buffer)
{
	for (uint32_t mip = 0; mip < mip_count; mip++)
	{
		uint32_t mip_size = map_size * std::pow(0.5, mip);

		mem::StackBuffer& vertex_buffer = Antuco::get_engine().get_backend()->get_vertex_buffer();
		mem::StackBuffer& index_buffer = Antuco::get_engine().get_backend()->get_index_buffer();

		std::vector<VkClearValue> clear_values;

		VkClearValue color_clear{};
		color_clear.color.float32[0] = 0.f;
		color_clear.color.float32[1] = 0.f;
		color_clear.color.float32[2] = 0.f;
		color_clear.color.float32[3] = 0.f;

		clear_values.push_back(color_clear);

		VkRect2D render_area{};
		render_area.offset.x = 0;
		render_area.offset.y = 0;
		render_area.extent.height = mip_size;
		render_area.extent.width = mip_size;

		VkRenderPassBeginInfo skybox_info{};
		skybox_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		skybox_info.framebuffer = outputs[INDEX(face, mip, mip_count)].get_api_buffer();
		skybox_info.renderArea = render_area;
		skybox_info.renderPass = pass.get_api_pass();
		skybox_info.clearValueCount = clear_values.size();
		skybox_info.pClearValues = clear_values.data();

		input_image->change_layout(vk::ImageLayout::eShaderReadOnlyOptimal, device_->get_graphics_queue());

		vkCmdBeginRenderPass(command_buffer, &skybox_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport newViewport{};
		newViewport.x = 0;
		newViewport.y = 0;
		newViewport.width = mip_size;
		newViewport.height = mip_size;
		newViewport.minDepth = 0.0;
		newViewport.maxDepth = 1.0;

		vkCmdSetViewport(command_buffer, 0, 1, &newViewport);

		VkRect2D scissor{};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.height = mip_size;
		scissor.extent.width = mip_size;

		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		const VkDeviceSize offsets[] = { 0, offsetof(Vertex, normal), offsetof(Vertex, tex_coord) };
		VkBuffer v_buffer = vertex_buffer.buffer;
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &v_buffer, offsets);

		vkCmdBindIndexBuffer(command_buffer, index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_api_pipeline());

		ResourceCollection* collection = pipeline.get_resource_collection(0);
		VkDescriptorSet set = collection->get_api_set(face);

		//vkCmdDraw(command_buffers[i], 3, 1, 0, 0);

		glm::vec4 roughness = glm::vec4((float)mip / (float)(mip_count - 1), 0.0, 0.0, 0.0);
		vkCmdPushConstants(command_buffer,
						   pipeline.get_api_layout(),
						   VK_SHADER_STAGE_FRAGMENT_BIT, 0,
						   sizeof(glm::vec4), &roughness);

		std::vector<Primitive>& prims = cubemap_model->object_model.get_prims();
		for (int j = 0; j < prims.size(); j++)
		{
			vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_api_layout(), 0, 1, &set, 0, nullptr);

			vkCmdDrawIndexed(command_buffer, prims[j].index_count, 1, prims[j].index_start + cubemap_model->buffer_index_offset, cubemap_model->buffer_vertex_offset, 0);
		}

		// end render pass
		vkCmdEndRenderPass(command_buffer);
	}
}