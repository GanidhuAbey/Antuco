#pragma once

#include <mesh_component.hpp>
#include <processor.hpp>

#include <passes/forward_draw_pass.hpp>

namespace tuco 
{

class MeshProcessor : public Processor
friend class MeshComponent
{
private:
	pass::ForwardDrawPass* m_draw_pass;

public:
	MeshProcessor() = default;
	~MeshProcessor() = default;
private:
	void load_mesh(const std::string& file_name);
};

}