#include <vulkan_wrapper/descriptor_layout.hpp>

#include <logger/interface.hpp>

using namespace v;

void DescriptorLayout::build(std::shared_ptr<v::Device> device)
{
	p_device = device;

	std::vector<VkDescriptorSetLayoutBinding> binding_data;
	for (auto& binding : bindings)
	{
		binding_data.push_back(binding.info);
	}

	VkDescriptorSetLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;
	info.bindingCount = binding_data.size();
	info.pBindings = binding_data.data();

	vkCreateDescriptorSetLayout(device->get(), &info, nullptr, &layout);

	built = true;
}

DescriptorLayout::~DescriptorLayout()
{
	if (built)
	{
		vkDestroyDescriptorSetLayout(p_device->get(), layout, nullptr);
		layout = VK_NULL_HANDLE;
		built = false;
	}
}