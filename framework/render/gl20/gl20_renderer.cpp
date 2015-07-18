#include "framework/renderer_interface.h"
#include "framework/render/gl20/gl20.h"

namespace
{

class OGL20RendererFactory : public OpenApoc::RendererFactory
{
bool alreadyInitialised;
bool functionLoadSuccess;
public:
	OGL20RendererFactory()
		: alreadyInitialised(false), functionLoadSuccess(false){}
	virtual OpenApoc::Renderer *create()
	{
		if (!alreadyInitialised)
		{
			alreadyInitialised = true;
			OpenApoc::GL20 gl20;
			if (!gl20.loadedSuccessfully)
				return nullptr;
		}
		return nullptr;
	}
};

OpenApoc::RendererRegister<OGL20RendererFactory> register_at_load_gl_2_0_renderer("GL_2_0");

}; //anonymous namespace
