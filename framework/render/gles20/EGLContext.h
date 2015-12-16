#ifndef OPENAPOC_GLCONTEXT_H_
#define OPENAPOC_GLCONTEXT_H_

#include "egl.hpp"
#include "gles_2_0.hpp"

#if defined(__ANDROID__)

#elif defined(_WIN32)

#endif

namespace OpenApoc
{

//--------------------------------------------------------------------------------
// Class
//--------------------------------------------------------------------------------

/******************************************************************
* OpenGL context handler
* The class handles OpenGL and EGL context based on Android activity life cycle
* The caller needs to call corresponding methods for each activity life cycle events as it's done in
* sample codes.
*
* Also the class initializes OpenGL ES3 when the compatible driver is installed in the device.
* getGLVersion() returns 3.0~ when the device supports OpenGLES3.0
*
* Thread safety: OpenGL context is expecting used within dedicated single thread,
* thus GLContext class is not designed as a thread-safe
*/
class GLContext
{
  private:
	// EGL configurations
	egl::NativeWindowType window_;
	egl::Display display_;
	egl::Surface surface_;
	egl::Context context_;
	egl::Config config_;

	// Screen parameters
	EGLint screen_width_;
	EGLint screen_height_;
	EGLint color_size_;
	EGLint depth_size_;

	// Flags
	bool gles_initialized_;
	bool egl_context_initialized_;
	bool es3_supported_;
	float gl_version_;
	bool context_valid_;

	void InitGLES();
	void Terminate();
	bool InitEGLSurface();
	bool InitEGLContext();

	GLContext(GLContext const &);
	void operator=(GLContext const &);
	GLContext();
	virtual ~GLContext();

  public:
	static GLContext *GetInstance()
	{
		// Singleton
		static GLContext instance;

		return &instance;
	}

	bool Init(NativeWindowType window);
	EGLint Swap();
	bool Invalidate();

	void Suspend();
	EGLint Resume(NativeWindowType window);

	EGLint GetScreenWidth() { return screen_width_; }
	EGLint GetScreenHeight() { return screen_height_; }

	EGLint GetBufferColorSize() { return color_size_; }
	EGLint GetBufferDepthSize() { return depth_size_; }
	float GetGLVersion() { return gl_version_; }
	bool CheckExtension(const char *extension);
};
}
#endif /* OPENAPOC_GLCONTEXT_H_ */