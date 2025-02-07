#include <cubemap.hpp>
#include <api_config.hpp>

#include <vector>
#include <antuco.hpp>
#include <api_graphics.hpp>

using namespace tuco;

#define SKYBOX_NAMES {"px.png", "nx.png", "py.png", "ny.png", "pz.png", "nz.png"}
#define CUBEMAP_FACES 6
#define SKYBOX_SIZE 1024

void Cubemap::init(std::string file_path, GameObject* model)
{
	cubemap_model = model;

	device_ = Antuco::get_engine().get_backend()->p_device;
	physical_device_ = Antuco::get_engine().get_backend()->p_physical_device;
	set_pool_ = Antuco::get_engine().get_backend()->get_set_pool();

	// create render pass
	create_skybox_pass();

	// create pipeline to generate skybox.
	create_skybox_pipeline();

	// create 6 output images 
	create_cubemap_faces();

	// create frame buffer
	create_skybox_framebuffers();

	// record command buffers
	command_pool_.init(device_, device_->get_graphics_family());
	record_command_buffers();

	// create render frame
	gpu_sync.resize(MAX_FRAMES_IN_FLIGHT);
	cpu_sync.resize(MAX_FRAMES_IN_FLIGHT);
	render_to_image();

	// render 6 times to each image

	// collect into final cubemap image
	cubemap_faces[0].set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void Cubemap::render_to_image()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkSemaphoreCreateInfo gpu_sync_info{};
		gpu_sync_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo cpu_sync_info{};
		cpu_sync_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		cpu_sync_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		vkCreateSemaphore(device_->get(), &gpu_sync_info, nullptr, &gpu_sync[i]);
		vkCreateFence(device_->get(), &cpu_sync_info, nullptr, &cpu_sync[i]);
	}

	//vkResetFences(device_->get(), MAX_FRAMES_IN_FLIGHT, cpu_sync.data());

	int32_t prev_image = -1;
	int32_t curr_image = (prev_image + 1) % MAX_FRAMES_IN_FLIGHT;
	for (int i = 0; i < CUBEMAP_FACES; i++)
	{

		vkWaitForFences(device_->get(), 1, &cpu_sync[curr_image], VK_TRUE, UINT64_MAX);
		vkResetFences(device_->get(), 1, &cpu_sync[curr_image]);

		VkSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pNext = nullptr;
		info.waitSemaphoreCount = prev_image == -1 ? 0 : 1;
		info.pWaitSemaphores = prev_image == -1 ? nullptr : &gpu_sync[prev_image];
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &gpu_sync[curr_image];
		info.commandBufferCount = 1;
		info.pCommandBuffers = &command_buffers[i];
		VkPipelineStageFlags stages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		info.pWaitDstStageMask = &stages;

		vkQueueSubmit(device_->get_graphics_queue(), 1, &info, cpu_sync[curr_image]);

		prev_image = curr_image;
		curr_image = (curr_image + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	vkWaitForFences(device_->get(), MAX_FRAMES_IN_FLIGHT, cpu_sync.data(), VK_TRUE, UINT64_MAX);
}

void Cubemap::record_command_buffers()
{
	command_buffers.resize(CUBEMAP_FACES);
	command_pool_.allocate_command_buffers(CUBEMAP_FACES, command_buffers.data());

	mem::StackBuffer& vertex_buffer = Antuco::get_engine().get_backend()->get_vertex_buffer();
	mem::StackBuffer& index_buffer = Antuco::get_engine().get_backend()->get_index_buffer();


	for (int i = 0; i < CUBEMAP_FACES; i++)
	{
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;                  // Optional
		begin_info.pInheritanceInfo = nullptr; // Optional

		vkBeginCommandBuffer(command_buffers[i], &begin_info);

		VkClearValue color_clear{};
		color_clear.color.float32[0] = 0.f;
		color_clear.color.float32[1] = 0.f;
		color_clear.color.float32[2] = 0.f;
		color_clear.color.float32[3] = 0.f;

		VkRect2D render_area{};
		render_area.offset.x = 0;
		render_area.offset.y = 0;
		render_area.extent.height = SKYBOX_SIZE;
		render_area.extent.width = SKYBOX_SIZE;

		VkRenderPassBeginInfo skybox_info{};
		skybox_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		skybox_info.framebuffer = skybox_outputs[i].get_api_buffer();
		skybox_info.renderArea = render_area;
		skybox_info.renderPass = skybox_pass.get_api_pass();
		skybox_info.clearValueCount = 1;
		skybox_info.pClearValues = &color_clear;

		vkCmdBeginRenderPass(command_buffers[i], &skybox_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport newViewport{};
		newViewport.x = 0;
		newViewport.y = 0;
		newViewport.width = SKYBOX_SIZE;
		newViewport.height = SKYBOX_SIZE;
		newViewport.minDepth = 0.0;
		newViewport.maxDepth = 1.0;

		vkCmdSetViewport(command_buffers[i], 0, 1, &newViewport);

		VkRect2D scissor{};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.height = SKYBOX_SIZE;
		scissor.extent.width = SKYBOX_SIZE;

		vkCmdSetScissor(command_buffers[i], 0, 1, &scissor);

		const VkDeviceSize offsets[] = { 0, offsetof(Vertex, normal), offsetof(Vertex, tex_coord) };
		VkBuffer v_buffer = vertex_buffer.buffer;
		//vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &v_buffer, offsets);

		//vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline.get_api_pipeline());

		vkCmdDraw(command_buffers[i], 3, 1, 0, 0);

		std::vector<Primitive>& prims = cubemap_model->object_model.get_prims();
		for (int j = 0; j < prims.size(); j++)
		{

			//vkCmdDrawIndexed(command_buffers[i], prims[j].index_count, 1, prims[j].index_start + cubemap_model->buffer_index_offset, cubemap_model->buffer_vertex_offset, 0);
		}

		// end render pass
		vkCmdEndRenderPass(command_buffers[i]);

		// end command buffer recording
		ASSERT(vkEndCommandBuffer(command_buffers[i]) == VK_SUCCESS, "could not successfully record command buffer");
	}
}

void Cubemap::create_skybox_pass()
{
	ColourConfig config{};
	config.format = vk::Format::eR32G32B32A32Sfloat;
	config.final_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	config.load_op = vk::AttachmentLoadOp::eClear;
	skybox_pass.init(device_, true, false, config);
}

void Cubemap::create_skybox_pipeline()
{
	std::vector<VkDynamicState> dynamic_states = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR,
	VK_DYNAMIC_STATE_DEPTH_BIAS,
	};

	std::vector<VkPushConstantRange> push_ranges;

	//VkPushConstantRange pushRange{};
	//pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//pushRange.offset = 0;
	//pushRange.size = sizeof(glm::vec4);

	//push_ranges.push_back(pushRange);

	PipelineConfig config{};
	config.vert_shader_path = SHADER("skybox/create_skybox.vert");
	config.frag_shader_path = SHADER("skybox/create_skybox.frag");
	config.dynamic_states = dynamic_states;
	config.pass = skybox_pass.get_api_pass();
	config.subpass_index = 0;
	config.screen_extent = vk::Extent2D(SKYBOX_SIZE, SKYBOX_SIZE); // TODO: hardcoding skybox size, we'll see if it matters.
	//config.push_ranges = push_ranges;
	config.blend_colours = true;
	config.attribute_descriptions = {};
	config.binding_descriptions = {};
	//config.cull_mode = VK_CULL_MODE_FRONT_BIT;
	//config.front_face = VK_FRONT_FACE_CLOCKWISE;

	skybox_pipeline.init(device_, set_pool_, config);
}

void Cubemap::create_cubemap_faces()
{
	br::ImageData data{};
	data.image_info.extent.width = SKYBOX_SIZE;
	data.image_info.extent.height = SKYBOX_SIZE;
	data.image_info.extent.depth = 1.0;

	data.image_info.format = vk::Format::eR32G32B32A32Sfloat;

	data.image_info.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;

	data.image_info.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

	data.image_info.queueFamilyIndexCount = 1;
	data.image_info.pQueueFamilyIndices = &device_->get_graphics_family();
	data.image_view_info.aspect_mask = vk::ImageAspectFlagBits::eColor;

	cubemap_faces.resize(CUBEMAP_FACES);
	for (int i = 0; i < CUBEMAP_FACES; i++)
	{
		data.name = "Skybox face: " + std::to_string(i);
		cubemap_faces[i].init(physical_device_, device_, data);
	}
}

void Cubemap::create_skybox_framebuffers()
{
	skybox_outputs.resize(CUBEMAP_FACES);
	for (int i = 0; i < CUBEMAP_FACES; i++)
	{
		skybox_outputs[i].add_attachment(cubemap_faces[i].get_api_image_view(), v::AttachmentType::COLOR);
		skybox_outputs[i].set_render_pass(skybox_pass.get_api_pass());
		skybox_outputs[i].set_size(SKYBOX_SIZE, SKYBOX_SIZE, 1);
		skybox_outputs[i].build(device_);
	}
}

Cubemap::~Cubemap()
{
	skybox_pipeline.destroy();
	skybox_pass.destroy();

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device_->get(), gpu_sync[i], nullptr);
		vkDestroyFence(device_->get(), cpu_sync[i], nullptr);
	}
}