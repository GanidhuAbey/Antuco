#include <lut.hpp>
#include <api_config.hpp>

#include <vector>
#include <antuco.hpp>
#include <api_graphics.hpp>

#include <vulkan_wrapper/limits.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace tuco;

#define CUBEMAP_FACES 6

void LUT::init(std::string name, std::string& vert, std::string& frag, uint32_t size, uint32_t mip_count, br::ImageFormat format)
{
	LUT::name = name;

	input_image = nullptr;

	LUT::format = format;

	ubo_buffer_offsets.clear();
	map_size = size;
	LUT::mip_count = mip_count;

	device_ = Antuco::get_engine().get_backend()->p_device;
	physical_device_ = Antuco::get_engine().get_backend()->p_physical_device;
	set_pool_ = Antuco::get_engine().get_backend()->get_set_pool();

	// create render pass
	create_pass();

	// create pipeline to generate skybox.
	create_pipeline(vert, frag);

	// create image and views
	create_image();

	// create frame buffer
	create_framebuffers();
}
//
//void LUT::set_input(br::Image* image)
//{
//	input_image = image;
//	// update descriptors
//	write_descriptors();
//}

void LUT::write_descriptors()
{
	//ResourceCollection* collection = pipeline.get_resource_collection(0);
	//ubo_buffer_offsets.resize(CUBEMAP_FACES);

	//ImageDescription image_info{};
	//image_info.binding = 1;
	//image_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//image_info.sampler = input_image->get_sampler();
	//image_info.image = input_image->get_api_image();
	//image_info.image_view = input_image->get_api_image_view();
	//image_info.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	//mem::SearchBuffer& buffer = Antuco::get_engine().get_backend()->get_model_buffer();

	//// create descriptor set
	//collection->addSets(CUBEMAP_FACES, *set_pool_);

	//for (int i = 0; i < CUBEMAP_FACES; i++)
	//{
	//	//hdr_image.transfer(vk::ImageLayout::eShaderReadOnlyOptimal, device_->get_graphics_queue());
	//	ubo_buffer_offsets[i] = buffer.allocate(sizeof(UniformBufferObject), v::Limits::get().uniformBufferOffsetAlignment);

	//	BufferDescription b_info{};
	//	b_info.binding = 0;
	//	b_info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//	b_info.buffer = buffer.buffer;
	//	b_info.bufferRange = sizeof(UniformBufferObject);
	//	b_info.bufferOffset = ubo_buffer_offsets[i];

	//	collection->addBuffer(b_info, i);
	//	collection->addImage(image_info, i);

	//	collection->updateSet(i);
	//}

	//// update UBO data.
	//glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	//glm::mat4 captureViews[] =
	//{
	//   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	//   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	//   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	//   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	//   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	//   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	//};


	//for (int i = 0; i < CUBEMAP_FACES; i++)
	//{
	//	UniformBufferObject ubo{};
	//	//ubo.modelToWorld = glm::mat4(glm::vec4(5.f, 0.f, 0.f, 0.f), glm::vec4(0.f, 5.f, 0.f, 0.f), glm::vec4(0.f, 0.f, 5.f, 0.f), glm::vec4(0.f, 0.f, 0.f, 1.f));
	//	ubo.worldToCamera = captureViews[i];
	//	ubo.projection = captureProjection;

	//	buffer.writeLocal(device_->get(), ubo_buffer_offsets[i], sizeof(UniformBufferObject), &ubo);
	//}

	//Antuco::get_engine().get_backend()->write_screen_set();
}

void LUT::record_command_buffer(uint32_t face, VkCommandBuffer command_buffer)
{
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
	render_area.extent.height = map_size;
	render_area.extent.width = map_size;

	VkRenderPassBeginInfo skybox_info{};
	skybox_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	skybox_info.framebuffer = output.get_api_buffer();
	skybox_info.renderArea = render_area;
	skybox_info.renderPass = pass.get_api_pass();
	skybox_info.clearValueCount = clear_values.size();
	skybox_info.pClearValues = clear_values.data();

	if (input_image) {
		input_image->change_layout(vk::ImageLayout::eShaderReadOnlyOptimal, device_->get_graphics_queue());
	}

	vkCmdBeginRenderPass(command_buffer, &skybox_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport newViewport{};
	newViewport.x = 0;
	newViewport.y = 0;
	newViewport.width = map_size;
	newViewport.height = map_size;
	newViewport.minDepth = 0.0;
	newViewport.maxDepth = 1.0;

	vkCmdSetViewport(command_buffer, 0, 1, &newViewport);

	VkRect2D scissor{};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.height = map_size;
	scissor.extent.width = map_size;

	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	//vkCmdBindVertexBuffers(command_buffer, 0, 1, &v_buffer, offsets);

	//vkCmdBindIndexBuffer(command_buffer, index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_api_pipeline());

	//ResourceCollection* collection = pipeline.get_resource_collection(0);
	//VkDescriptorSet set = collection->get_api_set(face);

	vkCmdDraw(command_buffer, 3, 1, 0, 0);

	// end render pass
	vkCmdEndRenderPass(command_buffer);
}

void LUT::create_pass()
{
	ColourConfig config{};
	config.format = br::Image::get_vk_format(format, nullptr, nullptr);
	config.final_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	config.load_op = vk::AttachmentLoadOp::eClear;
	pass.init(device_, true, false, config);
}

void LUT::create_pipeline(std::string& vert, std::string& frag)
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
	config.vert_shader_path = vert;
	config.frag_shader_path = frag;
	config.dynamic_states = dynamic_states;
	config.pass = pass.get_api_pass();
	config.subpass_index = 0;
	config.depth_test_enable = VK_FALSE;
	config.screen_extent = vk::Extent2D(map_size, map_size); // TODO: hardcoding skybox size, we'll see if it matters.
	//config.push_ranges = push_ranges;
	config.blend_colours = true;
	config.attribute_descriptions = {};
	config.binding_descriptions = {};
	config.cull_mode = VK_CULL_MODE_FRONT_BIT;
	config.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	override_pipeline(config);

	pipeline.init(device_, set_pool_, config);
}

void LUT::create_image()
{
	br::ImageDetails info{};
	info.format = format;
	info.type = br::ImageType::Image_2D;
	info.usage = br::ImageUsage::RENDER_OUTPUT;

	lut.init(name);

	lut.load_blank(info, map_size, map_size, 1, mip_count);
	lut.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	lut.create_view(1, 0, 0, br::ImageType::Image_2D);

}

void LUT::create_framebuffers()
{
	output.add_attachment(lut.get_api_image_view(), v::AttachmentType::COLOR);
	output.set_render_pass(pass.get_api_pass());
	output.set_size(map_size, map_size, 1);
	output.build(device_);

	//outputs.resize(CUBEMAP_FACES * mip_count);
	//for (int face = 0; face < CUBEMAP_FACES; face++)
	//{
	//	for (int mip = 0; mip < mip_count; mip++)
	//	{
	//		uint32_t mip_size = map_size * std::pow(0.5, mip);
	//		outputs[INDEX(face, mip, mip_count)].add_attachment(cubemap.get_api_image_view(INDEX(face, mip, mip_count) + 1), v::AttachmentType::COLOR);
	//		outputs[INDEX(face, mip, mip_count)].set_render_pass(pass.get_api_pass());
	//		outputs[INDEX(face, mip, mip_count)].set_size(mip_size, mip_size, 1);
	//		outputs[INDEX(face, mip, mip_count)].build(device_);
	//	}
	//}
}

LUT::~LUT()
{
	pipeline.destroy();
	pass.destroy();
}