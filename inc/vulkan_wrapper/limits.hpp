#pragma once

#include <cstdint>

#define POOL_MAX_SET_COUNT 1000

namespace v {

class Limits
{
public:
	uint32_t uniformBufferOffsetAlignment = 0;

public:
	static Limits& get() {
		return m_instance;
	}
	Limits(const Limits&) = delete;
private:
	Limits();
	~Limits();

	static Limits m_instance;
	
};

}