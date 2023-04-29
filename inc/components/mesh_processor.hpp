#pragma once

#include <bedrock/processor.hpp>
#include <config.hpp>
#include <model.hpp>
#include <frame_update.hpp>

#include <passes/forward_draw_pass.hpp>

namespace tuco 
{

class MeshProcessor 
	: public br::Processor
	, public effect::UpdateEffect
{
public:

CLASS_ID;

private:
	pass::ForwardDrawPass* m_forward_draw = nullptr;

	std::vector<tuco::Model&> m_models;

public:
	MeshProcessor() = default;
	~MeshProcessor() = default;

	// br::Processor overrides
	void activate() override;

	// efffecT::UpdateEffect overrides
	void update() override;

	void add_model(tuco::Model& m_model) { m_models.push_back(m_model); }
};

}