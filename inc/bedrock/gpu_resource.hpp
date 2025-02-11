#pragma once

namespace br
{


// Abstract class used to define a structure of data which uploads information to the GPU.
class GPUResource
{
public:
	GPUResource() = default;
	~GPUResource() = default;
	// [TODO 08/24] - Figure out why we can't have abstract impl of this method.
	virtual void UpdateGPU() {};
};

}