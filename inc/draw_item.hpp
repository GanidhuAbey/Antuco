#pragma once

#include <data_structures.hpp>
#include <vector>

namespace tuco {

class DrawItem {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

}
