#include <bedrock/resource_group.hpp>

using namespace br;

void ResourceGroup::build(const v::Device& device, mem::Pool& pool) {
    ResourceGroup::m_device = const_cast<v::Device*>(&device);

    if (m_frequency == ResourceFrequency::Invalid) {
        ERR_V_MSG("The resource frequency was not specified for ResourceGroup.");
        return;
    }

    if (m_shader_bindings.size() == 0) {
        ERR_V_MSG("No resource bound to ResourceGroup, nothing to do.");
        return;
    }

    create_descriptor_set_layout();
    create_set(pool);
}

void ResourceGroup::destroy() {
    
}

void ResourceGroup::create_descriptor_set_layout() {
    std::vector<vk::DescriptorSetLayoutBinding> resources(m_shader_bindings.size());
    for (auto i = 0; i < m_shader_bindings.size(); i++) {
        auto resource = vk::DescriptorSetLayoutBinding(
            m_shader_bindings[i].bind_slot,
            static_cast<vk::DescriptorType>(m_shader_bindings[i].resource_type),
            static_cast<vk::ShaderStageFlags>(m_shader_bindings[i].shader_type),
            nullptr // immutable sampler
        );

        resources[i] = resource;
    }

    auto layout_info = vk::DescriptorSetLayoutCreateInfo(
        {},
        resources
    );

    m_layout = m_device->get().createDescriptorSetLayoutUnique(layout_info);
}

void ResourceGroup::add_resource(ShaderBinding resource) {
    if (resource.image_info && resource.buffer_info) {
        ERR_V_MSG("Both an image and a buffer cannot be added to the same ShaderBinding.");
        return;
    }

    m_shader_bindings.push_back(resource);
}

void ResourceGroup::create_set(mem::Pool& pool) {
    m_pool_index = pool.allocate(1);

    auto alloc_info = vk::DescriptorSetAllocateInfo(
        pool.pools[m_pool_index],
        m_layout.get()
    );

    m_set = m_device->get().allocateDescriptorSets(alloc_info).front();
}

void ResourceGroup::update_set() {
    std::vector<vk::WriteDescriptorSet> writes;
    for (auto i = 0; i < m_shader_bindings.size(); i++) {
        auto write = vk::WriteDescriptorSet(
            m_set,
            m_shader_bindings[i].bind_slot,
            static_cast<uint32_t>(0), // Assume no updating part of a resource.
            static_cast<uint32_t>(1),
            static_cast<vk::DescriptorType>(m_shader_bindings[i].resource_type),
            m_shader_bindings[i].image_info,
            m_shader_bindings[i].buffer_info
        );
        
        writes.push_back(write);
    }

    m_device->get().updateDescriptorSets(writes, 0);
}

void ResourceGroup::release_bindings() {
    m_shader_bindings.clear();
}