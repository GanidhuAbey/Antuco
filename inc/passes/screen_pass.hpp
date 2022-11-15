#pragma once

#include "frame_builder.hpp"

namespace pass {
	class ScreenPass : public RenderPass {
	private:
		std::vector<DrawItem> drawItems;
	public:
		ScreenPass();

		void initialize() override;
		void frame_begin() override;

		void write_commands(v::CommandBuffer cmd_buff) override;
	};
}