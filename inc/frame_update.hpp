#pragma once

namespace effect
{

class UpdateEffect
{
private:
	uint32_t update_id;
public:
	UpdateEffect() = default;
	~UpdateEffect() = default;

	virtual void update() = 0;
};

}