//---------------------------------------------------------------------------
//
// Copyright (c) 2016 Taehyun Rhee, Joshua Scott, Ben Allen
//
// This software is provided 'as-is' for assignment of COMP308 in ECS,
// Victoria University of Wellington, without any express or implied warranty. 
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// The contents of this file may not be copied or duplicated in any form
// without the prior permission of its owner.
//
//----------------------------------------------------------------------------

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <stdexcept>

#include "cgra_geometry.hpp"
#include "cgra_math.hpp"
#include "simple_image.hpp"
#include "simple_shader.hpp"
#include "opengl.hpp"

using namespace std;
using namespace cgra;

// Window
//
GLFWwindow* g_window;


struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

// Projection values
// 
float g_fovy = 20.0;
float g_znear = 0.1;
float g_zfar = 1000.0;


// Mouse controlled Camera values
//
bool g_leftMouseDown = false;
vec2 g_mousePosition;
float g_pitch = 0;
float g_yaw = 0;
float g_zoom = 1.0;

// Values and fields to showcase the use of shaders
// Remove when modifying main.cpp for Assignment 3
//
bool g_useShader = false;
GLuint g_texture = 0;
GLuint g_texture1 = 1;
GLuint textures[2];
GLuint g_shader = 0;


struct vertex {
	int p = 0; // index for point in m_points
	int t = 0; // index for uv in m_uvs
	int n = 0; // index for normal in m_normals
};

struct triangle {
	vertex v[3]; //requires 3 verticies
	int n = 0; // index for per face normals in m_trianglesNormals
};

// Feilds for storing raw obj information
std::string m_filename;
std::vector<std::vector<cgra::vec3>> v_m_points;	// Point list
std::vector<std::vector<cgra::vec2>> v_m_uvs;		// Texture Coordinate list
std::vector<std::vector<cgra::vec3>> v_m_normals;	// Normal list
std::vector<std::vector<triangle>> v_m_triangles;	// Triangle/Face list
std::vector<std::vector<cgra::vec3>> m_triangleNormals; // Per face normal list


// Mouse Button callback
// Called for mouse movement event on since the last glfwPollEvents
//
void cursorPosCallback(GLFWwindow* win, double xpos, double ypos) {
	// cout << "Mouse Movement Callback :: xpos=" << xpos << "ypos=" << ypos << endl;
	if (g_leftMouseDown) {
		g_yaw -= g_mousePosition.x - xpos;
		g_pitch -= g_mousePosition.y - ypos;
	}
	g_mousePosition = vec2(xpos, ypos);
}


// Mouse Button callback
// Called for mouse button event on since the last glfwPollEvents
//
void mouseButtonCallback(GLFWwindow *win, int button, int action, int mods) {
	// cout << "Mouse Button Callback :: button=" << button << "action=" << action << "mods=" << mods << endl;
	if (button == GLFW_MOUSE_BUTTON_LEFT)
		g_leftMouseDown = (action == GLFW_PRESS);
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		if (g_useShader) {
			g_useShader = false;
			cout << "Using the default OpenGL pipeline" << endl;
		}
		else {
			g_useShader = true;
			cout << "Using a shader" << endl;
		}
	}
}


// Scroll callback
// Called for scroll event on since the last glfwPollEvents
//
void scrollCallback(GLFWwindow *win, double xoffset, double yoffset) {
	// cout << "Scroll Callback :: xoffset=" << xoffset << "yoffset=" << yoffset << endl;
	g_zoom -= yoffset * g_zoom * 0.2;
}


// Keyboard callback
// Called for every key event on since the last glfwPollEvents
//
void keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods) {
	// cout << "Key Callback :: key=" << key << "scancode=" << scancode
	// 	<< "action=" << action << "mods=" << mods << endl;
	// YOUR CODE GOES HERE
	// ...
}


// Character callback
// Called for every character input event on since the last glfwPollEvents
//
void charCallback(GLFWwindow *win, unsigned int c) {
	// cout << "Char Callback :: c=" << char(c) << endl;
	// Not needed for this assignment, but useful to have later on
}


