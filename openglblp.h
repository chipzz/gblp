#pragma once
#include <epoxy/gl.h>

class OpenGlResources
{
public:
	void init();
	void load_blp(const char *const blp_filename);
	void load_blp_data(void *const blp_data);
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
