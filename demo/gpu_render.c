#include "gpu_render.h"
#define OPENGLES_DEBUG

// Initialize the shader and program object
int gl_window_init(struct Window *window)
{
	UserData *userData = (UserData *)window->userData;

	char vertShaderSrc[] = {
		"#version 300 es\n"
		"layout(location = 0) in vec3 vPosition;\n"
		"layout(location = 1) in vec2 aTexCoord;\n"
		"layout(location = 2) in vec4 aColor;\n"
		"out vec2 vTextureCoord;\n"
		"out vec4 vColor;\n"
		"uniform mat4 u_mvpMatrix;\n"
		"void main() {\n"
		"    gl_Position = u_mvpMatrix * vec4(vPosition, 1.0f);\n"
		"    vTextureCoord = vec2(1.0 - aTexCoord.x, 1.0 - aTexCoord.y);\n"
		"    vColor = aColor;\n"
		"}\n"};


	char fragShaderSrc[] = {
        "#version 300 es\n"
        "#extension GL_OES_EGL_image_external : require\n"
        "#extension GL_OES_EGL_image_external_essl3 : require\n"
        "precision mediump float;\n"
        "uniform samplerExternalOES uSampler;\n"
        "uniform bool useTexture;\n"
        "in vec2 vTextureCoord;\n"
        "in vec4 vColor;\n"
        "out vec4 outColor;\n"

        "void main() {\n"
        "	vec4 sampled = texture(uSampler, vTextureCoord);\n"
        "   outColor = vec4(sampled.r, sampled.g, sampled.b, 1.0);\n"
        "}\n"
    };

	userData->programObject = load_program(vertShaderSrc, fragShaderSrc);

    // Compute the window aspect ratio
	float aspect = (float)window->geometry.width / (float)window->geometry.height;

    float vertices[] = {
     // Vertex            // texture
     aspect,  1.0f, 0.0f, 1.0f, 0.0f, // point 1
     aspect, -1.0f, 0.0f, 1.0f, 1.0f, // point 2
    -aspect, -1.0f, 0.0f, 0.0f, 1.0f, // point 3
    -aspect,  1.0f, 0.0f, 0.0f, 0.0f  // point 4
    };

	unsigned int indices[] = {
		0, 1, 3, // The first triangle
		1, 2, 3, // The second triangle
	};

	userData->vertices = (GLfloat *)malloc(sizeof(vertices));
	memcpy(userData->vertices, vertices, sizeof(vertices));

	userData->num_indices[0] = sizeof(indices) / sizeof(indices[0]);

	// Bind Vertex Array Object (VAO) and Vertex Buffer Object (VBO), and send vertex data to GPU
	glGenVertexArrays(1, &userData->vao);
	glBindVertexArray(userData->vao);

	glGenBuffers(1, &userData->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, userData->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), userData->vertices, GL_STATIC_DRAW);

	// Copy the index array into an index buffer for OpenGL to use
	glGenBuffers(1, &userData->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, userData->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Set vertex pointer attribute
	GLuint positionAttrib = glGetAttribLocation(userData->programObject, "vPosition");
	glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(positionAttrib);

	// Texture position attribute
	GLuint textureAttrib = glGetAttribLocation(userData->programObject, "aTexCoord");
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(textureAttrib);

	// Generate a perspective matrix with a 60 degree FOV
	esMatrixLoadIdentity(&userData->perspective);
	esOrtho(&userData->perspective, -aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

	// Generate a model view matrix to rotate/translate the cube
	esMatrixLoadIdentity(&userData->modelview);

	// Translate away from the viewer
	esMatrixLoadIdentity(&userData->modelview);

	// Generate a mvp matrix
	esMatrixMultiply(&userData->mvpMatrix, &userData->modelview, &userData->perspective);

	return 1;
}

void gl_window_draw(struct Window *window)
{
	struct Display *display = window->display;
	UserData *userData = (UserData *)window->userData;

	window->callback = NULL;

	// Set the viewport
	glViewport(0, 0, window->geometry.width, window->geometry.height);
	// Clear the color buffer
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	// Use the program object
	glUseProgram(userData->programObject);

	// Load the MVP matrix
	glUniformMatrix4fv(glGetUniformLocation(userData->programObject, "u_mvpMatrix"), 1, GL_FALSE, (GLfloat *)&userData->mvpMatrix.m[0][0]);

	glBindVertexArray(userData->vao);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		CLOG_ERROR("OpenGL Error: %d\n", error);
	}

	// Rendering
	glUniform1i(glGetUniformLocation(userData->programObject, "useTexture"), GL_TRUE);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, userData->textures[userData->current_texture_index]); // Bind the correct texture
	glUniform1i(glGetUniformLocation(userData->programObject, "uSampler"), 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, userData->ebo);
	glDrawElements(GL_TRIANGLES, userData->num_indices[0], GL_UNSIGNED_INT, 0);

	// Swap buffer
    // if (ready_to_render) {
	    // Swap buffer
	    eglSwapBuffers(display->egl.dpy, window->egl_surface);

    //     ready_to_render = false;
    //     render_done = true;
    // }
}

