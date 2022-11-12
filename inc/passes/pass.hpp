#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

#include <vulkan_wrapper/physical_device.hpp>
#include <vulkan_wrapper/queue.hpp>
#include <vulkan_wrapper/surface.hpp>

#include <descriptor_set.hpp>

#include <pipeline.hpp>

namespace pass {

class Pass {
protected:
	
	tuco::TucoPipeline pipeline;

    tuco::ResourceCollection resources;

public:
	Pass() = default;
	~Pass() = default;

	virtual void initialize();

	virtual void frame_begin();
	virtual void frame_end();
	
	virtual void build_commmand_list();

};

}
