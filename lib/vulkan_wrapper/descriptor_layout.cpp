#include <vulkan_wrapper/descriptor_layout.hpp>

using namespace v;

void DescriptorLayout::build(std::shared_ptr<v::Device> device)
{
	p_device = device;

	VkDescriptorSetLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;
	info.bindingCount = bindings.size();
	info.pBindings = bindings.data();

	vkCreateDescriptorSetLayout(device->get(), &info, nullptr, &layout);
}