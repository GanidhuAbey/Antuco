#pragma once

#include <data_structures.hpp>
#include <bedrock/gpu_resource.hpp>

namespace br
{

class MeshDrawData : public GPUResource
{
public:
	MeshDrawData() = default;
	~MeshDrawData() = default;

	virtual void UpdateGPU() override;

private:
	// [TODO 08/24] - technically the projection & view matrices are not draw but rather camera dependent (move to scene descriptor?)
	tuco::UniformBufferObject mvp;
};

}