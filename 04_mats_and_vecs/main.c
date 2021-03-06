#include "gl_utils.h"
#include <GL/glew.h> /* include GLEW and new version of GL on Windows */
#include <GLFW/glfw3.h> /* GLFW helper library */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

/* keep track of window size for things like the viewport and the mouse
 *  cursor */
int g_gl_width = 640;
int g_gl_height = 480;
GLFWwindow* g_window = NULL;


int main(){

	assert(restart_gl_log());
	assert(start_gl());

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	GLfloat points[] = {
			 0.0f,	0.5f,	0.0f,
			 0.5f, -0.5f,	0.0f,
			-0.5f, -0.5f,	0.0f
	};

	GLfloat colours[] = {
			1.0f, 0.0f,  0.0f,
			0.0f, 1.0f,  0.0f,
			0.0f, 0.0f,  1.0f
	};

	float matrix[] = {
		1.0f, 0.0f, 0.0f, 0.0f, // first column
		0.0f, 1.0f, 0.0f, 0.0f, // first column
		0.0f, 0.0f, 1.0f, 0.0f, // first column
		0.5f, 0.0f, 0.0f, 1.0f, // first column
	};

	GLuint vao;
	char vertex_shader[1024 * 256];
	char fragmet_shader[1024 * 256];
	GLuint vs, fs, shader_programme;
	const GLchar* p;
	int params = -1;

	GLuint points_vbo;
	GLuint colours_vbo;

	glGenBuffers(1, &points_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), points, GL_STATIC_DRAW);

	/* create a second VBO, containing the array of colours.
	note that we could also put them both into a single vertex buffer. in this
	case we would use the pointer and stride parameters of glVertexAttribPointer()
	to describe the different data layouts */
	glGenBuffers(1, &colours_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), colours, GL_STATIC_DRAW);

	/* create the VAO.
	we bind each VBO in turn, and call glVertexAttribPointer() to indicate where
	the memory should be fetched for vertex shader input variables 0, and 1,
	respectively. we also have to explicitly enable both 'attribute' variables.
	'attribute' is the older name for vertex shader 'in' variables. */
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	// Load shaders from files here
	assert(parse_file_into_str("test_vs.glsl", vertex_shader, 1024*256));
	assert(parse_file_into_str("test_fs.glsl", fragmet_shader, 1024*256));

	vs = glCreateShader(GL_VERTEX_SHADER);
	p = (const GLchar*)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);

	// Check for shader compile errors
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if(GL_TRUE != params){
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", vs);
		_print_shader_info_log (vs);
		return 1; /* or exit or something */
	}

	fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar*)fragmet_shader;
	glShaderSource (fs, 1, &p, NULL);
	glCompileShader (fs);

	// Check for shader compile errors
	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if(GL_TRUE != params){
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", fs);
		_print_shader_info_log (fs);
		return 1; /* or exit or something */
	}

    shader_programme = glCreateProgram();
    glAttachShader(shader_programme, fs);
    glAttachShader(shader_programme, vs);
    glLinkProgram(shader_programme);

    /* check for shader linking errors - very important! */
	glGetProgramiv (shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (
			stderr,
			"ERROR: could not link shader programme GL index %i\n",
			shader_programme
		);
		_print_programme_info_log (shader_programme);
		return 1;
	}

	glEnable (GL_CULL_FACE); // cull face
	glCullFace (GL_BACK); // cull back face
	glFrontFace (GL_CW); // GL_CCW for counter clock-wise

	int matrix_location = glGetUniformLocation(shader_programme,
			                                   "matrix");
	glUseProgram(shader_programme);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, matrix);

	float speed = 1.0f;
	float last_position = 0.0f;

	while(!glfwWindowShouldClose(g_window)){
		_update_fps_counter(g_window);
		// wipe the drawing  surface clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_gl_width, g_gl_height);

		// Animation
	    static double previous_seconds = glfwGetTime();
	    double current_seconds = glfwGetTime();
	    double elpased_seconds = current_seconds - previous_seconds;
	    previous_seconds = current_seconds;

	    // reverse direction when going to far left or right
	    if(fabs(last_position) > 1.0f){
	    	speed = -speed;
	    }

	    // Update the matrix
	    matrix[12] = elpased_seconds * speed + last_position;
	    last_position = matrix[12];
		glUseProgram(shader_programme);
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, matrix);

		glUseProgram(shader_programme);
		glBindVertexArray(vao);

		/* draw points 0-3 from the currently bound VAO with current in-use
         shader */
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// Update events like input
		glfwPollEvents();
		if(GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE)){
			glfwSetWindowShouldClose(g_window, 1);
		}
		// put the stuff we ve been drawing onto the display
		glfwSwapBuffers(g_window);
	}

	// close GL context and any other GLFW resources
	glfwTerminate();

	return 0;

}
