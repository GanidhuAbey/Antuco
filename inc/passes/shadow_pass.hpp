#pragma once

#include <passes/render_pass.hpp>

namespace pass {

class ShadowPass : public RenderPass {
private:
	const static uint32_t s_shadow_pass_size = 2048;
public:
	ShadowPass();

	void initialize() override;
};

}