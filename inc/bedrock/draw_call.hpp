#pragma once

#include <cstdint>
#include <descriptor_set.hpp>

namespace br {
class DrawCall {
public:
    DrawCall() = default;
    ~DrawCall() = default;

    void set_index_offset(uint32_t index_offset) {
        m_index_offset = index_offset;
    }

    void set_vertex_count(uint32_t vertex_count) {
        m_vertex_count = vertex_count;
    }

private:
    uint32_t m_index_offset;
    uint32_t m_vertex_count;

    // TODO: set resource abstraction for descriptor set to handle creation.
    tuco::ResourceCollection resources;
}; 
}