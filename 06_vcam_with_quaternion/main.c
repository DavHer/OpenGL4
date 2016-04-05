#include "gl_utils.h"
#include "maths_funcs.h"
#include <GL/glew.h> /* include GLEW and new version of GL on Windows */
#include <GLFW/glfw3.h> /* GLFW helper library */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "obj_parser.h"

#define MESH_FILE "sphere.obj"
#define VERTEX_SHADER_FILE "test_vs.glsl"
#define FRAGMENT_SHADER_FILE "test_fs.glsl"

#define NUM_SPHERES 4

/* create a unit quaternion q from an angle in degrees a, and an axis x,y,z */
void create_versor (float* q, float a, float x, float y, float z) {
    float rad = ONE_DEG_IN_RAD * a;
    q[0] = cosf (rad / 2.0f);
    q[1] = sinf (rad / 2.0f) * x;
    q[2] = sinf (rad / 2.0f) * y;
    q[3] = sinf (rad / 2.0f) * z;
}

/* convert a unit quaternion q to a 4x4 matrix m */
void quat_to_mat4 (float* m, float* q) {
    float w = q[0];
    float x = q[1];
    float y = q[2];
    float z = q[3];
    m[0] = 1.0f - 2.0f * y * y - 2.0f * z * z;
    m[1] = 2.0f * x * y + 2.0f * w * z;
    m[2] = 2.0f * x * z - 2.0f * w * y;
    m[3] = 0.0f;
    m[4] = 2.0f * x * y - 2.0f * w * z;
    m[5] = 1.0f - 2.0f * x * x - 2.0f * z * z;
    m[6] = 2.0f * y * z + 2.0f * w * x;
    m[7] = 0.0f;
    m[8] = 2.0f * x * z + 2.0f * w * y;
    m[9] = 2.0f * y * z - 2.0f * w * x;
    m[10] = 1.0f - 2.0f * x * x - 2.0f * y * y;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
}


/* normalise a quaternion in case it got a bit mangled */
void normalise_quat (float* q) {
    // norm(q) = q / magnitude (q)
    // magnitude (q) = sqrt (w*w + x*x...)
    // only compute sqrt if interior sum != 1.0
    float sum = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    // NB: floats have min 6 digits of precision
    const float thresh = 0.0001f;
    if (fabs (1.0f - sum) < thresh) {
        return;
    }
    float mag = sqrt (sum);
    for (int i = 0; i < 4; i++) {
        q[i] = q[i] / mag;
    }
}

/* multiply quaternions to get another one. result=R*S */
void mult_quat_quat (float* result, float* r, float* s) {
    result[0] = s[0] * r[0] - s[1] * r[1] -
        s[2] * r[2] - s[3] * r[3];
    result[1] = s[0] * r[1] + s[1] * r[0] -
        s[2] * r[3] + s[3] * r[2];
    result[2] = s[0] * r[2] + s[1] * r[3] +
        s[2] * r[0] - s[3] * r[1];
    result[3] = s[0] * r[3] - s[1] * r[2] +
        s[2] * r[1] + s[3] * r[0];
    // re-normalise in case of mangling
    normalise_quat (result);
}


// camera matrices. it's easier if they are global
mat4 view_mat;
mat4 proj_mat;
vec3 cam_pos (0.0f, 0.0f, 5.0f);
// a world position for each sphere in the scene
vec3 sphere_pos_wor[] = {
    vec3 (-2.0, 0.0, 0.0),
    vec3 (2.0, 0.0, 0.0),
    vec3 (-2.0, 0.0, -2.0),
    vec3 (1.5, 1.0, -1.0)
};



/* keep track of window size for things like the viewport and the mouse
 *  cursor */
int g_gl_width = 640;
int g_gl_height = 480;
GLFWwindow* g_window = NULL;

