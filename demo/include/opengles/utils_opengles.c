/*
 * Copyright © 2011 Benjamin Franzke
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <math.h>
#include <assert.h>
#include <linux/input.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <stb_image.h>
#include <utils_opengles.h>

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a)[0])
#define MIN(a, b) ((a) < (b) ? (a) : (b))

RunningState global_state = {1}; // Set the initial value to 1, indicating that it is running

void log_error(const char *message)
{
	fprintf(stderr, "Error: %s\n", message);
}

static bool
check_egl_extension(const char *extensions, const char *extension)
{
	size_t extlen = strlen(extension);
	const char *end = extensions + strlen(extensions);

	while (extensions < end)
	{
		size_t n = 0;

		/* Skip whitespaces, if any */
		if (*extensions == ' ')
		{
			extensions++;
			continue;
		}

		n = strcspn(extensions, " ");

		/* Compare strings */
		if (n == extlen && strncmp(extension, extensions, n) == 0)
			return true; /* Found */

		extensions += n;
	}

	/* Not found */
	return false;
}

static inline void *
platform_get_egl_proc_address(const char *address)
{
	const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

	if (extensions &&
		(check_egl_extension(extensions, "EGL_EXT_platform_wayland") ||
		 check_egl_extension(extensions, "EGL_KHR_platform_wayland")))
	{
		return (void *)eglGetProcAddress(address);
	}

	return NULL;
}

static inline EGLDisplay
platform_get_egl_display(EGLenum platform, void *native_display,
						 const EGLint *attrib_list)
{
	static PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;

	if (!get_platform_display)
	{
		get_platform_display = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
			platform_get_egl_proc_address(
				"eglGetPlatformDisplayEXT");
	}

	if (get_platform_display)
		return get_platform_display(platform,
									native_display, attrib_list);

	return eglGetDisplay((EGLNativeDisplayType)native_display);
}

static inline EGLSurface
platform_create_egl_surface(EGLDisplay dpy, EGLConfig config,
							void *native_window,
							const EGLint *attrib_list)
{
	static PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC
		create_platform_window = NULL;

	if (!create_platform_window)
	{
		create_platform_window = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)
			platform_get_egl_proc_address(
				"eglCreatePlatformWindowSurfaceEXT");
	}

	if (create_platform_window)
		return create_platform_window(dpy, config,
									  native_window,
									  attrib_list);

	return eglCreateWindowSurface(dpy, config,
								  (EGLNativeWindowType)native_window,
								  attrib_list);
}

static inline EGLBoolean
platform_destroy_egl_surface(EGLDisplay display,
							 EGLSurface surface)
{
	return eglDestroySurface(display, surface);
}

