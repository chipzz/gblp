#include "openglblp.h"
#include "blp.h"

#include <cstdio>
static void glPrintError()
{
	GLuint const error = glGetError();
	if (error == GL_NO_ERROR)
		return;
	switch (error)
	{
		case GL_INVALID_ENUM:
			fprintf(stderr, "GL_INVALID_ENUM\n");
			break;
		case GL_INVALID_VALUE:
			fprintf(stderr, "GL_INVALID_VALUE\n");
			break;
		case GL_INVALID_OPERATION:
			fprintf(stderr, "GL_INVALID_OPERATION\n");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			fprintf(stderr, "GL_INVALID_FRAMEBUFFER_OPERATION\n");
			break;
		case GL_OUT_OF_MEMORY:
			fprintf(stderr, "GL_OUT_OF_MEMORY\n");
			break;
		case GL_STACK_UNDERFLOW:
			fprintf(stderr, "GL_STACK_UNDERFLOW\n");\
			break;
		case GL_STACK_OVERFLOW:
			fprintf(stderr, "GL_STACK_OVERFLOW\n");
			break;
		default:
			fprintf(stderr, "UNKNOWN glGetError value\n");
			break;
	}
}

#include <cstdlib>
static GLuint gl_blp_load_data(const BLPFile &file, uint32_t &width, uint32_t &height)
{
	const BLPHeader &header = file.header;
	GLuint id = 0;

	const char *const compressions[] = { "JPEG", "RAW1", "DXT", "RAW3" };
	printf("compression=%u (%s", header.compression, compressions[header.compression]);
	if (header.compression == 2)
	{
		const char *const dxt_types[] = { "DXT1 RGB", "DXT3", NULL, NULL, NULL, NULL, NULL, "DXT5" };
		printf(" alpha_type=%u (%s", header.alpha_type, dxt_types[header.alpha_type]);
		if (header.alpha_type == 0 && header.alpha_depth)
			printf("A");
		printf(")");
	}
	else
	{
		if (header.compression == 3)
//			printf(" flags=%b", header.alpha_depth);
			printf(" flags=%x", header.alpha_depth);
		else
			printf(" alpha_bits=%u", header.alpha_depth);
	}
//	printf(") mips=%b width=%u height=%u\n", header.has_mips, header.width, header.height);
	printf(") mips=%u width=%u height=%u\n", header.has_mips, header.width, header.height);

	if (header.type == 0)
	{
		printf("JPEG NOT SUPPORTED\n");
		return 0;	// JPG not supported, unused by WoW
	}

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	unsigned char l = 0;

	switch (header.compression)
	{
		case 1:
		{
			uint32_t *const buffer = (uint32_t *) malloc(header.width * header.height * 4);

			for (l = 0; header.mipmap_offsets[l]; l++)
			{
				uint32_t length;
				const uint8_t *const mipmap_data = (const uint8_t *) file.GetMipMap(l, width, height, length);
				size_t const size = width * height;

				for (unsigned int o = 0; o < size; o++)
					buffer[o] = header.palette[mipmap_data[o]] & 0xffffff | (
						header.alpha_depth ?
							(((mipmap_data[(size + o * header.alpha_depth / 8)] >> (o % 8 / header.alpha_depth)) & ((1 << header.alpha_depth) - 1))
							* 255 / ((1 << header.alpha_depth) - 1)) << 24 :
						0xff000000);

#if 1
				printf("MIPMAP level=%u width=%u height=%u size=%u length=%u", l, width, height, size, length);
#endif
				glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA, (GLsizei) width, (GLsizei) height, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
				if (glGetError() == GL_INVALID_VALUE)
				{
#if 1
					printf("alpha_bits=%u ERROR!\n", header.alpha_depth);
#endif
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
					break;
				}
#if 1
				printf("\n");
#endif
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, l);
			}
			free(buffer);
			break;
		}
		case 2:
		{
			GLenum format;

			switch (header.alpha_type)
			{
				case 0:		format = header.alpha_depth ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT; break;
				case 1:		format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
				case 7:		format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
				default:
#if 1
						printf("BLP UNKNOWN TYPE=%i\n", header.alpha_type);
#endif
						return 0;
			}

			for (l = 0; header.mipmap_offsets[l]; l++)
			{
				uint32_t length;
				const void *mipmap_data = file.GetMipMap(l, width, height, length);
				length = (header.alpha_type == 0 ? 8 : 16) * ((width + 3) >> 2) * ((height + 3) >> 2);
#if 1
				printf("MIPMAP level=%u width=%u height=%u length=%u", l, width, height, length);
#endif
				glCompressedTexImage2D(GL_TEXTURE_2D, l, format, (GLsizei) width, (GLsizei) height, 0, (GLsizei) length, mipmap_data);
				if (glGetError() == GL_INVALID_VALUE)
				{
#if 1
					printf(" alpha_type=%u alpha_bits=%u ERROR!\n", header.alpha_type, header.alpha_depth);
#endif
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
					break;
				}
#if 1
				printf("\n");
#endif
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, l);
			}
			break;
		}
		case 3:
		{
			for (l = 0; header.mipmap_offsets[l]; l++)
			{
				uint32_t length;
				const void *mipmap_data = file.GetMipMap(l, width, height, length);
#if 1
				printf("MIPMAP level=%u width=%u height=%u length=%u", l, width, height, length);
#endif
				glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA, (GLsizei) width, (GLsizei) height, 0, GL_BGRA, GL_UNSIGNED_BYTE, mipmap_data);
				// Incorrect; alpha_bits are flags
				// glTexImage2D(GL_TEXTURE_2D, l, header.alpha_depth ? GL_RGBA : GL_RGB, (GLsizei) width, (GLsizei) height, 0, header.alpha_depth ? GL_BGRA : GL_BGR, GL_UNSIGNED_BYTE, mipmap_data);
				if (glGetError() == GL_INVALID_VALUE)
				{
#if 1
					printf("alpha_type=%u alpha_bits=%u ERROR!\n", header.alpha_type, header.alpha_depth);
#endif
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
					break;
				}
#if 1
				printf("\n");
#endif
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, l);
			}
			break;
		}
		default:
