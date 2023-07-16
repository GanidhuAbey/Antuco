#include <components/mesh_processor.hpp>

#include <antuco.hpp>
#include <logger/interface.hpp>

using namespace tuco;

DEFINE_ID(MeshProcessor);

void MeshProcessor::activate()
{
	Antuco::get_engine().set_updater(this);

	// declare pass.
	// Antuco::get_engine().create_pass(...) 
}

void MeshProcessor::update()
{
	// where draw item would be submitted...
}
