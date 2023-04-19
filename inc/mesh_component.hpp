#pragma once

#include <component.hpp>

namespace tuco 
{

class MeshComponent : public Component 
{
public:
	CLASS_ID;
public:

	std::string m_model_path;
public:
	MeshComponent() = default;
	~MeshComponent() = default;

	void set_model_path(std::string path) { m_model_path = path; }
	std::string& get_model_path() { return m_model_path; }
};

}