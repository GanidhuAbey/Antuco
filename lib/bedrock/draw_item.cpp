#include <bedrock/draw_item.hpp>

using namespace br;

void DrawItem::set_draw_offsets(uint32_t index_offset, uint32_t vertex_offset)
{
	DrawItem::vertex_offset = vertex_offset;
	DrawItem::index_offset = index_offset;
}