// Sets up where and what the light is
// Called once on start up
// 
void initLight() {
	float dir_pos[] = { 2.0f, 1.0f, 5.0f, 0.0f };
	float dir_int[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	//float ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };

	glLightfv(GL_LIGHT0, GL_POSITION, dir_pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, dir_int);
	//glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glEnable(GL_LIGHT0);

	float spot_pos[] = { 0.0f, 5.0f, 0.0f, 0.0f };
	float spot_dir[] = { 0.0f, -1.0f, 0.0f };
	float spot_int[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	//float ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };

	glLightfv(GL_LIGHT1, GL_POSITION, spot_pos);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, spot_int);
	//glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glEnable(GL_LIGHT1);
}

/*void loadTexture(GLuint texture, const char* filename)
{
	Image tex(filename);

	glActiveTexture(GL_TEXTURE0); // Use slot 0, need to use GL_TEXTURE1 ... etc if using more than one texture PER OBJECT
	glGenTextures(1, &texture); // Generate texture ID
	glBindTexture(GL_TEXTURE_2D, texture); // Bind it as a 2D texture

	// Setup sampling strategies
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Finnaly, actually fill the data into our texture
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, tex.w, tex.h, tex.glFormat(), GL_UNSIGNED_BYTE, tex.dataPointer());
}*/

GLuint loadTexture(const char* file)
{
    GLuint result;
    Image img(file);
  
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, img.w, img.h, img.glFormat(), GL_UNSIGNED_BYTE, img.dataPointer());
    
    return result;
}



// An example of how to load a texure from a hardcoded location
//
void initTexture() {


	glGenTextures(1, &g_texture); // Generate texture ID


	Image tex0("./work/res/textures/wood.jpg");

	glActiveTexture(GL_TEXTURE0); // Use slot 0, need to use GL_TEXTURE1 ... etc if using more than one texture PER OBJECT
	glBindTexture(GL_TEXTURE_2D, g_texture); // Bind it as a 2D texture

											   // Setup sampling strategies
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Finnaly, actually fill the data into our texture
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, tex0.w, tex0.h, tex0.glFormat(), GL_UNSIGNED_BYTE, tex0.dataPointer());



	//textures = new GLuint[2];
	//glGenTextures(2, textures);

	//loadTexture(textures[0], "./work/res/textures/wood.jpg");
	//loadTexture(textures[1], "./work/res/textures/brick.jpg");


}


// An example of how to load a shader from a hardcoded location
//
void initShader() {
	// To create a shader program we use a helper function
	// We pass it an array of the types of shaders we want to compile
	// and the corrosponding locations for the files of each stage
	g_shader = makeShaderProgramFromFile({GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "./work/res/shaders/shaderDemo.vert", "./work/res/shaders/shaderDemo.frag" });
}


// Sets up where the camera is in the scene
// 
void setupCamera(int width, int height) {
	// Set up the projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(g_fovy, width / float(height), g_znear, g_zfar);

	// Set up the view part of the model view matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0, 0, -50 * g_zoom);
	glRotatef(g_pitch, 1, 0, 0);
	glRotatef(g_yaw, 0, 1, 0);
}


void renderGEOM(std::vector<cgra::vec3> m_points, std::vector<cgra::vec2> m_uvs, std::vector<cgra::vec3> m_normals, std::vector<triangle> m_triangles) {

	for (int tri = 0; tri < m_triangles.size(); tri++) {
		for (int vtri = 0; vtri < 3; vtri++) {
			glNormal3f(m_normals[m_triangles[tri].v[vtri].n].x, m_normals[m_triangles[tri].v[vtri].n].y, m_normals[m_triangles[tri].v[vtri].n].z);
			glTexCoord2f(10*m_uvs[m_triangles[tri].v[vtri].t].x, 10*m_uvs[m_triangles[tri].v[vtri].t].y);
			glVertex3f(m_points[m_triangles[tri].v[vtri].p].x, m_points[m_triangles[tri].v[vtri].p].y, m_points[m_triangles[tri].v[vtri].p].z);

		}
	}

	glEnd();
}

