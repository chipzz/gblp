#pragma once
#include <epoxy/gl.h>

class OpenGlResources
{
public:
	void init();
	void load_blp_data(void *const blp_data, uint32_t &width, uint32_t &height);
	void render();
	void exit();

	void set_texture_alpha(bool alpha) { texture_alpha = alpha; }

private:
	GLuint blp_texture_id;
	GLuint vao;
	GLuint program;
	GLuint mvp_location;

	bool texture_alpha;
};
