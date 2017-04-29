/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "piglit-util-gl.h"
#include "piglit-glx-util.h"

/*static Display *dpy;
static GLXFBConfig *fbconfig;
static XVisualInfo *visinfo;
static Window win;
static GLXWindow glxWin;
static GLXContext ctx;
static GLuint prog1, prog2, vao;*/

static Display *dpy;
static Window x_windows[2];
static GLXWindow glx_windows[2];
static GLXContext glx_contexts[2];
static GLuint prog1[2], prog2[2], vao[2], fbo, tex_color, tex_depth;

static int visual_attributes[] =
 {
	GLX_X_RENDERABLE    , True,
	GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
	GLX_RENDER_TYPE     , GLX_RGBA_BIT,
	GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
	GLX_RED_SIZE        , 8,
	GLX_GREEN_SIZE      , 8,
	GLX_BLUE_SIZE       , 8,
	GLX_ALPHA_SIZE      , 8,
	GLX_DEPTH_SIZE      , 24,
	GLX_STENCIL_SIZE    , 8,
	GLX_DOUBLEBUFFER    , True,
	None
 };

static const char *vs_text =
	"#version 430\n"
	"in vec4 pos_in;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = pos_in;\n"
	"}\n";

static const char *fs_text1 =
	"#version 430\n"
	"out vec4 color;\n"
	"void main()\n"
	"{\n"
	"  gl_FragDepth = 0.5f;\n"
	"	color = vec4(0.0, 1.0, 0.0, 1.0);\n"
	"}\n";

static const char *fs_text2 =
	"#version 430\n"
	"out vec4 color;\n"
	"void main()\n"
	"{\n"
	"	color = vec4(1.0, 0.0, 0.0, 1.0);\n"
	"}\n";

static const char *fs_text3 =
	"#version 430\n"
	"out vec4 color;\n"
	"void main()\n"
	"{\n"
	"	int i;\n"
	"	for (i = 0; i < 10; i++) {\n"
	"		color = vec4(1.0, 0.0, 0.0, 1.0);\n"
	"		i = 5;\n"
	"	}\n"
	"}\n";

const int context_attributes[] = {
	GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
	GLX_CONTEXT_MINOR_VERSION_ARB, 0,
	GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB, GLX_LOSE_CONTEXT_ON_RESET_ARB,
	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_RESET_ISOLATION_BIT_ARB,
	None
};

PFNGLXCREATECONTEXTATTRIBSARBPROC __piglit_glXCreateContextAttribsARB;
#define glXCreateContextAttribsARB(dpy, config, share, direct, attrib)     \
	(*__piglit_glXCreateContextAttribsARB)(dpy, config, share, direct, \
					       attrib)

static Window
make_x_window(Display *dpy, XVisualInfo *visinfo)
{
	XSetWindowAttributes window_attr;
	unsigned long mask;
	int screen = DefaultScreen(dpy);
	Window root_win = RootWindow(dpy, screen);
	Window win;

	window_attr.background_pixel = 0;
	window_attr.border_pixel = 0;
	window_attr.colormap = XCreateColormap(dpy, root_win,
					       visinfo->visual, AllocNone);
	window_attr.event_mask = StructureNotifyMask | ExposureMask |
		KeyPressMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	piglit_width = 1080;
	piglit_height = 1080;

	win = XCreateWindow(dpy, root_win, 0, 0,
			    piglit_width, piglit_height,
			    0, visinfo->depth, InputOutput,
			    visinfo->visual, mask, &window_attr);

	if (piglit_automatic)
		piglit_glx_window_set_no_input(dpy, win);

	XMapWindow(dpy, win);

	return win;
}

