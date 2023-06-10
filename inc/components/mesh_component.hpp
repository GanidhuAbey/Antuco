#pragma once

#include <component.hpp>
#include <model.hpp>
#include <frame_update.hpp>

namespace tuco 
{

// Forward declare mesh processor
class MeshProcessor;

class MeshComponent 
	: public Component
	, public effect::UpdateEffect
{
public:
	CLASS_ID;
public:
	Model m_model;
	std::string m_model_path;

public:
	MeshComponent();
	~MeshComponent() = default;

	void set_model_path(std::string path);
	std::string& get_model_path() { return m_model_path; }

	void load_model();

	// effect::UpdateEffect::updater()
	void update() override;

	// Component::activate()
	void activate() override;

private:
	MeshProcessor* m_mesh_processor = nullptr;
};

}