void init_egl(struct Display *display, struct Window *window)
{
	static const struct
	{
		char *extension, *entrypoint;
	} swap_damage_ext_to_entrypoint[] = {
		{
			.extension = "EGL_EXT_swap_buffers_with_damage",
			.entrypoint = "eglSwapBuffersWithDamageEXT",
		},
		{
			.extension = "EGL_KHR_swap_buffers_with_damage",
			.entrypoint = "eglSwapBuffersWithDamageKHR",
		},
	};
	// Specify the OpenGLES3.0 version that EGL context should support
	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE};
	const char *extensions;

	EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_NONE};

	EGLint major, minor, n, count, i;
	EGLConfig *configs;
	EGLBoolean ret;

	if (window->opaque || window->buffer_size == 16)
		config_attribs[9] = 0;
	// Create EGL and obtain display connection
	display->egl.dpy = platform_get_egl_display(EGL_PLATFORM_WAYLAND_KHR, display->display, NULL);
	assert(display->egl.dpy);
	// Initialize EGL and configure
	ret = eglInitialize(display->egl.dpy, &major, &minor);
	assert(ret == EGL_TRUE);
	ret = eglBindAPI(EGL_OPENGL_ES_API);
	assert(ret == EGL_TRUE);

	if (!eglGetConfigs(display->egl.dpy, NULL, 0, &count) || count < 1)
	{
		log_error("Failed to get EGL configurations or no configurations available.");
	}

	configs = calloc(count, sizeof *configs);
	assert(configs);

	ret = eglChooseConfig(display->egl.dpy, config_attribs,
						  configs, count, &n);
	assert(ret && n >= 1);

	for (i = 0; i < n; i++)
	{
		EGLint buffer_size, red_size;
		eglGetConfigAttrib(display->egl.dpy,
						   configs[i], EGL_BUFFER_SIZE, &buffer_size);
		eglGetConfigAttrib(display->egl.dpy,
						   configs[i], EGL_RED_SIZE, &red_size);
		if ((window->buffer_size == 0 ||
			 window->buffer_size == buffer_size) &&
			red_size < 10)
		{
			display->egl.conf = configs[i];
			break;
		}
	}
	free(configs);
	if (display->egl.conf == NULL)
	{
		fprintf(stderr, "did not find config with buffer size %d\n",
				window->buffer_size);
		exit(EXIT_FAILURE);
	}
	// Create EGL context
	display->egl.ctx = eglCreateContext(display->egl.dpy,
										display->egl.conf,
										EGL_NO_CONTEXT, context_attribs);
	assert(display->egl.ctx);

	display->swap_buffers_with_damage = NULL;
	extensions = eglQueryString(display->egl.dpy, EGL_EXTENSIONS);
	if (extensions &&
		check_egl_extension(extensions, "EGL_EXT_buffer_age"))
	{
		for (i = 0; i < (int)ARRAY_LENGTH(swap_damage_ext_to_entrypoint); i++)
		{
			if (check_egl_extension(extensions, swap_damage_ext_to_entrypoint[i].extension))
			{
				/* The EXTPROC is identical to the KHR one */
				display->swap_buffers_with_damage =
					(PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC)
						eglGetProcAddress(swap_damage_ext_to_entrypoint[i].entrypoint);
				break;
			}
		}
	}

	if (display->swap_buffers_with_damage)
		printf("has EGL_EXT_buffer_age and %s\n", swap_damage_ext_to_entrypoint[i].extension);
}

static void
fini_egl(struct Display *display)
{
	eglTerminate(display->egl.dpy);
	eglReleaseThread();
}

GLuint load_shader(const char *source, GLenum shader_type)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(shader_type);
	assert(shader != 0);

	glShaderSource(shader, 1, (const char **)&source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		char log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		fprintf(stderr, "Error: compiling %s: %.*s\n",
				shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
				len, log);
		exit(1);
	}

	return shader;
}

void init_gl(struct Window *window)
{
	GLuint frag, vert;
	GLuint program;
	GLint status;
	EGLBoolean ret;

	window->native = wl_egl_window_create(window->surface,
										  window->geometry.width,
										  window->geometry.height);

	window->egl_surface = platform_create_egl_surface(window->display->egl.dpy,
													  window->display->egl.conf,
													  window->native, NULL);

	ret = eglMakeCurrent(window->display->egl.dpy, window->egl_surface,
						 window->egl_surface, window->display->egl.ctx);
	assert(ret == EGL_TRUE);
}

static void
handle_surface_configure(void *data, struct xdg_surface *surface,
						 uint32_t serial)
{
	struct Window *window = data;

	xdg_surface_ack_configure(surface, serial);

	window->wait_for_configure = false;
}

static const struct xdg_surface_listener xdg_surface_listener = {
	handle_surface_configure};

static void
handle_toplevel_configure(void *data, struct xdg_toplevel *toplevel,
						  int32_t width, int32_t height,
						  struct wl_array *states)
{
	struct Window *window = data;
	uint32_t *p;

	window->fullscreen = 0;
	window->maximized = 0;
	wl_array_for_each(p, states)
	{
		uint32_t state = *p;
		switch (state)
		{
		case XDG_TOPLEVEL_STATE_FULLSCREEN:
			window->fullscreen = 1;
			break;
		case XDG_TOPLEVEL_STATE_MAXIMIZED:
			window->maximized = 1;
			break;
		}
	}

	if (width > 0 && height > 0)
	{
		if (!window->fullscreen && !window->maximized)
		{
			window->window_size.width = width;
			window->window_size.height = height;
		}
		window->geometry.width = width;
		window->geometry.height = height;
	}
	else if (!window->fullscreen && !window->maximized)
	{
		window->geometry = window->window_size;
	}

	if (window->native)
		wl_egl_window_resize(window->native,
							 window->geometry.width,
							 window->geometry.height, 0, 0);
}