#if 1
			printf("BLP UNKNOWN TYPE=%i\n", header.compression);
#endif
			return 0;
	}

	if (id)
	{
		// Default is GL_LINEAR
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if 1
//		printf("MipMapping mips=%b level=%u width=%u height=%u", header.has_mips, l - 1, width, height);
		printf("MipMapping mips=%x level=%u width=%u height=%u", header.has_mips, l - 1, width, height);
#endif
		// Default is GL_NEAREST_MIPMAP_LINEAR
		if (header.has_mips && l > 1 && width == 1 && height == 1)
		{
			GLint l;
			glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &l);
#if 1
			printf(" GL_TEXTURE_MAX_LEVEL=%i", l);
#endif
			if (l > 0)
			{
#if 1
				printf(" ENABLED\n");
#endif
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
#if 1
				printf(" DISABLED\n");
#endif
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
		}
		else
		{
#if 1
			printf(" DISABLED\n");
#endif
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		// Default is GL_REPEAT
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	return id;
}

static GLuint gl_create_shader(int shader_type, const char *source, void **error, GLuint *shader_out)
{
	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	int status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		int log_len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

		/*
		char *buffer = g_malloc(log_len + 1);
		glGetShaderInfoLog(shader, log_len, NULL, buffer);


		g_set_error(error, GLAREA_ERROR, GLAREA_ERROR_SHADER_COMPILATION,
								 "Compilation failure in %s shader: %s",
								 shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
								 buffer);

		g_free(buffer);
		*/
		glDeleteShader(shader);
		shader = 0;
	}

	if (shader_out != NULL)
		*shader_out = shader;

	return shader != 0;
}