static int
glx_init()
{
	GLXFBConfig *fbconfig;
	XVisualInfo *visinfo[2];
	int fb_count = 2;
	dpy = piglit_get_glx_display();

	piglit_require_glx_version(dpy, 1, 4);
	piglit_require_glx_extension(dpy, "GLX_ARB_create_context");
	piglit_require_glx_extension(dpy, "GLX_ARB_create_context_robustness");
	piglit_require_glx_extension(dpy, "GLX_ARB_robustness_application_isolation");
	piglit_require_glx_extension(dpy, "GLX_ARB_robustness_share_group_isolation");

	__piglit_glXCreateContextAttribsARB =
		(PFNGLXCREATECONTEXTATTRIBSARBPROC)
		glXGetProcAddress((const GLubyte *)
				  "glXCreateContextAttribsARB");
	assert(__piglit_glXCreateContextAttribsARB != NULL);

	fbconfig = glXChooseFBConfig(dpy, DefaultScreen(dpy), visual_attributes,
		&fb_count);
	visinfo[0] = glXGetVisualFromFBConfig(dpy, fbconfig[0]);
	visinfo[1] = glXGetVisualFromFBConfig(dpy, fbconfig[1]);

	x_windows[0] = make_x_window(dpy, visinfo[0]);
	glx_windows[0] = glXCreateWindow(dpy, fbconfig[0], x_windows[0], NULL);

	glx_contexts[0] = glXCreateContextAttribsARB(dpy, fbconfig[0], NULL, True,
		context_attributes);

	x_windows[1] = make_x_window(dpy, visinfo[1]);
	glx_windows[1] = glXCreateWindow(dpy, fbconfig[1], x_windows[1], NULL);

	glx_contexts[1] = glXCreateContextAttribsARB(dpy, fbconfig[1], NULL, True,
		context_attributes);

	glXMakeContextCurrent(dpy, glx_windows[0], glx_windows[0], glx_contexts[0]);
	glXMakeContextCurrent(dpy, glx_windows[1], glx_windows[1], glx_contexts[1]);
	piglit_dispatch_default_init(PIGLIT_DISPATCH_GL);

	return 1;
}

static GLuint
make_fbo(void)
{
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo );
	glBindTexture(GL_TEXTURE_2D, tex_color);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		tex_color, 0);
	glBindTexture(GL_TEXTURE_2D, tex_depth);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D, tex_depth, 0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	return fbo;
}

static GLuint
make_texture_color(void)
{
	GLuint tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, piglit_width, piglit_height, 0,
		GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

static GLuint
make_texture_depth(void)
{
	GLuint tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, piglit_width,
		piglit_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

static GLuint
make_shader_program1(void)
{
	GLuint prog;

	prog = piglit_build_simple_program(vs_text, fs_text1);
	glUseProgram(prog);

	glBindAttribLocation(prog, 0, "pos_in");

	glLinkProgram(prog);

	if (!piglit_check_gl_error(GL_NO_ERROR)) {
		piglit_report_result(PIGLIT_FAIL);
	}

	return prog;
}

static GLuint
make_shader_program2(void)
{
	GLuint prog;

	prog = piglit_build_simple_program(vs_text, fs_text2);
	glUseProgram(prog);

	glBindAttribLocation(prog, 0, "pos_in");

	glLinkProgram(prog);

	if (!piglit_check_gl_error(GL_NO_ERROR)) {
		piglit_report_result(PIGLIT_FAIL);
	}

	return prog;
}

static GLuint
make_shader_program3(void)
{
	GLuint prog;

	prog = piglit_build_simple_program(vs_text, fs_text3);
	glUseProgram(prog);

	glBindAttribLocation(prog, 0, "pos_in");

	glLinkProgram(prog);

	if (!piglit_check_gl_error(GL_NO_ERROR)) {
		piglit_report_result(PIGLIT_FAIL);
	}

	return prog;
}

static GLuint
make_vao(void)
{
	static const float pos_tc[12][2] = {
		{ -1.0, -1.0 },
		{  0.0, -1.0 },
		{  0.0,  1.0 },
		{  0.0,  1.0 },
		{ -1.0,  1.0 },
		{ -1.0, -1.0 },
		{ -1.0, -1.0 },
		{  1.0, -1.0 },
		{  1.0,  1.0 },
		{  1.0,  1.0 },
		{ -1.0,  1.0 },
		{ -1.0, -1.0 }
	};
	const int stride = sizeof(pos_tc[0]);
	GLuint vbo, vao;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pos_tc), pos_tc, GL_STATIC_DRAW);
	piglit_check_gl_error(GL_NO_ERROR);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *) 0);

	glEnableVertexAttribArray(0);

	if (!piglit_check_gl_error(GL_NO_ERROR)) {
		piglit_report_result(PIGLIT_FAIL);
	}

	return vbo;
}

