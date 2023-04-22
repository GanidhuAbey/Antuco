#pragma once

#include <config.hpp>

namespace br 
{

class Processor 
{
public:
CLASS_ID;

private:
	TypeId runtime_id;

public:
	TypeId get_id() { return runtime_id; }
	void set_id(TypeId id) { runtime_id = id; }

public:
	Processor() = default;
	~Processor() = default;
};

}