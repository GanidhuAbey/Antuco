#include <vulkan_wrapper/framebuffer.hpp>
#include <logger/interface.hpp>

using namespace v;

Framebuffer::Framebuffer()
{
	frame_buffer_ = nullptr;
}

void Framebuffer::add_attachment(VkImageView attachment, [[maybe_unused]] AttachmentType type)
{
	attachments.push_back(attachment);
}

void Framebuffer::set_render_pass(VkRenderPass render_pass)
{
	render_pass_ = render_pass;
}

void Framebuffer::set_size(uint32_t width, uint32_t height, uint32_t layers)
{
	width_ = width;
	height_ = height;
	layers_ = layers;
}

void Framebuffer::build(std::shared_ptr<v::Device> device)
{
	device_ = device;

	VkFramebufferCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;
	info.renderPass = render_pass_;
	info.attachmentCount = attachments.size();
	info.pAttachments = attachments.data();
	info.width = width_;
	info.height = height_;
	info.layers = layers_;

	ASSERT(vkCreateFramebuffer(device->get(), &info, nullptr, &frame_buffer_) == VK_SUCCESS, "failed to create frame buffer");
}

Framebuffer::~Framebuffer()
{
	if (frame_buffer_)
	{
		vkDestroyFramebuffer(device_->get(), frame_buffer_, nullptr);
	}
}