static void
gl_init(int argc, char **argv)
{
	glXMakeContextCurrent(dpy, glx_windows[0], glx_windows[0], glx_contexts[0]);
	piglit_require_extension("GL_ARB_robustness");
	piglit_require_extension("GL_ARB_robustness_isolation");
	piglit_require_gl_version(40);
	prog1[0] = make_shader_program1();
	prog2[0] = make_shader_program2();
	vao[0] = make_vao();
	tex_color = make_texture_color();
	tex_depth = make_texture_depth();
	fbo = make_fbo();
	glEnable(GL_DEPTH_TEST);

	glXMakeContextCurrent(dpy, glx_windows[1], glx_windows[1], glx_contexts[1]);
	piglit_require_extension("GL_ARB_robustness");
	piglit_require_extension("GL_ARB_robustness_isolation");
	piglit_require_gl_version(40);

	prog1[1] = make_shader_program1();
	prog2[1] = make_shader_program3();
	vao[1] = make_vao();
	glEnable(GL_DEPTH_TEST);
}


static enum piglit_result
gl_draw(Display *dpy)
{
	int i;
	bool pass = true;
	GLenum status;

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, piglit_width, piglit_height);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	printf("draw 1\n");
	glXMakeContextCurrent(dpy, glx_windows[0], glx_windows[0], glx_contexts[0]);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(prog1[0]);;
	glDrawArrays(GL_TRIANGLES, 0, 12);
	//glUseProgram(prog2[0]);
	///glDrawArrays(GL_TRIANGLES, 6, 6);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_BACK);
	for (i = 0; i < 100000; i++) {
		glBlitFramebuffer(0, 0, piglit_width, piglit_height, 0, 0, piglit_width,
			piglit_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
	printf("flush\n");
	glFlush();
	//glXSwapBuffers(dpy, glx_windows[0]);

	printf("draw 2\n");
	glXMakeContextCurrent(dpy, glx_windows[1], glx_windows[1], glx_contexts[1]);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(prog2[1]);;
	glDrawArrays(GL_QUADS, 0, 12);
	//glUseProgram(prog2[1]);
	//glDrawArrays(GL_TRIANGLES, 6, 6);
	//glXSwapBuffers(dpy, glx_windows[1]);
	printf("flush\n");
	glFlush();

	printf("draw 1\n");
	glXMakeContextCurrent(dpy, glx_windows[0], glx_windows[0], glx_contexts[0]);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(prog1[0]);;
	glDrawArrays(GL_TRIANGLES, 0, 12);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_BACK);
	for (i = 0; i < 1000; i++) {
		glBlitFramebuffer(0, 0, piglit_width, piglit_height, 0, 0, piglit_width,
			piglit_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
	printf("flush\n");
	glFlush();
	glFinish();

	glXMakeContextCurrent(dpy, glx_windows[1], glx_windows[1], glx_contexts[1]);
	status = glGetGraphicsResetStatusARB();
	glXMakeContextCurrent(dpy, glx_windows[0], glx_windows[0], glx_contexts[0]);
	status = glGetGraphicsResetStatusARB();
	printf("status 2: %i\n", status);

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

int
main (int argc, char *argv[])
{
	glx_init();
	gl_init(argc, argv);
	//piglit_glx_event_loop(dpy, gl_draw);
	gl_draw(dpy);

	return 0;
}
