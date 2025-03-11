/*
 * Copyright (C) 2025 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef __GPU_RENDER_H_
#define __GPU_RENDER_H_
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <utils_opengles.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "cam_log.h"

#include <gbm.h>
#include <libdrm/drm_fourcc.h>
#include <GLES2/gl2ext.h>

#include <sys/ioctl.h>
#include <linux/dma-heap.h>
#include <drm/drm_fourcc.h>
#include <errno.h>
#include <wayland-server-core.h>

// #define GPU_RENDER_SAVE
typedef struct
{
	// Handle to a program object
	GLuint programObject;
	// Uniform locations
	GLint mvpLoc;
	// Vertex data
	GLfloat *vertices;
	int num_indices[2];
	// Rotation angle
	GLfloat angle;
	uint32_t rotate_benchmark_time;
	// MVP matrix
	ESMatrix mvpMatrix;
	ESMatrix perspective;
	ESMatrix modelview;
	// vertex array, buffer
	GLuint vao, vbo, ebo;
	GLuint textureID;

	GLuint *textures;
	GLuint *out_textures;
	int current_texture_index;

} UserData;

int gl_window_init(struct Window *window);
void gl_window_draw(struct Window *window);
void gl_window_shutdown(void *data);
GLuint create_texture_dma(struct Display *display, int width, int height, int dma_buf_fd);
GLuint create_texture_outdma(struct Display *display, void **buffer, int nrChannels);
bool save_buffer_to_bin(void *buffer, const char *filename, int total_size);

#endif