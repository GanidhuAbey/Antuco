#pragma once

#include "vulkan_wrapper/device.hpp"

#include <type_traits>
#include <vector>

namespace v {

enum QueueTypes {
    Compute,
    Render,
    Transfer,
    Present
};

class Queue {
public:
    Queue(PhysicalDevice& device, Surface& surface);
    ~Queue() = default;
private:
    void find_queue_families(
        v::PhysicalDevice& device, v::Surface& surface);
   

    void set_required(std::vector<QueueTypes> types);
    bool found_queues();

    std::optional<uint32_t> render_family;
    std::optional<uint32_t> present_family;
    std::optional<uint32_t> transfer_family;
    std::optional<uint32_t> compute_family;

    bool need_render = false;
    bool need_transfer = false;
    bool need_compute = false;
    bool need_present = false;
};
}