void createNormals(std::vector<cgra::vec3>& m_points, std::vector<cgra::vec3>&  m_normals, std::vector<triangle>&  m_triangles) {


	// first set all normals to (0, 0, 0) so we can add to each of them later on

	int nIndex = 0;
	for (int tri = 0; tri < m_triangles.size(); tri++) {
		for (int vec = 0; vec < 3; vec++) {
			m_normals.push_back(vec3(0, 0, 0));
			m_triangles[tri].v[vec].n = nIndex;
			nIndex++;
		}
	}

	for (int tri = 0; tri < m_triangles.size(); tri++) {

		vec3 u = m_points[m_triangles[tri].v[1].p] - m_points[m_triangles[tri].v[0].p];
		vec3 v = m_points[m_triangles[tri].v[2].p] - m_points[m_triangles[tri].v[0].p];
		// add face normals to the current vector normal, as each normal is
		// already stored the new value is the sum of all encountered adjacent
		// face normals.
		for (int vec = 0; vec < 3; vec++) {
			m_normals[m_triangles[tri].v[vec].n] = (m_normals[m_triangles[tri].v[vec].n] + cross(u, v));
		}
	}

	// have to go through and normalize each vectors normal after
	// summing contribution from each face so that normalized values
	// aren't contributing to the sum.
	for (int tri = 0; tri < m_triangles.size(); tri++) {
		for (int vec = 0; vec < 3; vec++) {
			m_normals[m_triangles[tri].v[vec].n] = normalize(m_normals[m_triangles[tri].v[vec].n]);
		}
	}
}


void readOBJ(std::vector<std::vector<cgra::vec3>>* v_m_points, std::vector<std::vector<cgra::vec2>>* v_m_uvs, std::vector<std::vector<cgra::vec3>>* v_m_normals, std::vector<std::vector<triangle>>* v_m_triangles) {

	string filename = m_filename;

	std::vector<cgra::vec3> m_points = { vec3(0,0,0) };
	std::vector<cgra::vec2> m_uvs = { vec2(0,0) };
	std::vector<cgra::vec3> m_normals = { vec3(0,0,1) };
	std::vector<triangle> m_triangles;	// Triangle/Face list
	std::vector<cgra::vec3> m_triangleNormals;

	ifstream objFile(filename);

	cout << filename << endl;

	if (!objFile.is_open()) {
		cerr << "Error reading " << filename << endl;
		throw runtime_error("Error :: could not open file.");
	}

	cout << "Reading file " << filename << endl;

	// good() means that failbit, badbit and eofbit are all not set
	while (objFile.good()) {

		// Pull out line from file
		string line;
		std::getline(objFile, line);
		istringstream objLine(line);

		// Pull out mode from line
		string mode;
		objLine >> mode;

		// Reading like this means whitespace at the start of the line is fine
		// attempting to read from an empty string/line will set the failbit
		if (!objLine.fail()) {

			if (mode == "v") {
				vec3 v;
				objLine >> v.x >> v.y >> v.z;
				m_points.push_back(v);

			}
			else if (mode == "vn") {
				vec3 vn;
				objLine >> vn.x >> vn.y >> vn.z;
				m_normals.push_back(vn);

			}
			else if (mode == "vt") {
				vec2 vt;
				objLine >> vt.x >> vt.y;
				m_uvs.push_back(vt);

			}
			else if (mode == "f") {

				vector<vertex> verts;
				while (objLine.good()) {
					vertex v;

					//-------------------------------------------------------------
					// [Assignment 1] :
					// Modify the following to parse the bunny.obj. It has no uv
					// coordinates so each vertex for each face is in the format
					// v//vn instead of the usual v/vt/vn.
					//
					// Modify the following to parse the dragon.obj. It has no
					// normals or uv coordinates so the format for each vertex is
					// v instead of v/vt/vn or v//vn.
					//
					// Hint : Check if there is more than one uv or normal in
					// the uv or normal vector and then parse appropriately.
					//-------------------------------------------------------------

					if (m_uvs.size() > 1) {

						// Assignment code (assumes you have all of v/vt/vn for each vertex)
						objLine >> v.p;		// Scan in position index
						objLine.ignore(1);	// Ignore the '/' character
						objLine >> v.t;		// Scan in uv (texture coord) index
						objLine.ignore(1);	// Ignore the '/' character
						objLine >> v.n;		// Scan in normal index
					}
					else if (m_normals.size() > 1) {
						objLine >> v.p;
						objLine.ignore(2);
						objLine >> v.n;
					}
					else {
						objLine >> v.p;

					}
					verts.push_back(v);
				}
				cout << verts.size() << endl;

				// IFF we have 3 verticies, construct a triangle
				if (verts.size() >= 3) {
					triangle tri;
					tri.v[0] = verts[0];
					tri.v[1] = verts[1];
					tri.v[2] = verts[2];
					cout << "Making a triangle" << endl;
					m_triangles.push_back(tri);

				}
			}
		}
	}

	v_m_points->push_back(m_points);
	v_m_normals->push_back(m_normals);
	v_m_triangles->push_back(m_triangles);
	v_m_uvs->push_back(m_uvs);

	cout << "Reading OBJ file is DONE." << endl;
	cout << m_points.size() - 1 << " points" << endl;
	cout << m_uvs.size() - 1 << " uv coords" << endl;
	cout << m_normals.size() - 1 << " normals" << endl;
	cout << m_triangles.size() << " faces" << endl;


	// If we didn't have any normals, create them
	if (m_normals.size() <= 1) {
		createNormals(m_points, m_normals, m_triangles);
		cout << "creating norms";
	}
}


