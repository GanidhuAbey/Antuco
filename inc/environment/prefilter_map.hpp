#pragma once

#include <cubemap.hpp>

namespace tuco {

class PrefilterMap : public Cubemap
{
protected:
	void override_pipeline(PipelineConfig& config) override;
public:
	void record_command_buffer(uint32_t face, VkCommandBuffer command_buffer) override;
};

}