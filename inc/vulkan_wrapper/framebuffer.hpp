#pragma once

#include <vulkan_wrapper/device.hpp>

#include <vulkan/vulkan.h>
#include <vector>

namespace v {

enum class AttachmentType
{
	COLOR,
	DEPTH,
	DEPTH_STENCIL
};

class Framebuffer
{
private:
	VkFramebuffer frame_buffer_;
	VkRenderPass render_pass_;

	std::vector<VkImageView> attachments;

	std::shared_ptr<v::Device> device_;

	uint32_t width_;
	uint32_t height_;
	uint32_t layers_;

public:
	Framebuffer();
	~Framebuffer();

	// specify the image attachment that the renderpass is rendering to.
	void add_attachment(VkImageView attachment, AttachmentType type);
	void set_render_pass(VkRenderPass render_pass);
	void set_size(uint32_t width, uint32_t height, uint32_t layers);

	void build(std::shared_ptr<v::Device> device);

	VkFramebuffer& get_api_buffer() { return frame_buffer_; }
};

}