// Draw function
//
void render(int width, int height) {

	// Grey/Blueish background
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// Enable flags for normal rendering
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);

	setupCamera(width, height);


	initLight();


	// Without shaders
	// Uses the default OpenGL pipeline
	//
	if (!g_useShader) {

		// Texture setup
		//

		// Render a single square as our geometry
		// You would normally render your geometry here

		//glBegin(GL_QUADS);
		//glNormal3f(0.0, 0.0, 1.0);
		//glTexCoord2f(0.0, 0.0);
		//glVertex3f(-5.0, -5.0, 0.0);
		//glTexCoord2f(0.0, 1.0);
		//glVertex3f(-5.0, 5.0, 0.0);
		//glTexCoord2f(1.0, 1.0);
		//glVertex3f(5.0, 5.0, 0.0);
		//glTexCoord2f(1.0, 0.0);
		//glVertex3f(5.0, -5.0, 0.0);
		//glEnd();


		//table
		// Enable Drawing texures
		glEnable(GL_TEXTURE_2D);
		// Use Texture as the color
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		// Set the location for binding the texture
		glActiveTexture(GL_TEXTURE0);
		// Bind the texture
		glBindTexture(GL_TEXTURE_2D, textures[0]);

			glBegin(GL_TRIANGLES);

		renderGEOM(v_m_points[0], v_m_uvs[0], v_m_normals[0], v_m_triangles[0]);

		glDisable(GL_TEXTURE_2D);


		// teapot
		glPushMatrix(); {
			glTranslatef(-4, 0.5, -4);



			glBegin(GL_TRIANGLES);

			float ambient[] = { 0.15,0.1,0.3 ,1.0 };
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			float diffuse[] = { 0.15,0.1,0.3 ,1.0 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			float specular[] = { 0.3,0.3,0.3, 1.0 };
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			float shininess[] = { 0.10*128.0 };
			glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

			renderGEOM(v_m_points[1], v_m_uvs[1], v_m_normals[1], v_m_triangles[1]);
		} glPopMatrix();

		// torus
		glPushMatrix(); {

			glTranslatef(4, 1, 5);



			glBegin(GL_TRIANGLES);

			float ambient[] = { 0.8,0.0,0.0 ,1.0};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			float diffuse[] = { 0.8,0.0,0.0 ,1.0};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			float specular[] = { 0.9,0.9,0.9, 1.0};
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			float shininess[] = { 0.25*128.0 };
			glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

		renderGEOM(v_m_points[2], v_m_uvs[2], v_m_normals[2], v_m_triangles[2]);
		} glPopMatrix();


		// sphere
		glPushMatrix(); {

		glTranslatef(-6, 2, 3);
		glBegin(GL_TRIANGLES);

			float ambient[] = { 0.8,0.8,0.0 ,1.0};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			float diffuse[] = { 0.8,0.8,0.0 ,1.0};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			float specular[] = { 0.5,0.5,0.5, 1.0};
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			float shininess[] = { 0.50*128.0 };
			glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

		renderGEOM(v_m_points[3], v_m_uvs[3], v_m_normals[3], v_m_triangles[3]);

		} glPopMatrix();

		// box

		glEnable(GL_TEXTURE_2D);
		// Use Texture as the color
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		// Set the location for binding the texture
		glActiveTexture(GL_TEXTURE0);
		// Bind the texture
		glBindTexture(GL_TEXTURE_2D, textures[1]);

		glPushMatrix(); {

			glScalef(0.8, 0.8, 0.8);
			glTranslatef(6, 2.5, -5);
			glBegin(GL_TRIANGLES);

			float diffuse[] = { 1.0,1.0,1.0 ,1.0};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);

			renderGEOM(v_m_points[4], v_m_uvs[4], v_m_normals[4], v_m_triangles[4]);

		} glPopMatrix();

		glDisable(GL_TEXTURE_2D);


		glFlush();
	}


	// With shaders (no lighting)
	// Uses the shaders that you bind for the graphics pipeline
	//
	else {

		// Texture setup
		//
		// Enable Drawing texures
		glEnable(GL_TEXTURE_2D);
		// Set the location for binding the texture
		glActiveTexture(GL_TEXTURE0);
		// Bind the texture
		glBindTexture(GL_TEXTURE_2D, g_texture);

		// Use the shader we made
		glUseProgram(g_shader);

		// Set our sampler (texture0) to use GL_TEXTURE0 as the source
		glUniform1i(glGetUniformLocation(g_shader, "texture0"), 0);


		// Render a single square as our geometry
		// You would normally render your geometry here
		//glBegin(GL_QUADS);
		//glNormal3f(0.0, 0.0, 1.0);
		//glTexCoord2f(0.0, 0.0);
		//glVertex3f(-5.0, -5.0, 0.0);
		//glTexCoord2f(0.0, 1.0);
		//glVertex3f(-5.0, 5.0, 0.0);
		//glTexCoord2f(1.0, 1.0);
		//glVertex3f(5.0, 5.0, 0.0);
		//glTexCoord2f(1.0, 0.0);
		//glVertex3f(5.0, -5.0, 0.0);
		//glEnd();
		//glFlush();

		renderGEOM(v_m_points[0], v_m_uvs[0], v_m_normals[0], v_m_triangles[0]);

		glPushMatrix(); {
			glTranslatef(-4, 0.5, -4);

			renderGEOM(v_m_points[1], v_m_uvs[1], v_m_normals[1], v_m_triangles[1]);
		} glPopMatrix();

		glPushMatrix(); {

			glTranslatef(4, 1, 5);

			renderGEOM(v_m_points[2], v_m_uvs[2], v_m_normals[2], v_m_triangles[2]);
		} glPopMatrix();


		glPushMatrix(); {

			glTranslatef(-6, 2, 3);

			renderGEOM(v_m_points[3], v_m_uvs[3], v_m_normals[3], v_m_triangles[3]);

		} glPopMatrix();

		glPushMatrix(); {

			glScalef(0.8, 0.8, 0.8);
			glTranslatef(6, 2.5, -5);

			renderGEOM(v_m_points[4], v_m_uvs[4], v_m_normals[4], v_m_triangles[4]);

		} glPopMatrix();

		// Unbind our shader
		glUseProgram(0);
	}


	// Disable flags for cleanup (optional)
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_NORMALIZE);
}


