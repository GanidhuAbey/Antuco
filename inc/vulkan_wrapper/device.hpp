#pragma once

#include <vulkan/vulkan.hpp>

namespace v {
class Device {
public:
    Device();
    ~Device();

private:
    vk::Device device;
    void create_logical_device();

};
}
