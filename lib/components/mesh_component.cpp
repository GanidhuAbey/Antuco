#include <components/mesh_component.hpp>
#include <bedrock/processor_manager.hpp>

using namespace tuco;

DEFINE_ID(MeshComponent);

MeshComponent::MeshComponent()
{
	m_mesh_processor = br::ProcessorManager::get_processor<MeshProcessor>();
}