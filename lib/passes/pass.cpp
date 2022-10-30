#include "passes/pass.hpp"

using namespace pass;

Pass::Pass(v::PhysicalDevice& phys_device, v::Surface& surface)
	: m_queue(phys_device, surface) {
}