static void
handle_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	global_state.running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	handle_toplevel_configure,
	handle_toplevel_close,
};

void create_surface(struct Window *window)
{
	struct Display *display = window->display;

	window->surface = wl_compositor_create_surface(display->compositor);

	window->xdg_surface = xdg_wm_base_get_xdg_surface(display->wm_base,
													  window->surface);
	xdg_surface_add_listener(window->xdg_surface,
							 &xdg_surface_listener, window);

	window->xdg_toplevel =
		xdg_surface_get_toplevel(window->xdg_surface);
	xdg_toplevel_add_listener(window->xdg_toplevel,
							  &xdg_toplevel_listener, window);

	xdg_toplevel_set_title(window->xdg_toplevel, "simple-egl");
	xdg_toplevel_set_app_id(window->xdg_toplevel,
							"org.freedesktop.weston.simple-egl");

	if (window->fullscreen)
		xdg_toplevel_set_fullscreen(window->xdg_toplevel, NULL);
	else if (window->maximized)
		xdg_toplevel_set_maximized(window->xdg_toplevel);

	window->wait_for_configure = true;
	wl_surface_commit(window->surface);

	if (!window->frame_sync)
		eglSwapInterval(display->egl.dpy, 0);
}

void destroy_surface(struct Window *window)
{
	/* Required, otherwise segfault in egl_dri2.c: dri2_make_current()
	 * on eglReleaseThread(). */
	eglMakeCurrent(window->display->egl.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
				   EGL_NO_CONTEXT);

	platform_destroy_egl_surface(window->display->egl.dpy,
								 window->egl_surface);
	wl_egl_window_destroy(window->native);

	if (window->xdg_toplevel)
		xdg_toplevel_destroy(window->xdg_toplevel);
	if (window->xdg_surface)
		xdg_surface_destroy(window->xdg_surface);
	wl_surface_destroy(window->surface);

	if (window->callback)
		wl_callback_destroy(window->callback);
}

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
					 uint32_t serial, struct wl_surface *surface,
					 wl_fixed_t sx, wl_fixed_t sy)
{
	struct Display *display = data;
	struct wl_buffer *buffer;
	struct wl_cursor *cursor = display->default_cursor;
	struct wl_cursor_image *image;

	if (display->window->fullscreen)
		wl_pointer_set_cursor(pointer, serial, NULL, 0, 0);
	else if (cursor)
	{
		image = display->default_cursor->images[0];
		buffer = wl_cursor_image_get_buffer(image);
		if (!buffer)
			return;
		wl_pointer_set_cursor(pointer, serial,
							  display->cursor_surface,
							  image->hotspot_x,
							  image->hotspot_y);
		wl_surface_attach(display->cursor_surface, buffer, 0, 0);
		wl_surface_damage(display->cursor_surface, 0, 0,
						  image->width, image->height);
		wl_surface_commit(display->cursor_surface);
	}
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
					 uint32_t serial, struct wl_surface *surface)
{
}

static void
pointer_handle_motion(void *data, struct wl_pointer *pointer,
					  uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
}

static void
pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
					  uint32_t serial, uint32_t time, uint32_t button,
					  uint32_t state)
{
	struct Display *display = data;

	if (!display->window->xdg_toplevel)
		return;

	if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
		xdg_toplevel_move(display->window->xdg_toplevel,
						  display->seat, serial);
}

static void
pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
					uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

static const struct wl_pointer_listener pointer_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis,
};

static void
touch_handle_down(void *data, struct wl_touch *wl_touch,
				  uint32_t serial, uint32_t time, struct wl_surface *surface,
				  int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
	struct Display *d = (struct Display *)data;

	if (!d->wm_base)
		return;

	xdg_toplevel_move(d->window->xdg_toplevel, d->seat, serial);
}

static void
touch_handle_up(void *data, struct wl_touch *wl_touch,
				uint32_t serial, uint32_t time, int32_t id)
{
}

static void
touch_handle_motion(void *data, struct wl_touch *wl_touch,
					uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
}

static void
touch_handle_frame(void *data, struct wl_touch *wl_touch)
{
}

static void
touch_handle_cancel(void *data, struct wl_touch *wl_touch)
{
}

