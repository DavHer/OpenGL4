#include "gl_utils.h"
#include "maths_funcs.h"
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

	glGenBuffers(1, &colours_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), colours, GL_STATIC_DRAW);

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

/*********************Create camera matrices******************************/
	/* create proyection matrix */
	#define ONE_DEG_IN_RAD (2.0 * M_PI) / 360.0 // 0.017444444
	//input variables
	float near = 0.1f;   // clipping plane
	float far = 100.0f; // clipping plane
	float fov = 67.0f * ONE_DEG_IN_RAD;
	float aspect = (float)g_gl_width / (float)g_gl_height; //aspect ratio
	//matrix componenets
        float range = tan (fov * 0.5f) * near;                                  
        float Sx = (2.0f * near) / (range * aspect + range * aspect);           
        float Sy = near / range;                                                
        float Sz = -(far + near) / (far - near);                                
        float Pz = -(2.0f * far * near) / (far - near);                         
        GLfloat proj_mat[] = {                                                  
                Sx, 0.0f, 0.0f, 0.0f,                                           
                0.0f, Sy, 0.0f, 0.0f,                                           
                0.0f, 0.0f, Sz, -1.0f,                                          
                0.0f, 0.0f, Pz, 0.0f                                            
        };   

	/* create view matrix */
        float cam_speed = 1.0f; // 1 unit per second                            
        float cam_yaw_speed = 10.0f; // 10 degrees per second                   
        float cam_pos[] = {0.0f, 0.0f, 2.0f}; // don't start at zero, or we will be too close
        float cam_yaw = 0.0f; // y-rotation in degrees                          
        mat4 T = translate (identity_mat4 (), vec3 (-cam_pos[0], -cam_pos[1], -cam_pos[2]));
        mat4 R = rotate_y_deg (identity_mat4 (), -cam_yaw);                     
        mat4 view_mat = R * T; 
	
	/* get location numbers of matrices in shader */
	GLint view_mat_location = glGetUniformLocation(shader_programme, "view");
	GLint proj_mat_location = glGetUniformLocation(shader_programme, "proj");
	/* use program (make current in state machine) and set default values */
        glUseProgram (shader_programme);                                        
        glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view_mat.m);        
        glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, proj_mat);          


	while(!glfwWindowShouldClose(g_window)){
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
	    if (glfwGetKey (g_window, GLFW_KEY_A)) {                        
	        cam_pos[0] -= cam_speed * elapsed_seconds;              
		cam_moved = true;                                       
	    }                                                               
	    if (glfwGetKey (g_window, GLFW_KEY_D)) {                        
		cam_pos[0] += cam_speed * elapsed_seconds;              
		cam_moved = true;                                       
	    }                                                               
	    if (glfwGetKey (g_window, GLFW_KEY_PAGE_UP)) {                  
		cam_pos[1] += cam_speed * elapsed_seconds;              
		cam_moved = true;                                       
	    }                                                               
	    if (glfwGetKey (g_window, GLFW_KEY_PAGE_DOWN)) {                
		cam_pos[1] -= cam_speed * elapsed_seconds;              
	        cam_moved = true;                                       
            }                                                               
            if (glfwGetKey (g_window, GLFW_KEY_W)) {                        
                cam_pos[2] -= cam_speed * elapsed_seconds;              
                cam_moved = true;                                       
            }                                                               
            if (glfwGetKey (g_window, GLFW_KEY_S)) {                        
                cam_pos[2] += cam_speed * elapsed_seconds;              
                cam_moved = true;                                       
            }                                                               
            if (glfwGetKey (g_window, GLFW_KEY_LEFT)) {                     
                 cam_yaw += cam_yaw_speed * elapsed_seconds;             
                 cam_moved = true;                                       
            }                                                               
            if (glfwGetKey (g_window, GLFW_KEY_RIGHT)) {                    
                 cam_yaw -= cam_yaw_speed * elapsed_seconds;             
                 cam_moved = true;                                       
            } 
	    if(GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE)){
		    glfwSetWindowShouldClose(g_window, 1);
	    }

	    /* update view matrix */
	    if(cam_moved){
	        mat4 T = translate (identity_mat4 (), vec3 (-cam_pos[0], -cam_pos[1], -cam_pos[2])); // cam translation
	 	mat4 R = rotate_y_deg (identity_mat4 (), -cam_yaw); //  
		mat4 view_mat = R * T;                                  
	        glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view_mat.m);
	    }
	    // put the stuff we ve been drawing onto the display
	    glfwSwapBuffers(g_window);
	}

	// close GL context and any other GLFW resources
	glfwTerminate();

	return 0;

}
