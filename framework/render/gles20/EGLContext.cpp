#include "EGLContext.h"
#include "framework/logger.h"

#include "egl.inl"

namespace OpenApoc
{

//--------------------------------------------------------------------------------
// eGLContext
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------
GLContext::GLContext()
    : display_(egl::NO_DISPLAY), surface_(egl::NO_SURFACE), context_(egl::NO_CONTEXT),
      screen_width_(0), screen_height_(0), es3_supported_(false), egl_context_initialized_(false),
      gles_initialized_(false)
{
	auto eglInitialized = egl::sys::LoadFunctions();
	if (!eglInitialized)
	{
		LogError("Could not initialize EGL, number of failed functions: %d",
		         eglInitialized.GetNumMissing());
	}
	auto glInitialized = gl::sys::LoadFunctions();
	if (!glInitialized)
	{
		LogError("Could not initialize GLES2, number of failed functions: %d",
		         glInitialized.GetNumMissing());
	}
}

void GLContext::InitGLES()
{
	if (gles_initialized_)
		return;
	const char *versionStr = (const char *)gl::GetString(gl::VERSION);
	/*
	if (strstr(versionStr, "OpenGL ES 3.") && gl3stubInit())
	{
	    es3_supported_ = true;
	    gl_version_ = 3.0f;
	}
	else
	{
	    gl_version_ = 2.0f;
	}
	*/
	LogInfo("GL version = %s", versionStr);
	gl_version_ = 2.0f;
	gles_initialized_ = true;
}

//--------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------
GLContext::~GLContext() { Terminate(); }

bool GLContext::Init(EGLNativeWindowType window)
{
	if (egl_context_initialized_)
		return true;

	//
	// Initialize EGL
	//
	window_ = window;
	InitEGLSurface();
	InitEGLContext();
	InitGLES();

	egl_context_initialized_ = true;

	return true;
}

bool GLContext::InitEGLSurface()
{
	display_ = egl::GetDisplay(egl::DEFAULT_DISPLAY);
	egl::Initialize(display_, 0, 0);

	/*
	* Here specify the attributes of the desired configuration.
	* Below, we select an EGLConfig with at least 8 bits per color
	* component compatible with on-screen windows
	*/
	const EGLint attribs[] = {egl::conf::RENDERABLE_TYPE,
	                          egl::conf::OPENGL_ES2_BIT, // Request opengl ES2.0
	                          egl::conf::SURFACE_TYPE,
	                          egl::conf::WINDOW_BIT,
	                          egl::conf::BLUE_SIZE,
	                          8,
	                          egl::conf::GREEN_SIZE,
	                          8,
	                          egl::conf::RED_SIZE,
	                          8,
	                          egl::conf::DEPTH_SIZE,
	                          egl::DONT_CARE,
	                          egl::conf::STENCIL_SIZE,
	                          egl::DONT_CARE,
	                          egl::NONE};
	color_size_ = 8;
	depth_size_ = 16;

	EGLint num_configs;
	egl::ChooseConfig(display_, attribs, &config_, 1, &num_configs);
	/*
	if (!num_configs)
	{
	    //Fall back to 16bit depth buffer
	    const EGLint attribs[] = {
	        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, //Request opengl ES2.0
	        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	        EGL_BLUE_SIZE, 8,
	        EGL_GREEN_SIZE, 8,
	        EGL_RED_SIZE, 8,
	        EGL_DEPTH_SIZE, EGL_DONT_CARE,
	        EGL_STENCIL_SIZE, EGL_DONT_CARE,
	        EGL_NONE };
	    eglChooseConfig(display_, attribs, &config_, 1, &num_configs);
	    depth_size_ = 16;
	}
	*/
	if (!num_configs)
	{
		LogWarning("Unable to retrieve EGL config");
		return false;
	}

	surface_ = egl::CreateWindowSurface(display_, config_, window_, NULL);
	egl::QuerySurface(display_, surface_, egl::WIDTH, &screen_width_);
	egl::QuerySurface(display_, surface_, egl::HEIGHT, &screen_height_);

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	* guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	* As soon as we picked a EGLConfig, we can safely reconfigure the
	* ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	EGLint format;
	egl::GetConfigAttrib(display_, config_, egl::NATIVE_VISUAL_ID, &format);
#ifdef __ANDROID__
	ANativeWindow_setBuffersGeometry(window_, 0, 0, format);
#endif

	return true;
}

bool GLContext::InitEGLContext()
{
	const EGLint context_attribs[] = {egl::CONTEXT_CLIENT_VERSION, 2, // Request opengl ES2.0
	                                  egl::NONE, egl::NONE};
	context_ = egl::CreateContext(display_, config_, NULL, context_attribs);

	if (egl::MakeCurrent(display_, surface_, surface_, context_) == egl::conf::FALSE_)
	{
		LogWarning("Unable to eglMakeCurrent");
		return false;
	}

	context_valid_ = true;
	return true;
}

EGLint GLContext::Swap()
{
	bool b = egl::SwapBuffers(display_, surface_);
	if (!b)
	{
		EGLint err = egl::GetError();
		if (err == egl::BAD_SURFACE)
		{
			// Recreate surface
			InitEGLSurface();
			return egl::SUCCESS; // Still consider glContext is valid
		}
		else if (err == egl::CONTEXT_LOST || err == egl::BAD_CONTEXT)
		{
			// Context has been lost!!
			context_valid_ = false;
			Terminate();
			InitEGLContext();
		}
		return err;
	}
	return egl::SUCCESS;
}

void GLContext::Terminate()
{
	if (display_ != egl::NO_DISPLAY)
	{
		egl::MakeCurrent(display_, egl::NO_SURFACE, egl::NO_SURFACE, egl::NO_CONTEXT);
		if (context_ != egl::NO_CONTEXT)
		{
			egl::DestroyContext(display_, context_);
		}

		if (surface_ != egl::NO_SURFACE)
		{
			egl::DestroySurface(display_, surface_);
		}
		egl::Terminate(display_);
	}

	display_ = egl::NO_DISPLAY;
	context_ = egl::NO_CONTEXT;
	surface_ = egl::NO_SURFACE;
	context_valid_ = false;
}

EGLint GLContext::Resume(EGLNativeWindowType window)
{
	if (egl_context_initialized_ == false)
	{
		Init(window);
		return egl::SUCCESS;
	}

	EGLint original_widhth = screen_width_;
	EGLint original_height = screen_height_;

	// Create surface
	window_ = window;
	surface_ = egl::CreateWindowSurface(display_, config_, window_, NULL);
	egl::QuerySurface(display_, surface_, egl::WIDTH, &screen_width_);
	egl::QuerySurface(display_, surface_, egl::HEIGHT, &screen_height_);

	if (screen_width_ != original_widhth || screen_height_ != original_height)
	{
		// Screen resized
		LogInfo("Screen resized");
	}

	if (egl::MakeCurrent(display_, surface_, surface_, context_) == egl::conf::TRUE_)
		return egl::SUCCESS;

	EGLint err = egl::GetError();
	LogWarning("Unable to eglMakeCurrent [error code: %d]", err);

	if (err == egl::CONTEXT_LOST)
	{
		// Recreate context
		LogWarning("Re-creating egl context");
		InitEGLContext();
	}
	else
	{
		// Recreate surface
		Terminate();
		InitEGLSurface();
		InitEGLContext();
	}

	return err;
}

void GLContext::Suspend()
{
	if (surface_ != egl::NO_SURFACE)
	{
		egl::DestroySurface(display_, surface_);
		surface_ = egl::NO_SURFACE;
	}
}

bool GLContext::Invalidate()
{
	Terminate();

	egl_context_initialized_ = false;
	return true;
}

bool GLContext::CheckExtension(const char *extension)
{
	if (extension == NULL)
		return false;

	std::string extensions = std::string((char *)gl::GetString(gl::EXTENSIONS));
	std::string str = std::string(extension);
	str.append(" ");

	size_t pos = 0;
	if (extensions.find(extension, pos) != std::string::npos)
	{
		return true;
	}

	return false;
}
}