#pragma once

#include <memory_allocator.hpp>

namespace br 
{

class DrawItem
{
private:
	mem::StackBuffer* vertex_buffer;
	mem::StackBuffer* index_buffer;

	uint32_t vertex_offset;
	uint32_t index_offset;

public:
	DrawItem() = default;
	~DrawItem() = default;

	
};

}