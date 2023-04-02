#pragma once

#include <memory>
#include <unordered_map>
#include <processor.hpp>

namespace tuco	
{

class ProcessorLookup
{
private:
	std::unique_ptr<ProcessorLookup> mp_instance;
	std::unordered_map<ProcessorId, Processor> m_lookup;
public:
	ProcessorLookup* get() 
	{
		if (!mp_lookup) 
		{
			mp_instance = std::make_unique<ProcessorLookup>(new ProcessorLookup());
		}

		return mp_instance;
	}

	void register_processor();

	Processor* get_processor();

private:
	ProcessorLookup() = default;
	~ProcessorLookup() = default;
};

}