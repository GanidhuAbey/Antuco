#include <cubemap.hpp>
#include <api_config.hpp>

#include <vector>
#include <antuco.hpp>
#include <api_graphics.hpp>

#include <vulkan_wrapper/limits.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace tuco;

#define SKYBOX_NAMES {"px.png", "nx.png", "py.png", "ny.png", "pz.png", "nz.png"}
#define CUBEMAP_FACES 6
#define SKYBOX_SIZE 1024

void Environment::init(std::string file_path, GameObject* model)
{
	ubo_buffer_offsets.clear();
	cubemap_model = model;

	device_ = Antuco::get_engine().get_backend()->p_device;
	physical_device_ = Antuco::get_engine().get_backend()->p_physical_device;
	set_pool_ = Antuco::get_engine().get_backend()->get_set_pool();

	// create input hdr image
	hdr_image.init("environment map");
	hdr_image.load_float_image(file_path, br::ImageFormat::HDR_COLOR, br::ImageType::Image_2D);
	hdr_image.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	// create render pass
	create_skybox_pass();

	// create pipeline to generate skybox.
	create_skybox_pipeline();

	// create 6 output images 
	create_cubemap_faces();

	// create frame buffer
	create_skybox_framebuffers();

	// update descriptors
	write_descriptors();

	// record command buffers
	command_pool_.init(device_, device_->get_graphics_family());
	record_command_buffers();

	// create render frame
	gpu_sync.resize(MAX_FRAMES_IN_FLIGHT);
	cpu_sync.resize(MAX_FRAMES_IN_FLIGHT);
	render_to_image();

	// render 6 times to each image

	// collect into final cubemap image
}

void Environment::write_descriptors()
{
	ResourceCollection* collection = skybox_pipeline.get_resource_collection(0);
	ubo_buffer_offsets.resize(CUBEMAP_FACES);

	ImageDescription image_info{};
	image_info.binding = 1;
	image_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	image_info.sampler = hdr_image.get_sampler();
	image_info.image = hdr_image.get_api_image();
	image_info.image_view = hdr_image.get_api_image_view();
	image_info.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	mem::SearchBuffer& buffer = Antuco::get_engine().get_backend()->get_model_buffer();

	// create descriptor set
	collection->addSets(CUBEMAP_FACES, *set_pool_);

	for (int i = 0; i < CUBEMAP_FACES; i++)
	{
		//hdr_image.transfer(vk::ImageLayout::eShaderReadOnlyOptimal, device_->get_graphics_queue());
		ubo_buffer_offsets[i] = buffer.allocate(sizeof(UniformBufferObject), v::Limits::get().uniformBufferOffsetAlignment);

		BufferDescription b_info{};
		b_info.binding = 0;
		b_info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		b_info.buffer = buffer.buffer;
		b_info.bufferRange = sizeof(UniformBufferObject);
		b_info.bufferOffset = ubo_buffer_offsets[i];

		collection->addBuffer(b_info, i);
		collection->addImage(image_info, i);

		collection->updateSet(i);
	}

	// update UBO data.
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};


	for (int i = 0; i < CUBEMAP_FACES; i++)
	{
		UniformBufferObject ubo{};
		//ubo.modelToWorld = glm::mat4(glm::vec4(5.f, 0.f, 0.f, 0.f), glm::vec4(0.f, 5.f, 0.f, 0.f), glm::vec4(0.f, 0.f, 5.f, 0.f), glm::vec4(0.f, 0.f, 0.f, 1.f));
		ubo.worldToCamera = captureViews[i];
		ubo.projection = captureProjection;

		buffer.writeLocal(device_->get(), ubo_buffer_offsets[i], sizeof(UniformBufferObject), &ubo);
	}

	//Antuco::get_engine().get_backend()->write_screen_set();
}

void Environment::render_to_image()
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

