#pragma once

#include <unordered_map>
#include <memory>

#include <bedrock/processor.hpp>
#include <components/mesh_processor.hpp>

#include <config.hpp>

namespace br
{

class ProcessorManager
{
public:
	CLASS_ID;
private:
	static std::unordered_map<TypeId, std::unique_ptr<Processor>> processors;
public:
	template <typename T>
	static void add_processor() 
	{
		T processor;
		add_to_processor(std::make_unique<T>(processor), T::ID);
	}

	template <typename T>
	static T* get_processor()
	{
		return static_cast<T*>(get_processor(T::ID));
	}

private:
	static void add_to_processor(std::unique_ptr<Processor> processor, TypeId processor_id)
	{
		processors[processor_id] = std::move(processor);
	}

	static Processor* get_processor(TypeId processor_id)
	{
		auto search = processors.find(processor_id);
		if (search != processors.end())
		{
			return search->second.get();
		}
		return nullptr;
	}
};

}