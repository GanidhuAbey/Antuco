#include <vulkan_wrapper/binding_layout.hpp>

using namespace v;

void BindingLayouts::initialize_layouts(std::vector<DescriptorSetLayoutData>& info) {
    m_layouts.resize(info.size());  
    for (uint32_t i = 0; i < m_layouts.size(); i++) {
        vkCreateDescriptorSetLayout(
            m_device.get(), 
            &info[i].create_info, 
            nullptr, 
            &m_layouts[i]
        );
    }
}

VkDescriptorSetLayout& BindingLayouts::get_layout(uint32_t index) {
    assert(index < m_layouts.size());

    return m_layouts[index];
}
