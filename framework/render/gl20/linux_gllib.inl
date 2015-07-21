#ifdef LINUX_GLLIB_INL
#error linux_gllib.inl should only be included onse
#endif

#define LINUX_GLLIB_INL

#include "framework/logger.h"

#include <dlfcn.h>
#include <functional>

namespace OpenApoc 
{

template<typename T>
std::function<T> fn_ptr_cast(void* ptr)
{
	return reinterpret_cast<T*>(ptr);
}

class GLLib
{
private:
	void *dlHandle;
	std::function<void*(const char*)> GetProcAddress;
public:
	bool loaded;

	GLLib()
		: loaded(false)
	{
		dlHandle = dlopen("libGL.so", RTLD_NOW);
		if (!dlHandle)
		{
			LogWarning("Failed to open libGL.so: \"%s\"", dlerror());
			return;
		}

		void *ptr = dlsym(dlHandle, "glXGetProcAddress");
		const char *err = dlerror();
		if (!ptr || err)
		{
			LogWarning("Failed to open glXGetProcAddress() - got %p: \"%s\"",
				ptr, err);
			return;
		}

		this->GetProcAddress = fn_ptr_cast<void*(const char*)>(ptr);


		loaded = true;
	}

	~GLLib()
	{
		if (dlHandle)
			dlclose(dlHandle);
	}

	template<typename T>
	bool load(const UString &name, std::function<T> &fn)
	{
		UString glName = "gl" + name;
		void *ptr = this->GetProcAddress(glName.str().c_str());
		if (!ptr)
		{
			LogWarning("Failed to load \"%s\": \"%s\"", glName.str().c_str(), dlerror());
			return false;
		}
		fn = fn_ptr_cast<T>(ptr);
		return true;
	}
};

}; //namespace OpenApoc
