#include <environment.hpp>
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
	p_device = Antuco::get_engine().get_backend()->p_device;

	input_image.init("environment map");
	input_image.load_float_image(file_path, br::ImageFormat::HDR_COLOR, br::ImageType::Image_2D);
	input_image.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	skybox.init(SHADER("skybox/create_skybox.vert"), SHADER("skybox/create_skybox.frag"), model, 1024);
	skybox.set_input(&input_image);

	irradiance_map.init(SHADER("skybox/create_irradiance.vert"), SHADER("skybox/create_irradiance.frag"), model, 32);
	irradiance_map.set_input(&skybox.get_image());

	command_pool_.init(p_device, p_device->get_graphics_family());
	record_command_buffers();

	gpu_sync.resize(MAX_FRAMES_IN_FLIGHT);
	cpu_sync.resize(MAX_FRAMES_IN_FLIGHT);
	render_to_image();
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

		vkCreateSemaphore(p_device->get(), &gpu_sync_info, nullptr, &gpu_sync[i]);
		vkCreateFence(p_device->get(), &cpu_sync_info, nullptr, &cpu_sync[i]);
	}

	//vkResetFences(p_device->get(), MAX_FRAMES_IN_FLIGHT, cpu_sync.data());

	int32_t prev_image = -1;
	int32_t curr_image = (prev_image + 1) % MAX_FRAMES_IN_FLIGHT;
	for (int i = 0; i < CUBEMAP_FACES; i++)
	{

		vkWaitForFences(p_device->get(), 1, &cpu_sync[curr_image], VK_TRUE, UINT64_MAX);
		vkResetFences(p_device->get(), 1, &cpu_sync[curr_image]);

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

		vkQueueSubmit(p_device->get_graphics_queue(), 1, &info, cpu_sync[curr_image]);

		prev_image = curr_image;
		curr_image = (curr_image + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	vkWaitForFences(p_device->get(), MAX_FRAMES_IN_FLIGHT, cpu_sync.data(), VK_TRUE, UINT64_MAX);
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
		
		skybox.record_command_buffer(i, command_buffers[i]);
		irradiance_map.record_command_buffer(i, command_buffers[i]);

		// end command buffer recording
		ASSERT(vkEndCommandBuffer(command_buffers[i]) == VK_SUCCESS, "could not successfully record command buffer");
	}
}

Environment::~Environment()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(p_device->get(), gpu_sync[i], nullptr);
		vkDestroyFence(p_device->get(), cpu_sync[i], nullptr);
	}
}