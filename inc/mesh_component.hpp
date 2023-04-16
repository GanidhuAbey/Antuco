#pragma once

#include <component.hpp>

namespace tuco 
{

class MeshComponent : public Component 
{
private:
	static const uint32_t id = 3; // need method of generating id's easily...

	std::string m_model_path;
public:
	void set_model_path(std::string&& path) { m_model_path = path; }
	std::string& get_model_path() { return m_model_path; }
};

}