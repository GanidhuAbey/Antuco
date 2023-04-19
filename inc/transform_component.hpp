#pragma once

#include <component.hpp>

namespace tuco 
{

class TransformComponent : public Component 
{
public:
	CLASS_ID;

public:
	TransformComponent() = default;
	~TransformComponent() = default;

};

}