int main() {

	assert(restart_gl_log());
	assert(start_gl());

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//--------------Create Geometry-------------------
	GLfloat* vp = NULL;		// array of vertex points
	GLfloat* vn = NULL;		// array of vertex normals
	GLfloat* vt = NULL;		// array of texture coordinates
	int point_count;

	assert(load_obj_file(MESH_FILE, vp, vt, vn, point_count));

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint points_vbo;
	if(NULL != vp){
		glGenBuffers(1, &points_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
		glBufferData(GL_ARRAY_BUFFER,
				     3 * point_count * sizeof(GLfloat),
					 vp,
					 GL_STATIC_DRAW);
		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,NULL);
		glEnableVertexAttribArray(0);
	}

	//-----------------Create Shaders----------------*/
	char vertex_shader[1024 * 256];
	char fragmet_shader[1024 * 256];
	GLuint vs, fs, shader_programme;
	const GLchar* p;
	int params = -1;
	// Load shaders from files here
	assert(parse_file_into_str("test_vs.glsl", vertex_shader, 1024 * 256));
	assert(parse_file_into_str("test_fs.glsl", fragmet_shader, 1024 * 256));

	vs = glCreateShader(GL_VERTEX_SHADER);
	p = (const GLchar*) vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);

	// Check for shader compile errors
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
		_print_shader_info_log(vs);
		return 1; /* or exit or something */
	}

	fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar*) fragmet_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);

	// Check for shader compile errors
	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
		_print_shader_info_log(fs);
		return 1; /* or exit or something */
	}

	shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	/* check for shader linking errors - very important! */
	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf(
		stderr, "ERROR: could not link shader programme GL index %i\n",
				shader_programme);
		_print_programme_info_log(shader_programme);
		return 1;
	}

	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CW); // GL_CCW for counter clock-wise

	/*********************Create camera matrices******************************/
	/* create proyection matrix */
#define ONE_DEG_IN_RAD (2.0 * M_PI) / 360.0 // 0.017444444
	//input variables
	float near = 0.1f;   // clipping plane
	float far = 100.0f; // clipping plane
	float fov = 67.0f * ONE_DEG_IN_RAD;
	float aspect = (float) g_gl_width / (float) g_gl_height; //aspect ratio
	//matrix componenets
	float range = tan(fov * 0.5f) * near;
	float Sx = (2.0f * near) / (range * aspect + range * aspect);
	float Sy = near / range;
	float Sz = -(far + near) / (far - near);
	float Pz = -(2.0f * far * near) / (far - near);
	GLfloat proj_mat[] = { Sx, 0.0f, 0.0f, 0.0f, 0.0f, Sy, 0.0f, 0.0f, 0.0f,
			0.0f, Sz, -1.0f, 0.0f, 0.0f, Pz, 0.0f };

	/* create view matrix */
	float cam_speed = 1.0f; // 1 unit per second
	float cam_yaw_speed = 10.0f; // 10 degrees per second
	float cam_pos[] = { 0.0f, 0.0f, 2.0f }; // don't start at zero, or we will be too close
	float cam_yaw = 0.0f; // y-rotation in degrees
	mat4 T = translate(identity_mat4(),
			vec3(-cam_pos[0], -cam_pos[1], -cam_pos[2]));
	mat4 R = rotate_y_deg(identity_mat4(), -cam_yaw);
	mat4 view_mat = R * T;

	/* get location numbers of matrices in shader */
	GLint view_mat_location = glGetUniformLocation(shader_programme, "view");
	GLint proj_mat_location = glGetUniformLocation(shader_programme, "proj");
	/* use program (make current in state machine) and set default values */
	glUseProgram(shader_programme);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view_mat.m);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, proj_mat);

	while (!glfwWindowShouldClose(g_window)) {
		_update_fps_counter(g_window);
		// wipe the drawing  surface clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_gl_width, g_gl_height);

		// Animation
		static double previous_seconds = glfwGetTime();
		double current_seconds = glfwGetTime();
		double elapsed_seconds = current_seconds - previous_seconds;
		previous_seconds = current_seconds;

		glUseProgram(shader_programme);
		glBindVertexArray(vao);
		/* draw points 0-3 from the currently bound VAO with current in-use
		 shader */
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// Update events like input
		glfwPollEvents();

		/*-----------------------------move camera here-------------------------------*/
		// control keys
		bool cam_moved = false;
		if (glfwGetKey(g_window, GLFW_KEY_A)) {
			cam_pos[0] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_D)) {
			cam_pos[0] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_PAGE_UP)) {
			cam_pos[1] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_PAGE_DOWN)) {
			cam_pos[1] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_W)) {
			cam_pos[2] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_S)) {
			cam_pos[2] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_LEFT)) {
			cam_yaw += cam_yaw_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_RIGHT)) {
			cam_yaw -= cam_yaw_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(g_window, 1);
		}

		/* update view matrix */
		if (cam_moved) {
			mat4 T = translate(identity_mat4(),
					vec3(-cam_pos[0], -cam_pos[1], -cam_pos[2])); // cam translation
			mat4 R = rotate_y_deg(identity_mat4(), -cam_yaw); //
			mat4 view_mat = R * T;
			glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view_mat.m);
		}
		// put the stuff we ve been drawing onto the display
		glfwSwapBuffers(g_window);
	}

	// close GL context and any other GLFW resources
	glfwTerminate();

	return 0;

}