//gl_Position = vec4(position, 0.0, 1.0);
const char *const vertex_shader_source = R"(#version 130

in vec2 position;

uniform mat3 mvp;

out vec2 texture_coordinate;

void main()
{
	gl_Position = vec4(mvp * vec3(position, 1.0), 1.0);
	texture_coordinate = position;
})";

const char *const fragment_shader_source = R"(#version 130

in vec2 texture_coordinate;

uniform sampler2D image;

out vec4 color;

void main()
{
	color = texture(image, texture_coordinate);
})";

void OpenGlResources::init()
{
	texture_alpha = true;

	float floats[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
	};

	glGenVertexArrays(1, &vao);
	glPrintError();
	glBindVertexArray(vao);
	glPrintError();

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glPrintError();
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glPrintError();
	unsigned const int sz = sizeof(float) * 2, num = 6;
	glBufferData(GL_ARRAY_BUFFER, sz * num, floats, GL_STATIC_DRAW);
	glPrintError();

	glEnableVertexAttribArray(0);
	glPrintError();
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sz, (GLvoid *)(0));
	glPrintError();

	GLuint vertex_shader;
	gl_create_shader(GL_VERTEX_SHADER, vertex_shader_source, nullptr, &vertex_shader);

	GLuint fragment_shader;
	gl_create_shader(GL_FRAGMENT_SHADER, fragment_shader_source, nullptr, &fragment_shader);
	
	/* link the vertex and fragment shaders together */
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	int status = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (status == GL_FALSE)
	{
		int log_len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);

		/*
		char *buffer = g_malloc(log_len + 1);
		glGetProgramInfoLog(program, log_len, NULL, buffer);

		g_set_error(error, GLAREA_ERROR, GLAREA_ERROR_SHADER_LINK,
					"Linking failure in program: %s", buffer);

		g_free(buffer);
		*/

		glDeleteProgram(program);
		program = 0;
	}

	mvp_location = glGetUniformLocation(program, "mvp");

#if 0
	/* get the location of the "position" attribute */
	position_location = glGetAttribLocation(program, "position");
#endif

	/* the individual shaders can be detached and destroyed */
	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);
/*
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
*/

//	glDeleteBuffers(1, &vbo);
}

#include <cstring>
void OpenGlResources::set_mvp(float *const mvp)
{
	std::memcpy(this->mvp, mvp, sizeof(float) * 9);
	mvp_changed = true;
}

void OpenGlResources::load_blp_data(void *const blp_data, uint32_t &width, uint32_t &height)
{
	BLPFile blp(blp_data);
	blp_texture_id = gl_blp_load_data(blp, width, height);
}

void OpenGlResources::render()
{
	glClearColor(0, 0, 0, 1.0);
	glPrintError();
	glClear(GL_COLOR_BUFFER_BIT);
	glPrintError();

	glEnable(GL_BLEND);
	glPrintError();
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(texture_alpha ? GL_SRC_ALPHA : GL_ONE, GL_DST_ALPHA);
	glPrintError();

	glEnable(GL_CULL_FACE);
	glPrintError();
	glEnable(GL_DEPTH_TEST);
	glPrintError();

	glUseProgram(program);
	glPrintError();

	if (mvp_changed)
	{
		glUniformMatrix3fv(mvp_location, 1, GL_FALSE, mvp);
		glPrintError();
		mvp_changed = false;
	}

	glBindVertexArray(vao);
	glPrintError();
	glBindTexture(GL_TEXTURE_2D, blp_texture_id);
	glPrintError();

	glDrawArrays(GL_TRIANGLES, 0, 6);
//	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glPrintError();

	glDisable(GL_BLEND);
	glPrintError();

	/* Flush the contents of the pipeline */
	glFlush();
	glPrintError();
}

void OpenGlResources::exit()
{
}