// Forward decleration for cleanliness (Ignore)
void APIENTRY debugCallbackARB(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, GLvoid*);


//Main program
// 
int main(int argc, char **argv) {

	// Initialize the GLFW library
	if (!glfwInit()) {
		cerr << "Error: Could not initialize GLFW" << endl;
		abort(); // Unrecoverable error
	}

	// Get the version for GLFW for later
	int glfwMajor, glfwMinor, glfwRevision;
	glfwGetVersion(&glfwMajor, &glfwMinor, &glfwRevision);

	// Create a windowed mode window and its OpenGL context
	g_window = glfwCreateWindow(640, 480, "Hello World", nullptr, nullptr);
	if (!g_window) {
		cerr << "Error: Could not create GLFW window" << endl;
		abort(); // Unrecoverable error
	}

	// Make the g_window's context is current.
	// If we have multiple windows we will need to switch contexts
	glfwMakeContextCurrent(g_window);



	// Initialize GLEW
	// must be done after making a GL context current (glfwMakeContextCurrent in this case)
	glewExperimental = GL_TRUE; // required for full GLEW functionality for OpenGL 3.0+
	GLenum err = glewInit();
	if (GLEW_OK != err) { // Problem: glewInit failed, something is seriously wrong.
		cerr << "Error: " << glewGetErrorString(err) << endl;
		abort(); // Unrecoverable error
	}



	// Print out our OpenGL verisions
	cout << "Using OpenGL " << glGetString(GL_VERSION) << endl;
	cout << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;
	cout << "Using GLFW " << glfwMajor << "." << glfwMinor << "." << glfwRevision << endl;



	// Attach input callbacks to g_window
	glfwSetCursorPosCallback(g_window, cursorPosCallback);
	glfwSetMouseButtonCallback(g_window, mouseButtonCallback);
	glfwSetScrollCallback(g_window, scrollCallback);
	glfwSetKeyCallback(g_window, keyCallback);
	glfwSetCharCallback(g_window, charCallback);



	// Enable GL_ARB_debug_output if available. Not nessesary, just helpful
	if (glfwExtensionSupported("GL_ARB_debug_output")) {
		// This allows the error location to be determined from a stacktrace
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		// Set the up callback
		glDebugMessageCallbackARB(debugCallbackARB, nullptr);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
		cout << "GL_ARB_debug_output callback installed" << endl;
	}
	else {
		cout << "GL_ARB_debug_output not available. No worries." << endl;
	}

	m_filename = "work/res/assets/table.obj";
	readOBJ(&v_m_points, &v_m_uvs, &v_m_normals, &v_m_triangles);


	m_filename = "work/res/assets/teapot.obj";
	readOBJ(&v_m_points, &v_m_uvs, &v_m_normals, &v_m_triangles);

	m_filename = "work/res/assets/torus.obj";
	readOBJ(&v_m_points, &v_m_uvs, &v_m_normals, &v_m_triangles);

	m_filename = "work/res/assets/sphere.obj";
	readOBJ(&v_m_points, &v_m_uvs, &v_m_normals, &v_m_triangles);

	m_filename = "work/res/assets/box.obj";
	readOBJ(&v_m_points, &v_m_uvs, &v_m_normals, &v_m_triangles);


	//cout << m_triangles.size() << endl;


	// Initialize Geometry/Material/Lights
	// YOUR CODE GOES HERE
	// ...
	textures[0] = loadTexture("./work/res/textures/wood.jpg");
	textures[1] = loadTexture("./work/res/textures/brick.jpg");

	//loadTexture(g_texture, "work/res/textures/wood.jpg");
	//loadTexture(g_texture1, "work/res/textures/brick.jpg");
	initTexture();
	initShader();

	// Loop until the user closes the window
	while (!glfwWindowShouldClose(g_window)) {

		// Make sure we draw to the WHOLE window
		int width, height;
		glfwGetFramebufferSize(g_window, &width, &height);

		// Main Render
		render(width, height);

		// Swap front and back buffers
		glfwSwapBuffers(g_window);

		// Poll for and process events
		glfwPollEvents();
	}

	glfwTerminate();
}