void Environment::record_command_buffers()
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
		render_area.extent.height = SKYBOX_SIZE;
		render_area.extent.width = SKYBOX_SIZE;

		VkRenderPassBeginInfo skybox_info{};
		skybox_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		skybox_info.framebuffer = skybox_outputs[i].get_api_buffer();
		skybox_info.renderArea = render_area;
		skybox_info.renderPass = skybox_pass.get_api_pass();
		skybox_info.clearValueCount = clear_values.size();
		skybox_info.pClearValues = clear_values.data();

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
		vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &v_buffer, offsets);

		vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline.get_api_pipeline());

		ResourceCollection* collection = skybox_pipeline.get_resource_collection(0);
		VkDescriptorSet set = collection->get_api_set(i);

		//vkCmdDraw(command_buffers[i], 3, 1, 0, 0);

		std::vector<Primitive>& prims = cubemap_model->object_model.get_prims();
		for (int j = 0; j < prims.size(); j++)
		{
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline.get_api_layout(), 0, 1, &set, 0, nullptr);

			vkCmdDrawIndexed(command_buffers[i], prims[j].index_count, 1, prims[j].index_start + cubemap_model->buffer_index_offset, cubemap_model->buffer_vertex_offset, 0);
		}

		// end render pass
		vkCmdEndRenderPass(command_buffers[i]);

		//Antuco::get_engine().get_backend()->render_to_screen(0, command_buffers[i]);

		// end command buffer recording
		ASSERT(vkEndCommandBuffer(command_buffers[i]) == VK_SUCCESS, "could not successfully record command buffer");
	}
}

void Environment::create_skybox_pass()
{
	ColourConfig config{};
	config.format = vk::Format::eR32G32B32A32Sfloat;
	config.final_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	config.load_op = vk::AttachmentLoadOp::eClear;
	skybox_pass.init(device_, true, false, config);
}

void Environment::create_skybox_pipeline()
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
	config.depth_test_enable = VK_FALSE;
	config.screen_extent = vk::Extent2D(SKYBOX_SIZE, SKYBOX_SIZE); // TODO: hardcoding skybox size, we'll see if it matters.
	//config.push_ranges = push_ranges;
	config.blend_colours = true;
	//config.attribute_descriptions = {};
	//config.binding_descriptions = {};
	config.cull_mode = VK_CULL_MODE_FRONT_BIT;
	config.front_face = VK_FRONT_FACE_CLOCKWISE;

	skybox_pipeline.init(device_, set_pool_, config);
}

void Environment::create_cubemap_faces()
{	
	br::ImageDetails info{};
	info.format = br::ImageFormat::HDR_COLOR;
	info.type = br::ImageType::Cube;
	info.usage = br::ImageUsage::RENDER_OUTPUT;

	cubemap.init("Skybox");

	cubemap.load_blank(info, SKYBOX_SIZE, SKYBOX_SIZE, 6);
	cubemap.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	cubemap.create_view(6, 0, br::ImageType::Cube);

	for (int i = 0; i < CUBEMAP_FACES; i++)
	{
		cubemap.create_view(1, i, br::ImageType::Image_2D);
	}
}

void Environment::create_skybox_framebuffers()
{
	skybox_outputs.resize(CUBEMAP_FACES);
	for (int i = 0; i < CUBEMAP_FACES; i++)
	{
		skybox_outputs[i].add_attachment(cubemap.get_api_image_view(i + 1), v::AttachmentType::COLOR);
		skybox_outputs[i].set_render_pass(skybox_pass.get_api_pass());
		skybox_outputs[i].set_size(SKYBOX_SIZE, SKYBOX_SIZE, 1);
		skybox_outputs[i].build(device_);
	}
}

Environment::~Environment()
{
	skybox_pipeline.destroy();
	skybox_pass.destroy();

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device_->get(), gpu_sync[i], nullptr);
		vkDestroyFence(device_->get(), cpu_sync[i], nullptr);
	}
}