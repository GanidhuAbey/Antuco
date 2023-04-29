#include <components/mesh_processor.hpp>

#include <antuco.hpp>
#include <logger/interface.hpp>

using namespace tuco;

DEFINE_ID(MeshProcessor);

void MeshProcessor::activate()
{
	Antuco::get_engine().set_updater(this);
}

void MeshProcessor::update()
{
	LOG("render mesh...");
}