//-------------------------------------------------------------
// Fancy debug stuff
//-------------------------------------------------------------

// function to translate source to string
string getStringForSource(GLenum source) {

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		return("API");
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		return("Window System");
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		return("Shader Compiler");
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		return("Third Party");
	case GL_DEBUG_SOURCE_APPLICATION:
		return("Application");
	case GL_DEBUG_SOURCE_OTHER:
		return("Other");
	default:
		return("n/a");
	}
}

// function to translate severity to string
string getStringForSeverity(GLenum severity) {

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		return("HIGH!");
	case GL_DEBUG_SEVERITY_MEDIUM:
		return("Medium");
	case GL_DEBUG_SEVERITY_LOW:
		return("Low");
	default:
		return("n/a");
	}
}

// function to translate type to string
string getStringForType(GLenum type) {
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		return("Error");
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return("Deprecated Behaviour");
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return("Undefined Behaviour");
	case GL_DEBUG_TYPE_PORTABILITY:
		return("Portability Issue");
	case GL_DEBUG_TYPE_PERFORMANCE:
		return("Performance Issue");
	case GL_DEBUG_TYPE_OTHER:
		return("Other");
	default:
		return("n/a");
	}
}

// actually define the function
void APIENTRY debugCallbackARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message, GLvoid*) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) return;

	cerr << endl; // extra space

	cerr << "Type: " <<
		getStringForType(type) << "; Source: " <<
		getStringForSource(source) << "; ID: " << id << "; Severity: " <<
		getStringForSeverity(severity) << endl;

	cerr << message << endl;

	if (type == GL_DEBUG_TYPE_ERROR_ARB) throw runtime_error("");
}


