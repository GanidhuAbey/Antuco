#pragma once

#include <cstdint>
#include <bedrock/resource_group.hpp>

namespace br {
class DrawCall {
public:
    DrawCall() = default;
    ~DrawCall() = default;

    void set_index_offset(uint32_t index_offset) {
        m_index_offset = index_offset;
    }

    void set_vertex_offset(uint32_t vertex_offset) {
        m_vertex_offset = vertex_offset;
    }

    void set_index_count(uint32_t index_count) {
        m_index_count = index_count;
    }

    uint32_t get_index_offset() {
        return m_index_offset;
    }

    uint32_t get_index_count() {
        return m_index_count;
    }

    uint32_t get_vertex_offset() {
        return m_vertex_offset;
    }

    br::ResourceGroup& get_draw_resources() {
        return m_draw_resource;
    }

private:
    uint32_t m_index_offset;
    uint32_t m_index_count;

    uint32_t m_vertex_offset;

    br::ResourceGroup m_draw_resource;

}; 
}