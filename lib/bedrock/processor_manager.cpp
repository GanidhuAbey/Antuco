#include <bedrock/processor_manager.hpp>

using namespace br;

DEFINE_ID(ProcessorManager);

std::unordered_map<TypeId, std::unique_ptr<Processor>> ProcessorManager::processors = {};