static const struct wl_touch_listener touch_listener = {
	touch_handle_down,
	touch_handle_up,
	touch_handle_motion,
	touch_handle_frame,
	touch_handle_cancel,
};

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
					   uint32_t format, int fd, uint32_t size)
{
	/* Just so we don’t leak the keymap fd */
	close(fd);
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
					  uint32_t serial, struct wl_surface *surface,
					  struct wl_array *keys)
{
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
					  uint32_t serial, struct wl_surface *surface)
{
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
					uint32_t serial, uint32_t time, uint32_t key,
					uint32_t state)
{
	struct Display *d = data;

	if (!d->wm_base)
		return;

	if (key == KEY_F11 && state)
	{
		if (d->window->fullscreen)
			xdg_toplevel_unset_fullscreen(d->window->xdg_toplevel);
		else
			xdg_toplevel_set_fullscreen(d->window->xdg_toplevel, NULL);
	}
	else if (key == KEY_ESC && state)
		global_state.running = 0;
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
						  uint32_t serial, uint32_t mods_depressed,
						  uint32_t mods_latched, uint32_t mods_locked,
						  uint32_t group)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
						 enum wl_seat_capability caps)
{
	struct Display *d = data;

	if ((caps & WL_SEAT_CAPABILITY_POINTER) && !d->pointer)
	{
		d->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(d->pointer, &pointer_listener, d);
	}
	else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && d->pointer)
	{
		wl_pointer_destroy(d->pointer);
		d->pointer = NULL;
	}

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !d->keyboard)
	{
		d->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(d->keyboard, &keyboard_listener, d);
	}
	else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && d->keyboard)
	{
		wl_keyboard_destroy(d->keyboard);
		d->keyboard = NULL;
	}

	if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !d->touch)
	{
		d->touch = wl_seat_get_touch(seat);
		wl_touch_set_user_data(d->touch, d);
		wl_touch_add_listener(d->touch, &touch_listener, d);
	}
	else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && d->touch)
	{
		wl_touch_destroy(d->touch);
		d->touch = NULL;
	}
}

static const struct wl_seat_listener seat_listener = {
	seat_handle_capabilities,
};

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
	xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
	xdg_wm_base_ping,
};

