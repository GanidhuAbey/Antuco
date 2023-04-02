#pragma once

namespace tuco 
{

// Processor class forms the connection between the ECS system and the renderer. Each component has its data set up to be
// optimal with user interactions, but before this data can be sent to the renderer we may need to convert that component data
// into something that the renderer can easily handle, this is where the Processor class comes to play.

// For example: MeshComponent may hold the path to the relevant asset, and may hold some connection to a material, but when we pass
//				it to the the renderer, it needs to be converted into vertices, indices, and texture coordinates. in this case, we 
//				can create a MeshProcessor, that will take the MeshComponent and output a new structure (MeshPacket) more suitable for 
//				rendering.
class Processor 
{
private:
	const static uint32_t id = 0;

public:
	static uint32_t get_id() { return id; }

private:
	Processor() = default;
	~Processor() = default;
};

using ProcessorId = uint32_t;

}