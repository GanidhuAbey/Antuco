#pragma once

#include <memory_allocator.hpp>
#include <pipeline.hpp>

namespace br
{

class DrawItem
{
private:
	mem::StackBuffer* vertex_buffer;
	mem::StackBuffer* index_buffer;
	
	uint32_t vertex_offset = 0;
	uint32_t index_offset = 0;

	tuco::TucoPipeline* pso;

	int draw_index = -1;
	int material_index = -1;

public:
	DrawItem() = default;
	~DrawItem() = default;

	void set_vertex_buffer(mem::StackBuffer* buffer) { vertex_buffer = buffer; }
	void set_index_buffer(mem::StackBuffer* buffer) { index_buffer = buffer; }

	void set_draw_offsets(uint32_t index_offset, uint32_t vertex_offset);


	// [TODO 08/24] - instead of setting PSO, set shaders and have query to match shader(s) to PSO.
	void set_pso(tuco::TucoPipeline* pso) { DrawItem::pso = pso; }

	void set_draw_index(uint32_t draw_index) { DrawItem::draw_index = draw_index; }
	void set_material_index(uint32_t material_index) { DrawItem::material_index = material_index; }
};

}