void registry_handle_global(void *data, struct wl_registry *registry,
							uint32_t name, const char *interface, uint32_t version)
{
	struct Display *d = data;

	if (strcmp(interface, "wl_compositor") == 0)
	{
		d->compositor =
			wl_registry_bind(registry, name,
							 &wl_compositor_interface,
							 MIN(version, 4));
	}
	else if (strcmp(interface, "xdg_wm_base") == 0)
	{
		d->wm_base = wl_registry_bind(registry, name,
									  &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(d->wm_base, &wm_base_listener, d);
	}
	else if (strcmp(interface, "wl_seat") == 0)
	{
		d->seat = wl_registry_bind(registry, name,
								   &wl_seat_interface, 1);
		wl_seat_add_listener(d->seat, &seat_listener, d);
	}
	else if (strcmp(interface, "wl_shm") == 0)
	{
		d->shm = wl_registry_bind(registry, name,
								  &wl_shm_interface, 1);
		d->cursor_theme = wl_cursor_theme_load(NULL, 32, d->shm);
		if (!d->cursor_theme)
		{
			fprintf(stderr, "unable to load default theme\n");
			return;
		}
		d->default_cursor =
			wl_cursor_theme_get_cursor(d->cursor_theme, "left_ptr");
		if (!d->default_cursor)
		{
			fprintf(stderr, "unable to load default left pointer\n");
			// TODO: abort ?
		}
	}
}

void registry_handle_global_remove(void *data, struct wl_registry *registry,
								   uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove};

void usage(int error_code)
{
	fprintf(stderr, "Usage: simple-egl [OPTIONS]\n\n"
					"  -d <us>\tBuffer swap delay in microseconds\n"
					"  -f\tRun in fullscreen mode\n"
					"  -m\tRun in maximized mode\n"
					"  -o\tCreate an opaque surface\n"
					"  -s\tUse a 16 bpp EGL config\n"
					"  -b\tDon't sync to compositor redraw (eglSwapInterval 0)\n"
					"  -h\tThis help text\n\n");

	exit(error_code);
}

static int handle_events(int fd, uint32_t mask, void *data) {
    struct Display *display = data;
    if (wl_display_dispatch_pending(display->display) == -1) {
        global_state.running = false;
    }
	return 0;
}
int create_window(struct Window *window, struct Display *display, int ret)
{

	display->display = wl_display_connect(NULL);
	// If the connection fails, prompt the user to set environment variables
	if (!display->display)
	{
		log_error("Failed to connect to the Wayland display. Please ensure that the WAYLAND_DISPLAY environment variable is set correctly.");
		free(display);
		return EXIT_FAILURE;
	}

	display->registry = wl_display_get_registry(display->display);
	wl_registry_add_listener(display->registry,
							 &registry_listener, display);

	wl_display_roundtrip(display->display);

	if (!display->wm_base)
	{
		fprintf(stderr, "xdg-shell support required. simple-egl exiting\n");
		return -1;
	}

	// fprintf(stderr, "Initializing EGL...\n");
	init_egl(display, window);

	create_surface(window);
	// fprintf(stderr, "Created surface success.\n");

	/* we already have wait_for_configure set after create_surface() */
	while (global_state.running && ret != -1 && window->wait_for_configure)
	{
		ret = wl_display_dispatch(display->display);
		/* wait until xdg_surface::configure acks the new dimensions */
		if (window->wait_for_configure)
			continue;

		init_gl(window);
	}

	display->cursor_surface = wl_compositor_create_surface(display->compositor);

#if defined(WL_OPAQUE_REGION)
	display->cursor_region = wl_compositor_create_region(display->compositor);
	// Add a rectangle to the region
	wl_region_add(window->display->cursor_region, 0, 0, window->geometry.width, window->geometry.height);
	// Set opaque area
	wl_surface_set_opaque_region(window->surface, window->display->cursor_region);
#endif

    struct wl_event_loop *event_loop = wl_display_get_event_loop(display->display);
    wl_event_loop_add_fd(event_loop, wl_display_get_fd(display->display), 1, handle_events, &display);

	return 0;
}

void destroy_window(struct Window *window, struct Display *display)
{
	// clean
	destroy_surface(window);
	fini_egl(display);
	wl_surface_destroy(display->cursor_surface);
#if defined(WL_OPAQUE_REGION)
	wl_region_destroy(display->cursor_region);
#endif
	if (display->cursor_theme)
		wl_cursor_theme_destroy(display->cursor_theme);

	if (display->shm)
		wl_shm_destroy(display->shm);

	if (display->pointer)
		wl_pointer_destroy(display->pointer);

	if (display->keyboard)
		wl_keyboard_destroy(display->keyboard);

	if (display->touch)
		wl_touch_destroy(display->touch);

	if (display->seat)
		wl_seat_destroy(display->seat);

	if (display->wm_base)
		xdg_wm_base_destroy(display->wm_base);

	if (display->compositor)
		wl_compositor_destroy(display->compositor);

	wl_registry_destroy(display->registry);
	wl_display_flush(display->display);
	wl_display_disconnect(display->display);
}

void esScale(ESMatrix *result, GLfloat sx, GLfloat sy, GLfloat sz)
{
	result->m[0][0] *= sx;
	result->m[0][1] *= sx;
	result->m[0][2] *= sx;
	result->m[0][3] *= sx;

	result->m[1][0] *= sy;
	result->m[1][1] *= sy;
	result->m[1][2] *= sy;
	result->m[1][3] *= sy;

	result->m[2][0] *= sz;
	result->m[2][1] *= sz;
	result->m[2][2] *= sz;
	result->m[2][3] *= sz;
}

void esTranslate(ESMatrix *result, GLfloat tx, GLfloat ty, GLfloat tz)
{
	result->m[3][0] += (result->m[0][0] * tx + result->m[1][0] * ty + result->m[2][0] * tz);
	result->m[3][1] += (result->m[0][1] * tx + result->m[1][1] * ty + result->m[2][1] * tz);
	result->m[3][2] += (result->m[0][2] * tx + result->m[1][2] * ty + result->m[2][2] * tz);
	result->m[3][3] += (result->m[0][3] * tx + result->m[1][3] * ty + result->m[2][3] * tz);
}

void esRotate(ESMatrix *result, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	GLfloat sinAngle, cosAngle;
	GLfloat mag = sqrtf(x * x + y * y + z * z);

	sinAngle = sinf(angle * PI / 180.0f);
	cosAngle = cosf(angle * PI / 180.0f);

	if (mag > 0.0f)
	{
		GLfloat xx, yy, zz, xy, yz, zx, xs, ys, zs;
		GLfloat oneMinusCos;
		ESMatrix rotMat;

		x /= mag;
		y /= mag;
		z /= mag;

		xx = x * x;
		yy = y * y;
		zz = z * z;
		xy = x * y;
		yz = y * z;
		zx = z * x;
		xs = x * sinAngle;
		ys = y * sinAngle;
		zs = z * sinAngle;
		oneMinusCos = 1.0f - cosAngle;

		rotMat.m[0][0] = (oneMinusCos * xx) + cosAngle;
		rotMat.m[0][1] = (oneMinusCos * xy) - zs;
		rotMat.m[0][2] = (oneMinusCos * zx) + ys;
		rotMat.m[0][3] = 0.0F;

		rotMat.m[1][0] = (oneMinusCos * xy) + zs;
		rotMat.m[1][1] = (oneMinusCos * yy) + cosAngle;
		rotMat.m[1][2] = (oneMinusCos * yz) - xs;
		rotMat.m[1][3] = 0.0F;

		rotMat.m[2][0] = (oneMinusCos * zx) - ys;
		rotMat.m[2][1] = (oneMinusCos * yz) + xs;
		rotMat.m[2][2] = (oneMinusCos * zz) + cosAngle;
		rotMat.m[2][3] = 0.0F;

		rotMat.m[3][0] = 0.0F;
		rotMat.m[3][1] = 0.0F;
		rotMat.m[3][2] = 0.0F;
		rotMat.m[3][3] = 1.0F;

		esMatrixMultiply(result, &rotMat, result);
	}
}

void esFrustum(ESMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ)
{
	float deltaX = right - left;
	float deltaY = top - bottom;
	float deltaZ = farZ - nearZ;
	ESMatrix frust;

	if ((nearZ <= 0.0f) || (farZ <= 0.0f) ||
		(deltaX <= 0.0f) || (deltaY <= 0.0f) || (deltaZ <= 0.0f))
	{
		return;
	}

	frust.m[0][0] = 2.0f * nearZ / deltaX;
	frust.m[0][1] = frust.m[0][2] = frust.m[0][3] = 0.0f;

	frust.m[1][1] = 2.0f * nearZ / deltaY;
	frust.m[1][0] = frust.m[1][2] = frust.m[1][3] = 0.0f;

	frust.m[2][0] = (right + left) / deltaX;
	frust.m[2][1] = (top + bottom) / deltaY;
	frust.m[2][2] = -(nearZ + farZ) / deltaZ;
	frust.m[2][3] = -1.0f;

	frust.m[3][2] = -2.0f * nearZ * farZ / deltaZ;
	frust.m[3][0] = frust.m[3][1] = frust.m[3][3] = 0.0f;

	esMatrixMultiply(result, &frust, result);
}

void esPerspective(ESMatrix *result, float fovy, float aspect, float nearZ, float farZ)
{
	// Field of View (FOV), measured in degrees. It defines the vertical range of the scene visible from the observer's position.
	// Float nearZ: Near Plane, which specifies how close objects will be rendered from the observer (camera) position. Any object closer to the observer will be cropped and will not participate in rendering.
	// Float farZ: Far Plane, which defines the farthest point of the rendering space.
	GLfloat frustumW, frustumH;

	frustumH = tanf(fovy / 360.0f * PI) * nearZ;
	frustumW = frustumH * aspect;

	esFrustum(result, -frustumW, frustumW, -frustumH, frustumH, nearZ, farZ);
}

void esOrtho(ESMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ)
{
	float deltaX = right - left;
	float deltaY = top - bottom;
	float deltaZ = farZ - nearZ;
	ESMatrix ortho;

	if ((deltaX == 0.0f) || (deltaY == 0.0f) || (deltaZ == 0.0f))
	{
		return;
	}

	esMatrixLoadIdentity(&ortho);
	ortho.m[0][0] = 2.0f / deltaX;
	ortho.m[3][0] = -(right + left) / deltaX;
	ortho.m[1][1] = 2.0f / deltaY;
	ortho.m[3][1] = -(top + bottom) / deltaY;
	ortho.m[2][2] = -2.0f / deltaZ;
	ortho.m[3][2] = -(nearZ + farZ) / deltaZ;

	esMatrixMultiply(result, &ortho, result);
}

void esMatrixMultiply(ESMatrix *result, ESMatrix *srcA, ESMatrix *srcB)
{
	ESMatrix tmp;
	int i;

	for (i = 0; i < 4; i++)
	{
		tmp.m[i][0] = (srcA->m[i][0] * srcB->m[0][0]) +
					  (srcA->m[i][1] * srcB->m[1][0]) +
					  (srcA->m[i][2] * srcB->m[2][0]) +
					  (srcA->m[i][3] * srcB->m[3][0]);

		tmp.m[i][1] = (srcA->m[i][0] * srcB->m[0][1]) +
					  (srcA->m[i][1] * srcB->m[1][1]) +
					  (srcA->m[i][2] * srcB->m[2][1]) +
					  (srcA->m[i][3] * srcB->m[3][1]);

		tmp.m[i][2] = (srcA->m[i][0] * srcB->m[0][2]) +
					  (srcA->m[i][1] * srcB->m[1][2]) +
					  (srcA->m[i][2] * srcB->m[2][2]) +
					  (srcA->m[i][3] * srcB->m[3][2]);

		tmp.m[i][3] = (srcA->m[i][0] * srcB->m[0][3]) +
					  (srcA->m[i][1] * srcB->m[1][3]) +
					  (srcA->m[i][2] * srcB->m[2][3]) +
					  (srcA->m[i][3] * srcB->m[3][3]);
	}

	memcpy(result, &tmp, sizeof(ESMatrix));
}

void esMatrixLoadIdentity(ESMatrix *result)
{
	memset(result, 0x0, sizeof(ESMatrix));
	result->m[0][0] = 1.0f;
	result->m[1][1] = 1.0f;
	result->m[2][2] = 1.0f;
	result->m[3][3] = 1.0f;
}

void esMatrixLookAt(ESMatrix *result,
					float posX, float posY, float posZ,
					float lookAtX, float lookAtY, float lookAtZ,
					float upX, float upY, float upZ)
{
	float axisX[3], axisY[3], axisZ[3];
	float length;

	// axisZ = lookAt - pos
	axisZ[0] = lookAtX - posX;
	axisZ[1] = lookAtY - posY;
	axisZ[2] = lookAtZ - posZ;

	// normalize axisZ
	length = sqrtf(axisZ[0] * axisZ[0] + axisZ[1] * axisZ[1] + axisZ[2] * axisZ[2]);

	if (length != 0.0f)
	{
		axisZ[0] /= length;
		axisZ[1] /= length;
		axisZ[2] /= length;
	}

	// axisX = up X axisZ
	axisX[0] = upY * axisZ[2] - upZ * axisZ[1];
	axisX[1] = upZ * axisZ[0] - upX * axisZ[2];
	axisX[2] = upX * axisZ[1] - upY * axisZ[0];

	// normalize axisX
	length = sqrtf(axisX[0] * axisX[0] + axisX[1] * axisX[1] + axisX[2] * axisX[2]);

	if (length != 0.0f)
	{
		axisX[0] /= length;
		axisX[1] /= length;
		axisX[2] /= length;
	}

	// axisY = axisZ x axisX
	axisY[0] = axisZ[1] * axisX[2] - axisZ[2] * axisX[1];
	axisY[1] = axisZ[2] * axisX[0] - axisZ[0] * axisX[2];
	axisY[2] = axisZ[0] * axisX[1] - axisZ[1] * axisX[0];

	// normalize axisY
	length = sqrtf(axisY[0] * axisY[0] + axisY[1] * axisY[1] + axisY[2] * axisY[2]);

	if (length != 0.0f)
	{
		axisY[0] /= length;
		axisY[1] /= length;
		axisY[2] /= length;
	}

	memset(result, 0x0, sizeof(ESMatrix));

	result->m[0][0] = -axisX[0];
	result->m[0][1] = axisY[0];
	result->m[0][2] = -axisZ[0];

	result->m[1][0] = -axisX[1];
	result->m[1][1] = axisY[1];
	result->m[1][2] = -axisZ[1];

	result->m[2][0] = -axisX[2];
	result->m[2][1] = axisY[2];
	result->m[2][2] = -axisZ[2];

	// translate (-posX, -posY, -posZ)
	result->m[3][0] = axisX[0] * posX + axisX[1] * posY + axisX[2] * posZ;
	result->m[3][1] = -axisY[0] * posX - axisY[1] * posY - axisY[2] * posZ;
	result->m[3][2] = axisZ[0] * posX + axisZ[1] * posY + axisZ[2] * posZ;
	result->m[3][3] = 1.0f;
}

typedef struct
{
	// Handle to a program object
	GLuint programObject;
	// Uniform locations
	GLint mvpLoc;
	// Vertex data
	GLfloat *vertices;
	GLuint *indices;
	int numIndices;
	// Rotation angle
	GLfloat angle;
	// MVP matrix
	ESMatrix mvpMatrix;
	ESMatrix perspective;
	ESMatrix modelview;
	// vertex array, buffer
	GLuint vao, vbo, ebo;

} UserData;

// Check shader compilation status
void checkShaderCompilation(GLuint shader)
{
	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE)
	{
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		char *log = (char *)malloc(logLength * sizeof(char));
		glGetShaderInfoLog(shader, logLength, NULL, log);
		printf("Shader compilation failed.\n");
		printf("Shader info log:\n%s\n", log);
		free(log);
		exit(EXIT_FAILURE);
	}
}

