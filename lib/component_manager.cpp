#include <component_manager.hpp>

using namespace tuco;

std::unordered_map<uint32_t, std::vector<std::unique_ptr<Component>>> ComponentManager::components = {};