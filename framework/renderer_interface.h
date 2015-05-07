#pragma once
#include "renderer.h"
#include <string>

namespace OpenApoc {

class RendererFactory
{
public:
	virtual Renderer *create() = 0;
	virtual ~RendererFactory() {};
};

void registerRenderer(RendererFactory *factory, std::string name);

template <typename T>
class RendererRegister
{
public:
	RendererRegister(std::string name)
	{
		registerRenderer(new T, name);
	}
};

}; //namespace OpenApoc