// Cleanup
void gl_window_shutdown(void *data)
{
	struct Window *window = data;

	UserData *userData = window->userData;

	if (userData->vbo != 0)	{
		glDeleteBuffers(1, &userData->vbo);
		userData->vbo = 0;
	}

	if (userData->vao != 0)	{
		glDeleteBuffers(1, &userData->vao);
		userData->vao = 0;
	}

	if (userData->ebo != 0)	{
		glDeleteBuffers(1, &userData->ebo);
		userData->ebo = 0;
	}

	if (userData->vertices != NULL) {
		free(userData->vertices);
	}

	// Delete program object
	glDeleteProgram(userData->programObject);
}

GLuint create_texture_dma(struct Display *display, int width, int height, int dma_buf_fd)
{
	int uv_plane_offset = width * height;
	int aligned_uv_pitch = width;

	// Configure EGL attributes for single-plane NV12
	EGLint attribs[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_NV12,
		EGL_DMA_BUF_PLANE0_FD_EXT, dma_buf_fd,             // Y plane
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,                  // Y plane offset
		EGL_DMA_BUF_PLANE0_PITCH_EXT, width,               // Y plane pitch

		EGL_DMA_BUF_PLANE1_FD_EXT, dma_buf_fd,               // UV plane (same FD)
		EGL_DMA_BUF_PLANE1_OFFSET_EXT, uv_plane_offset,      // UV plane offset
		EGL_DMA_BUF_PLANE1_PITCH_EXT, aligned_uv_pitch,      // UV plane pitch
		EGL_NONE
	};

	PFNEGLCREATEIMAGEKHRPROC create_image;
	create_image = (void *)eglGetProcAddress("eglCreateImageKHR");
	EGLImageKHR egl_image = create_image(display->egl.dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
										 NULL, attribs);

	const unsigned char *extensions = glGetString(GL_EXTENSIONS);
	// if (strstr(extensions, "GL_OES_EGL_image_external") == NULL) {
	// 	fprintf(stderr, "Error: GL_OES_EGL_image_external extension not supported!\n");
	// 	return -1;
	// }

	if (egl_image == EGL_NO_IMAGE_KHR) {
		CLOG_ERROR("EGLImageKHR creation failed, eglGetError():0x%x\n", eglGetError());
		return false;
	}

	GLuint ex_texture;
	glGenTextures(1, &ex_texture);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, ex_texture);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d; // for openGL ES2.0
	image_target_texture_2d = (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");
	image_target_texture_2d(GL_TEXTURE_EXTERNAL_OES, egl_image);
#if defined(OPENGLES_DEBUG)
	CLOG_INFO("w:%d, h:%d, glGetError(): %d\n", width, height, glGetError());
#endif
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
	return ex_texture;
}