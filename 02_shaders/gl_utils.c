#include "gl_utils.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#define GL_LOG_FILE "gl.log"
#define MAX_SHADER_LENGTH 262144

/*--------------------------------LOG FUNCTIONS------------------------------*/
bool restart_gl_log () {
	time_t now;
	char* date;
	FILE* file = fopen (GL_LOG_FILE, "w");

	if (!file) {
		fprintf (
				stderr,
				"ERROR: could not open GL_LOG_FILE log file %s for writing\n",
				GL_LOG_FILE
				);
		return false;
	}
	now = time (NULL);
	date = ctime (&now);
	fprintf (file, "GL_LOG_FILE log. local time %s\n", date);
	fclose (file);
	return true;
}

bool gl_log (const char* message, ...) {
	va_list argptr;
	FILE* file = fopen (GL_LOG_FILE, "a");
	if (!file) {
		fprintf (
				stderr,
				"ERROR: could not open GL_LOG_FILE %s file for appending\n",
				GL_LOG_FILE
				);
		return false;
	}
	va_start (argptr, message);
	vfprintf (file, message, argptr);
	va_end (argptr);
	fclose (file);
	return true;
}

/* same as gl_log except also prints to stderr */
bool gl_log_err (const char* message, ...) {
	va_list argptr;
	FILE* file = fopen (GL_LOG_FILE, "a");
	if (!file) {
		fprintf (
				stderr,
				"ERROR: could not open GL_LOG_FILE %s file for appending\n",
				GL_LOG_FILE
				);
		return false;
	}
	va_start (argptr, message);
	vfprintf (file, message, argptr);
	va_end (argptr);
	va_start (argptr, message);
	vfprintf (stderr, message, argptr);
	va_end (argptr);
	fclose (file);
	return true;
}

/* ---------------------------------GLFW3 and GLEW ----------------*/
bool start_gl(){
	const GLubyte* renderer;
	const GLubyte* version;

	gl_log("starting GLFW %s", glfwGetVersionString());
	glfwSetErrorCallback(glfw_error_callback);
	if(!glfwInit()){
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	g_window = glfwCreateWindow(
			g_gl_width, g_gl_height, "Extended Init.", NULL, NULL
			);	

	if(!g_window){
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(g_window, glfw_window_size_callback);
	glfwMakeContextCurrent(g_window);

	glfwWindowHint(GLFW_SAMPLES, 4);
	glewExperimental = GL_TRUE;
	glewInit();

	return true;
}

void glfw_error_callback(int error, const char* description){
	fputs(description, stderr);
	gl_log_err("%s\n", description);
}

void glfw_window_size_callback(GLFWwindow* window, int width, int height){
	g_gl_width = width;
	g_gl_height = height;
	printf("width %i height %i\n", width, height);
}

double previous_seconds;
int frame_count;

void _update_fps_counter(GLFWwindow* window){
	double current_seconds;
	double elapsed_seconds;

	previous_seconds = glfwGetTime();
	current_seconds = glfwGetTime (); 
	elapsed_seconds = current_seconds - previous_seconds;
	if (elapsed_seconds > 0.25) {
		double fps;
		char tmp[128];

		previous_seconds = current_seconds;
		fps = (double)frame_count / elapsed_seconds;
		sprintf (tmp, "opengl @ fps: %.2f", fps);
		glfwSetWindowTitle (window, tmp);
		frame_count = 0;
	}   
	frame_count++;
}