void checkProgramLinking(GLuint program)
{
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
		char *strInfoLog = (char *)malloc((size_t)(infoLogLength + 1));
		glGetProgramInfoLog(program, (GLsizei)infoLogLength, NULL, strInfoLog);
		printf("Linker failure in program:\n%s\n", strInfoLog);
		free(strInfoLog);
	}
}

//
/// \brief Load a vertex and fragment shader, create a program object, link program.
//         Errors output to log.
/// \param vertShaderSrc Vertex shader source code
/// \param fragShaderSrc Fragment shader source code
/// \return A new program object linked with the vertex/fragment shader pair, 0 on failure
//
GLint load_program(const char *vertShaderSrc, const char *fragShaderSrc)
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;

	// Load the vertex/fragment shaders
	vertexShader = load_shader(vertShaderSrc, GL_VERTEX_SHADER);

	if (vertexShader == 0)
	{
		return 0;
	}

	fragmentShader = load_shader(fragShaderSrc, GL_FRAGMENT_SHADER);

	if (fragmentShader == 0)
	{
		glDeleteShader(vertexShader);
		return 0;
	}

	// Create the program object
	programObject = glCreateProgram();

	if (programObject == 0)
	{
		return 0;
	}

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	// Link the program
	glLinkProgram(programObject);

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint infoLen = 0;

		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char *infoLog = malloc(sizeof(char) * infoLen);

			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			printf("Error linking program:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(programObject);
		return 0;
	}

	// Free up no longer needed shader resources
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return programObject;
}

void printVertices(float *vertices, int numVertices, int num_row)
{
	int i;
	for (i = 0; i < numVertices * num_row; i++)
	{
		printf("%f ", vertices[i]);
		if ((i + 1) % num_row == 0)
			printf("\n");
	}
}

GLuint loadTexture(const char *filename)
{

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	int width, height, nrChannels;
	unsigned char *data_texture = stbi_load(filename, &width, &height, &nrChannels, 0);

	if (data_texture)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_texture);
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data_texture);
	}
	else
	{
		log_error(filename);
		printf("Failed to load texture: %s\n", stbi_failure_reason());
	}

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return textureID;
}

unsigned char *load_imageData(const char *filename, int *width, int *height, int *channels, int req_comp)
{
	unsigned char *data = stbi_load(filename, width, height, channels, req_comp);
	if (!data)
	{
		fprintf(stderr, "Failed to load image '%s'.\n", filename);
		// 如果加载失败，直接返回 NULL
		return NULL;
	}
	return data;
}

void free_imageData(unsigned char *data)
{
	if (data != NULL)
	{
		stbi_image_free(data);
	}
}
