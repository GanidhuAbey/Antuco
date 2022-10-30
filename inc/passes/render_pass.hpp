#pragma once

#include <passes/pass.hpp>

#include <optional>

namespace pass {

class RenderPass : Pass {
public:
    std::optional<mem::Image> colour_image;
    std::optional<mem::Image> depth_image; 

    //store
public:
    RenderPass(std::vector<mem::Image> targets);
};

}
