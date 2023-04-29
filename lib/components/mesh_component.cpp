#include <components/mesh_component.hpp>
#include <bedrock/processor_manager.hpp>

#include <logger/interface.hpp>

using namespace tuco;

DEFINE_ID(MeshComponent);

MeshComponent::MeshComponent() : m_model()
{
	m_mesh_processor = br::ProcessorManager::get_processor<MeshProcessor>();
}

void MeshComponent::activate()
{
	Antuco::get_engine().set_updater(this);
}

void MeshComponent::set_model_path(std::string path) 
{ 
	m_model_path = path;

	load_model();
}

void MeshComponent::load_model()
{
	m_model.add_mesh(m_model_path);
}

void MeshComponent::update()
{
	LOG("mesh component frame");
}