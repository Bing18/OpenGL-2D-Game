#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>
#include <ao/ao.h>
#include <mpg123.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;

	GLenum PrimitiveMode;
	GLenum FillMode;
	int NumVertices;
};
typedef struct VAO VAO;

int fbwidth=1400,fbheight=800;
struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	//    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
			0,                  // attribute 0. Vertices
			3,                  // size (x,y,z)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
			1,                  // attribute 1. Color
			3,                  // size (r,g,b)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/
typedef struct shape{

	float trans_dir;
	float rot_dir;
	float rotation;
	float trans;
	float status;
}shape ;
shape trishape[10],rectshape[20];
typedef struct mirshape{
	float trans_x;
	float trans_y;
	float rot;
	float x1;
	float y1;
	float x2;
	float y2;
}mirshape;
mirshape mirror[5];
typedef struct bulletshape{
	float rad;
	int status;
	float angle;
	float trans;
	float newx;
	float newy;
	float nx;
	float ny;
}bulletshape;
bulletshape bullet[20];
float circle_rotation = 0;
float semicircle_rotation=0;
float brick_trans[20]={0},brick_status[20]={0},brick_x[20],brick_color[20];
double current_time1,last_update_time1=glfwGetTime();
double current_time2,last_update_time2=glfwGetTime();
double mfire=glfwGetTime()-1;
int rightkey=0,leftkey=0,rightctrl=0,rightalt=0;
int spaceflag=0;int score=0;
int bricks=0;
float brick_increment=0.03;
int zoom=0,flagmouse=0;float pan=0;
void* play_audio(string audioFile);

void* play_audio(string audioFile){
	mpg123_handle *mh;
	unsigned char *buffer;
	size_t buffer_size;
	size_t done;
	int err;

	int driver;
	ao_device *dev;

	ao_sample_format format;
	int channels, encoding;
	long rate;

	/* initializations */
	ao_initialize();
	driver = ao_default_driver_id();
	mpg123_init();
	mh = mpg123_new(NULL, &err);
	buffer_size = mpg123_outblock(mh);
	buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

	/* open the file and get the decoding format */
	mpg123_open(mh, &audioFile[0]);
	mpg123_getformat(mh, &rate, &channels, &encoding);

	/* set the output format and open the output device */
	format.bits = mpg123_encsize(encoding) * 8;
	format.rate = rate;
	format.channels = channels;
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;
	dev = ao_open_live(driver, &format, NULL);

	/* decode and play */
	char *p =(char *)buffer;
	while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
		ao_play(dev, p, done);

	/* clean up */
	free(buffer);
	ao_close(dev);
	mpg123_close(mh);
	mpg123_delete(mh);
}

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Function is called first on GLFW_PRESS.
	if (action == GLFW_RELEASE) {
		if(key==GLFW_KEY_RIGHT )
		{
			rightkey=0;
			rectshape[1].trans_dir=0;
			rectshape[2].trans_dir=0;
		}
		if(key==GLFW_KEY_LEFT)
		{
			leftkey=0;
			rectshape[1].trans_dir=0;
			rectshape[2].trans_dir=0;
		}
		if(key==GLFW_KEY_RIGHT_CONTROL)
		{
			rightctrl=0;
			rectshape[1].trans_dir=0;
		}
		if(key==GLFW_KEY_RIGHT_ALT)
		{
			rightalt=0;
			rectshape[2].trans_dir=0;
		}
		switch (key) {
			case GLFW_KEY_A:
				rectshape[0].rot_dir = 0;
				break;
			case GLFW_KEY_D:
				rectshape[0].rot_dir = 0;
				break;
			case GLFW_KEY_S:
				rectshape[0].trans_dir = 0;
				break;
			case GLFW_KEY_F:
				rectshape[0].trans_dir = 0;
				break;
			case GLFW_KEY_SPACE:
				spaceflag=0;
				break;
			default:
				break;
		}
	}
	else if (action == GLFW_PRESS) {
		flagmouse=0;
		if(key==GLFW_KEY_RIGHT)
			rightkey=1;
		if(key==GLFW_KEY_LEFT)
		{
			leftkey=1;
		}
		if(key==GLFW_KEY_RIGHT_CONTROL)
		{
			rightctrl=1;
		}
		if(key==GLFW_KEY_RIGHT_ALT)
		{
			rightalt=1;
		}
		if(rightctrl==1 && rightkey==1)
		{
			rectshape[1].trans_dir=1;
		}
		if(rightctrl==1 && leftkey==1)
		{
			rectshape[1].trans_dir=-1;
		}
		if(rightkey==1 && rightalt==1)
		{
			rectshape[2].trans_dir=1;
		}
		if(leftkey==1 && rightalt==1)
		{
			rectshape[2].trans_dir=-1;
		}
		if(key==GLFW_KEY_UP && zoom<4){
			zoom++;
			flagmouse=1;
		}
		if(key==GLFW_KEY_DOWN && zoom){
			zoom--;
			flagmouse=1;
		}
		if(key==GLFW_KEY_LEFT && zoom){
			pan++;
			flagmouse=1;
		}
		if(key==GLFW_KEY_RIGHT && zoom){
			pan--;
			flagmouse=1;
		}
		if(flagmouse){
			if(pan>zoom)
				pan=zoom;
			if(pan<-zoom)
				pan=-zoom;
			Matrices.projection = glm::ortho(-5.0f+zoom-pan, 5.0f-zoom-pan, -5.0f+zoom, 5.0f-zoom, 0.1f, 500.0f);
		}
		switch (key) {
			case GLFW_KEY_ESCAPE:
				quit(window);
				break;
			case GLFW_KEY_A:
				rectshape[0].rot_dir = 1;
				break;
			case GLFW_KEY_D:
				rectshape[0].rot_dir=-1;
				break;
			case GLFW_KEY_S:
				rectshape[0].trans_dir=1;
				break;
			case GLFW_KEY_F:
				rectshape[0].trans_dir=-1;
				break;
			case GLFW_KEY_SPACE:
				//system("canberra-gtk-play -f /home/sathwik/Downloads/smb_fireball.wav");
				spaceflag=1;
				break;
			case GLFW_KEY_N:
				brick_increment+=0.02;
				break;
			case GLFW_KEY_M:
				if(brick_increment-0.01>0.0)
					brick_increment-=0.01;
			default:
				break;
		}
	}
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
			exit(0);
			break;
		default:
			break;
	}
}
VAO *triangle[10], *rectangle[30],*circle[5],*semicircle,*brickblock[20],*bulletblocks[15];

int bullets=0;
void createbullets (GLfloat x1,GLfloat y1,
		GLfloat x2,GLfloat y2,
		GLfloat x3,GLfloat y3,
		GLfloat x4,GLfloat y4,
		int mouseclick)
{
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat color_buffer_data[18]={0};
	GLfloat vertex_buffer_data[]={
		x1,y1,0,
		x2,y2,0,
		x3,y3,0,
		x3,y3,0,
		x4,y4,0,
		x1,y1,0
	};
	for(int i=0;i<16;i+=3)
	{
		color_buffer_data[i]=0;
		color_buffer_data[i+1]=0.5;
		color_buffer_data[i+2]=1;
	}
	bullet[bullets%15].rad=0;
	if(!mouseclick)
		bullet[bullets%15].angle=0;
	bullet[bullets%15].status=1;
	bullet[bullets%15].trans=0;

	// create3DObject creates and returns a handle to a VAO that can be used later
	bulletblocks[bullets%15] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
	bullets++;
}

int m_redbasket=0,m_greenbasket=0,m_canon=0,m_flag=0;
double mouse_xpos,mouse_ypos,mouse_click_x;
static void cursor_position(GLFWwindow* window,double xpos,double ypos)
{
	mouse_xpos=(10*xpos/fbwidth)-5;
	mouse_ypos=-(10*ypos/fbheight)+5;
	if(m_redbasket==1 && 1+mouse_xpos<5.5 && 1+mouse_xpos>-1.75)
		rectshape[1].trans=1+mouse_xpos;
	if(m_greenbasket && -1+mouse_xpos<3.5 && -1+mouse_xpos>-3.75)
		rectshape[2].trans=-1+mouse_xpos;
	if(m_canon && mouse_ypos>-3.5 && mouse_ypos<3.5)
		rectshape[0].trans=mouse_ypos;
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	if(action==GLFW_RELEASE){
		if(button==GLFW_MOUSE_BUTTON_LEFT){
			m_redbasket=0;
			m_greenbasket=0;
			m_canon=0;
		}
		if(button==GLFW_MOUSE_BUTTON_RIGHT)
			m_flag=0;
	}
	else if(action==GLFW_PRESS){
		if(button==GLFW_MOUSE_BUTTON_LEFT){
			if(mouse_xpos>=-5.0 && mouse_xpos<=-4.65 && mouse_ypos>=rectshape[0].trans-0.1 && mouse_ypos<=rectshape[0].trans+0.1){
				m_canon=1;
			}
			else if(mouse_xpos>=-1.35+rectshape[1].trans && mouse_xpos<=-0.65+rectshape[1].trans && mouse_ypos<=-3.9 && mouse_ypos>=-4.9){
				m_redbasket=1;
			}
			else if(mouse_xpos>=0.65+rectshape[2].trans && mouse_xpos<=1.35+rectshape[2].trans && mouse_ypos<=-3.9 && mouse_ypos>=-4.9)
				m_greenbasket=1;
			else if(mouse_xpos>-4.42 && glfwGetTime()-mfire>=1){
				float slope=(mouse_ypos-rectshape[0].trans)/(mouse_xpos+4.42);
				float mouseangle=(atan(slope)*180.0)/M_PI;

				if(mouseangle>=-60 && mouseangle<=60){
					mfire=glfwGetTime();
					bullet[bullets%15].angle=mouseangle;
					rectshape[0].rotation=mouseangle;
					thread(play_audio,"/home/sathwik/Downloads/beep5.mp3").detach();
					createbullets(-0.09,0.03, -0.09,-0.03, 0.09,-0.03, 0.09,0.03,1);
				}

			}
		}
		if(button==GLFW_MOUSE_BUTTON_RIGHT){
			if(!m_flag){
				mouse_click_x=mouse_xpos;
			}
			m_flag=1;
		}
	}
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	zoom += yoffset;
	if(zoom>=5)
		zoom=4;
	if(zoom<0)
		zoom=0;
	if(pan>zoom)
		pan=zoom;
	if(pan<-zoom)
		pan=-zoom;
	Matrices.projection = glm::ortho(-5.0f+zoom-pan, 5.0f-zoom-pan, -5.0f+zoom, 5.0f-zoom, 0.1f, 500.0f);
}



/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	fbwidth=width, fbheight=height;
	/* With Retina display on Mac OS X, GLFW's FramebufferSize
	   is different from WindowSize */
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
	// Perspective projection for 3D views
	// Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

	// Ortho projection for 2D views
	Matrices.projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 500.0f);
}


// Creates the triangle object used in this sample code
void createTriangle (GLfloat x1,GLfloat y1,
		GLfloat x2,GLfloat y2,
		GLfloat x3,GLfloat y3,
		GLfloat c1,GLfloat c2,GLfloat c3,
		int j)
{
	GLfloat vertex_buffer_data[]={
		x1,y1,0,
		x2,y2,0,
		x3,y3,0,
	};
	GLfloat color_buffer_data[9]={0};
	for(int i=0;i<7;i+=3){
		color_buffer_data[i]=c1;
		color_buffer_data[i+1]=c2;
		color_buffer_data[i+2]=c3;
	}
	trishape[j].rotation=0;
	trishape[j].trans=0;
	trishape[j].rot_dir=0;
	trishape[j].trans_dir=0;
	trishape[j].status=0;
	triangle[j] = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}

// Creates the rectangle object used in this sample code
void createRectangle (GLfloat x1,GLfloat y1,
		GLfloat x2,GLfloat y2,
		GLfloat x3,GLfloat y3,
		GLfloat x4,GLfloat y4,
		int j, int color,int type)
{
	//color 1:red 2:GREEN 0:black 3:Canon(blue)
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat color_buffer_data[18]={0};
	GLfloat vertex_buffer_data[]={
		x1,y1,0,
		x2,y2,0,
		x3,y3,0,
		x3,y3,0,
		x4,y4,0,
		x1,y1,0
	};
	if(type==0){
		if(color==3){
			for(int i=0;i<16;i+=3)
			{
				color_buffer_data[i]=23.0/255.0;
				color_buffer_data[i+1]=32.0/255.0;
				color_buffer_data[i+2]=42.0/255.0;
			}
		}
		else if(color==1){
			for(int i=0;i<16;i+=3)
			{
				color_buffer_data[i]=203.0/255.0;
				color_buffer_data[i+1]=67.0/255.0;
				color_buffer_data[i+2]=53.0/255.0;
			}
		}
		else if(color==2){
			for(int i=0;i<16;i+=3)
			{
				color_buffer_data[i]=40.0/255.0;
				color_buffer_data[i+1]=180.0/255.0;
				color_buffer_data[i+2]=99.0/255.0;
			}
		}
		else if(color==4)
		{
			for(int i=0;i<16;i+=3)
			{
				color_buffer_data[i]=253.0/255.0;
				color_buffer_data[i+1]=254.0/255.0;
				color_buffer_data[i+2]=254.0/255.0;
			}
		}
		rectshape[j].rotation=0;
		rectshape[j].rot_dir=0;
		rectshape[j].trans_dir=0;
		rectshape[j].trans=0;
		rectshape[j].status=1;
	}
	if(type)
	{
		for(int i=0;i<16;i+=3)
		{
			color_buffer_data[i]=93.0/255.0;
			color_buffer_data[i+1]=173.0/255.0;
			color_buffer_data[i+2]=226.0/255.0;
		}
		if(type==1){
			mirror[type-1].trans_x=-1.5;
			mirror[type-1].trans_y=3.5;
			mirror[type-1].rot=120;
			mirror[type-1].x1=0.6*(cos(120*M_PI/180.0f))+mirror[type-1].trans_x;
			mirror[type-1].y1=sin(120*M_PI/180.0f)*(0.6)+mirror[type-1].trans_y;
			mirror[type-1].x2=-0.6*(cos(120*M_PI/180.0f))+mirror[type-1].trans_x;
			mirror[type-1].y2=sin(120*M_PI/180.0f)*(-0.6)+mirror[type-1].trans_y;
		}
		else if(type==2){
			mirror[type-1].trans_x=3.5;
			mirror[type-1].trans_y=3.0;
			mirror[type-1].rot=120;
			mirror[type-1].x1=0.6*(cos(120*M_PI/180.0f))+mirror[type-1].trans_x;
			mirror[type-1].y1=sin(120*M_PI/180.0f)*(0.6)+mirror[type-1].trans_y;
			mirror[type-1].x2=-0.6*(cos(120*M_PI/180.0f))+mirror[type-1].trans_x;
			mirror[type-1].y2=sin(120*M_PI/180.0f)*(-0.6)+mirror[type-1].trans_y;
		}
		else if(type==3)
		{
			mirror[type-1].trans_x=1;
			mirror[type-1].trans_y=-2.5;
			mirror[type-1].rot=25;
			mirror[type-1].x2=0.6*(cos(25*M_PI/180.0f))+mirror[type-1].trans_x;
			mirror[type-1].y2=sin(25*M_PI/180.0f)*(0.6)+mirror[type-1].trans_y;
			mirror[type-1].x1=-0.6*(cos(25*M_PI/180.0f))+mirror[type-1].trans_x;
			mirror[type-1].y1=sin(25*M_PI/180.0f)*(-0.6)+mirror[type-1].trans_y;
		}
	}
	// create3DObject creates and returns a handle to a VAO that can be used later
	rectangle[j] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createbricks (GLfloat x1,GLfloat y1,
		GLfloat x2,GLfloat y2,
		GLfloat x3,GLfloat y3,
		GLfloat x4,GLfloat y4,
		int color)
{
	//color 1:red 2:GREEN 0:black 3:Canon(blue)
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat color_buffer_data[18]={0};
	GLfloat vertex_buffer_data[]={
		x1,y1,0,
		x2,y2,0,
		x3,y3,0,
		x3,y3,0,
		x4,y4,0,
		x1,y1,0
	};
	//black
	if(color==0){
		for(int i=0;i<16;i+=3)
		{
			color_buffer_data[i]=0;
			color_buffer_data[i+1]=0;
			color_buffer_data[i+2]=0;
		}
	}
	//red
	else if(color==1){
		for(int i=0;i<16;i+=3)
		{
			color_buffer_data[i]=231.0/255.0;
			color_buffer_data[i+1]=76.0/255.0;
			color_buffer_data[i+2]=60.0/255.0;
		}
	}
	//green
	else if(color==2){
		for(int i=0;i<16;i+=3)
		{
			color_buffer_data[i]=46.0/255.0;
			color_buffer_data[i+1]=204.0/255.0;
			color_buffer_data[i+2]=113.0/255.0;
		}
	}
	brick_color[bricks%15]=color;
	brick_status[bricks%15]=1;
	// create3DObject creates and returns a handle to a VAO that can be used later
	brickblock[bricks%15] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
	bricks++;
}
void createCircle(float radius,int j)
{
	GLfloat vertex_buffer_data[360*9]={0};
	GLfloat color_buffer_data[360*9];
	for(int i=0;i<360;i++)
	{
		vertex_buffer_data[9*i+3]=radius*cos(i*M_PI/180.0f);
		vertex_buffer_data[9*i+4]=radius*sin(i*M_PI/180.0f);
		vertex_buffer_data[9*i+6]=radius*cos((i+1)*M_PI/180.0f);
		vertex_buffer_data[9*i+7]=radius*sin((i+1)*M_PI/180.0f);
	}
	for(int i=0;i<360*9;i++)
	{
		color_buffer_data[i]=0.6;
	}
	circle[j] = create3DObject(GL_TRIANGLES,360*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createSemicircle(float radius)
{
	GLfloat vertex_buffer_data[360*9]={0};
	GLfloat color_buffer_data[360*9];
	for(int i=0;i<90;i++)
	{
		vertex_buffer_data[9*i+3]=radius*cos(i*M_PI/180.0f);
		vertex_buffer_data[9*i+4]=radius*sin(i*M_PI/180.0f);
		vertex_buffer_data[9*i+6]=radius*cos((i+1)*M_PI/180.0f);
		vertex_buffer_data[9*i+7]=radius*sin((i+1)*M_PI/180.0f);
	}
	for(int i=270;i<360;i++)
	{
		vertex_buffer_data[9*i+3]=radius*cos(i*M_PI/180.0f);
		vertex_buffer_data[9*i+4]=radius*sin(i*M_PI/180.0f);
		vertex_buffer_data[9*i+6]=radius*cos((i+1)*M_PI/180.0f);
		vertex_buffer_data[9*i+7]=radius*sin((i+1)*M_PI/180.0f);
	}
	for(int i=0;i<360*9;i=i+3)
	{
		color_buffer_data[i]=142.0/255.0;
		color_buffer_data[i+1]=68.0/255.0;
		color_buffer_data[i+2]=173.0/255.0;
	}
	semicircle = create3DObject(GL_TRIANGLES,360*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void randombricks()
{
	int z=rand()%8;
	int p=rand()%3;
	brick_x[bricks%15]=z-3;
	float len=sqrt(0.3625);
	float mirx1=0.6*(cos(120*M_PI/180.0f))+0.05*(sin(120*M_PI/180.0f))+mirror[0].trans_x;
	float mirx2=-0.6*(cos(120*M_PI/180.0f))-0.05*(sin(120*M_PI/180.0f))+mirror[0].trans_x;
	float mirx3=0.6*(cos(120*M_PI/180.0f))+0.05*(sin(120*M_PI/180.0f))+mirror[1].trans_x;
	float mirx4=-0.6*(cos(120*M_PI/180.0f))-0.05*(sin(120*M_PI/180.0f))+mirror[1].trans_x;
	float mirx5=0.6*(cos(25*M_PI/180.0f))-0.05*(sin(25*M_PI/180.0f))+mirror[2].trans_x;
	float mirx6=-0.6*(cos(25*M_PI/180.0f))+0.05*(sin(25*M_PI/180.0f))+mirror[2].trans_x;
	//restrict bricks from falling on mirrors
	while((z-3>mirx1 && z-3<mirx2) || (z-3>mirx3 && z-3<mirx4) || (z-3>mirx6 && z-3<mirx5))
		z=rand()%8;
	createbricks(-0.1+z,0.2, -0.1+z,-0.2, 0.1+z,-0.2, 0.1+z,0.2, p);
}
//for penalty box
int wrong=0,tricount=1;
void checkcollision()
{
	for(int i=0;i<15;i++)
	{
		for(int j=0;j<15;j++)
		{
			if(brick_status[i] && bullet[j].status){
				if(bullet[j].newx+0.09*cos(bullet[j].angle*M_PI/180.0f)>=brick_x[i]-0.1 && bullet[j].newx+0.09*cos(bullet[j].angle*M_PI/180.0f)<=brick_x[i]+0.1 && bullet[j].newy>=4.55-brick_trans[i] && bullet[j].newy<=4.95-brick_trans[i])
				{
					if(brick_color[i]==0)
						score+=10;
					else{
						createTriangle (-0.2,0.25, -0.2,0.25, 0.1,-0.25, 250.0/255,23.0/255.0,5.0/255.0, tricount);
						createTriangle (0.1,0.25, 0.1,0.25, -0.2,-0.25, 250.0/255,23.0/255.0,5.0/255.0, tricount+1);
						tricount+=2;
						wrong++;
						score-=5;
						if(wrong>4)
						{
							printf("GAME OVER!\n");
							printf("Score: %d\n",score);
							thread(play_audio,"/home/sathwik/Downloads/beep4.mp3").detach();
							exit(0);
						}
					}
					brick_status[i]=0;
					bullet[j].status=0;
					bullet[j].angle=0;
					bullet[j].trans=0;
					brick_trans[i]=0;
					printf("Score: %d\n",score);
					break;
				}
			}
		}
	}
}
float x_intersection,y_intersection;
int intersection(float x0,float x1,float y0,float y1,int i)
{
	float x2=0.09*cos(bullet[i].angle*M_PI/180.0f)+bullet[i].newx;
	float y2=0.09*sin(bullet[i].angle*M_PI/180.0f)+bullet[i].newy-0.01;
	float x3=-0.09*cos(bullet[i].angle*M_PI/180.0f)+bullet[i].newx;
	float y3=-0.09*sin(bullet[i].angle*M_PI/180.0f)+bullet[i].newy-0.01;

	float s1_x, s1_y, s2_x, s2_y, q, p, r;

	s1_x = x1 - x0;
	s1_y = y1 - y0;
	s2_x = x3 - x2;
	s2_y = y3 - y2;

	r=s1_x*s2_y - s2_x*s1_y;
	if(r==0){
		return 0;
	}

	p = (s1_x*(y0-y2) - s1_y*(x0-x2))/(r*1.0f);
	q = (s2_x*(y0-y2) - s2_y*(x0-x2))/(r*1.0f);

	if (p>=0 && p<=1 && q>=0 && q<=1)
	{
		x_intersection = x0 + (q * s1_x);
		y_intersection = y0 + (q * s1_y);
		return 1;
	}
	return 0;
}
int reflect[20]={0};
void checkreflection()
{
	for(int i=0;i<15;i++)
	{
		//mirror1 with angle 120deg, -1.5 transx and 3.5 transy
		for(int j=0;j<3;j++)
		{
			if(bullet[i].status==1){
				if(intersection(mirror[j].x1,mirror[j].x2,mirror[j].y1,mirror[j].y2,i))
				{
					bullet[i].nx=x_intersection;
					bullet[i].ny=y_intersection+0.01;
					reflect[i]=1;
					bullet[i].angle=2*mirror[j].rot-bullet[i].angle;
					bullet[i].status=1;
					bullet[i].rad=0.16;
				}
			}
		}
	}
}
float camera_rotation_angle = 90;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
	// clear the color and depth in the frame buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	// Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model

	// Load identity to model matrix
	Matrices.model = glm::mat4(1.0f);

	/* Render your scene */

	glm::mat4 translateTriangle = glm::translate (glm::vec3(0.0f, -3.6f, 0.0f)); // glTranslatef
	glm::mat4 rotateTriangle = glm::rotate((float)(trishape[0].rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
	glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
	Matrices.model *= triangleTransform;
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	//draw3DObject(triangle[0]);

	// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
	// glPopMatrix ();
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateRectangle = glm::translate (glm::vec3(-4.77,rectshape[0].trans, 0));
	// glTranslatef
	glm::mat4 rotateRectangle = glm::rotate((float)(rectshape[0].rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateRectangle * rotateRectangle);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(rectangle[0]);

	//RED BASKET
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateRectangle1 = glm::translate (glm::vec3(-1+rectshape[1].trans,-4.4, 2));
	// glTranslatef
	glm::mat4 rotateRectangle1 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateRectangle1 * rotateRectangle1);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(rectangle[1]);

	//GREEN BASKET
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateRectangle2 = glm::translate (glm::vec3(1+rectshape[2].trans,-4.4, 2));
	// glTranslatef
	glm::mat4 rotateRectangle2 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateRectangle2 * rotateRectangle2);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(rectangle[2]);


	//***BRICKS***
	current_time1 = glfwGetTime();
	if ((current_time1 - last_update_time1) >= 2.0) {
		last_update_time1 = current_time1;
		randombricks();
	}
	for(int var=0;var<15;var++)
	{
		if(brick_status[var]==1)
		{
			Matrices.model = glm::mat4(1.0f);
			glm::mat4 translateRectangle4 = glm::translate (glm::vec3(-3,4.75-brick_trans[var],0));
			// glTranslatef
			glm::mat4 rotateRectangle4 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
			Matrices.model *= (translateRectangle4 * rotateRectangle4);
			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			draw3DObject(brickblock[var]);

			brick_trans[var]+=brick_increment;

			if(4.75-brick_trans[var]<-3.9)
			{
				if(brick_color[var]==1){
					if(abs(-1+rectshape[1].trans-(1+rectshape[2].trans))<=0.35)
						score--;
					else if(-1+rectshape[1].trans<=brick_x[var]+0.25 && -1+rectshape[1].trans>=brick_x[var]-0.25)
						score++;
					else
						score--;
				}

				if(brick_color[var]==2){
					if(abs(-1+rectshape[1].trans-(1+rectshape[2].trans))<=0.35)
						score--;
					else if(1+rectshape[2].trans<=brick_x[var]+0.25 && 1+rectshape[2].trans>=brick_x[var]-0.25)
					{
						score+=1;
					}
					else
						score-=1;
				}
				brick_status[var]=0;
				brick_trans[var]=0;
				printf("Score: %d\n",score);
				if(brick_color[var]==0)
				{
					//system("canberra-gtk-play -f /home/sathwik/Downloads/smb_gameover.wav");
					//thread(play_audio,"/home/sathwik/Downloads/beep4.mp3").detach();
					printf("\n GAMEOVER \n");
					printf("Score: %d \n",score);
					exit(0);
				}
			}
		}
	}
	for(int q=0;q<3;q++)
	{
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle5 = glm::translate (glm::vec3(mirror[q].trans_x,mirror[q].trans_y, 0));
		// glTranslatef
		glm::mat4 rotateRectangle5 = glm::rotate((float)(mirror[q].rot*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
		Matrices.model *= (translateRectangle5 * rotateRectangle5);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(rectangle[3+q]);
	}
	//BULLETS
	current_time2 = glfwGetTime();
	if ((current_time2 - last_update_time2) >= 1.0 && spaceflag==1) {
		last_update_time2 = current_time2;
		createbullets(-0.09,0.03, -0.09,-0.03, 0.09,-0.03, 0.09,0.03,0);
			thread(play_audio,"/home/sathwik/Downloads/beep5.mp3").detach();
	}
	for(int var=0;var<15;var++){
		if(bullet[var].status==1)
		{
			Matrices.model = glm::mat4(1.0f);
			if(bullet[var].angle==0 && reflect[var]==0)
				bullet[var].angle=rectshape[0].rotation;
			if(bullet[var].trans==0)
				bullet[var].trans=rectshape[0].trans;
			if(!reflect[var]){
				bullet[var].newx=-4.68+bullet[var].rad*cos(bullet[var].angle*M_PI/180.0f);
				bullet[var].newy=bullet[var].trans+bullet[var].rad*sin(bullet[var].angle*M_PI/180.0f);
			}
			if(reflect[var])
			{
				bullet[var].newx=bullet[var].nx+bullet[var].rad*(cos(bullet[var].angle*M_PI/180.0f));
				bullet[var].newy=bullet[var].ny+bullet[var].rad*sin(bullet[var].angle*M_PI/180.0f);
			}
			glm::mat4 translateRectangle3 = glm::translate (glm::vec3(bullet[var].newx,bullet[var].newy-0.01, 0));
			// glTranslatef
			glm::mat4 rotateRectangle3 = glm::rotate((float)(bullet[var].angle*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
			Matrices.model *= (translateRectangle3 * rotateRectangle3);
			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			draw3DObject(bulletblocks[var]);
			bullet[var].rad+=0.16;
			if(bullet[var].newx>4.8 || bullet[var].newx<-4.8 || bullet[var].newy>4.8 || bullet[var].newy<-4.8){
				bullet[var].status=0;
				bullet[var].rad=0;
				reflect[var]=0;
			}
		}
	}
	checkcollision();
	checkreflection();

	//penaltybox
	for(int i=0;i<4;i++){
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle6 = glm::translate (glm::vec3(-4.7+0.33*i,4.5,0));
		// glTranslatef
		glm::mat4 rotateRectangle6 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
		Matrices.model *= (translateRectangle6 * rotateRectangle6);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(rectangle[6+i]);
	}
	for(int j=0;j<wrong;j++){
		for(int i=0;i<2;i++){
			Matrices.model = glm::mat4(1.0f);
			glm::mat4 translateTriangle1 = glm::translate (glm::vec3(-4.7+0.33*j,4.5,0));
			// glTranslatef
			glm::mat4 rotateTriangle1 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
			Matrices.model *= (translateTriangle1 * rotateTriangle1);
			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			draw3DObject(triangle[1+i+2*j]);
		}
	}
	float laser_incr=0.1;
	float laser_trans_check=rectshape[3].trans+laser_incr*rectshape[3].trans_dir;
	if(laser_trans_check<9.0)
	{
		rectshape[3].trans=laser_trans_check;
	}
	else
	{
		rectshape[3].trans=0;
		rectshape[3].trans_dir=0;
	}

	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateCircle = glm::translate (glm::vec3(-1+rectshape[1].trans, -3.9, 0));        // glTranslatef
	glm::mat4 rotateCircle = glm::rotate((float)(65*M_PI/180.0f), glm::vec3(1,0,0)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateCircle * rotateCircle);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(circle[0]);

	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateCircle1 = glm::translate (glm::vec3(1+rectshape[2].trans,-3.9, 0));        // glTranslatef
	glm::mat4 rotateCircle1 = glm::rotate((float)(65*M_PI/180.0f), glm::vec3(1,0,0)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateCircle1 * rotateCircle1);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(circle[1]);

	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateSemicircle = glm::translate (glm::vec3(-5,rectshape[0].trans, 0));        // glTranslatef
	glm::mat4 rotateSemicircle = glm::rotate((float)(semicircle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateSemicircle * rotateSemicircle);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(semicircle);
	// Increment angles
	float increments = 1,trans_increment=0.03;
	float redbasket_trans_check=rectshape[1].trans+trans_increment*rectshape[1].trans_dir;
	if(redbasket_trans_check<5.5 && redbasket_trans_check>-1.75)
	{
		rectshape[1].trans=redbasket_trans_check;
	}
	float greenbasket_trans_check=rectshape[2].trans+trans_increment*rectshape[2].trans_dir;
	if(greenbasket_trans_check<3.5 && greenbasket_trans_check>-3.75)
	{
		rectshape[2].trans=greenbasket_trans_check;
	}
	//camera_rotation_angle++; // Simulating camera rotation
	float rectangle_rot_check=rectshape[0].rotation + increments*(rectshape[0].rot_dir);
	if(rectangle_rot_check<60 && rectangle_rot_check>-60)
	{
		rectshape[0].rotation=rectangle_rot_check;
	}
	float canon_trans_check=rectshape[0].trans+trans_increment*rectshape[0].trans_dir;
	if(canon_trans_check<3.5 && canon_trans_check>-3.5)
	{
		rectshape[0].trans=canon_trans_check;
	}
	//mousepan
	if(m_flag && zoom>0){
		pan-=(mouse_click_x - mouse_xpos);
		mouse_click_x=mouse_xpos;
		if(pan>zoom)
			pan=zoom;
		if(pan<-zoom)
			pan=-zoom;
		Matrices.projection = glm::ortho(-5.0f+zoom-pan, 5.0f-zoom-pan, -5.0f+zoom, 5.0f-zoom, 0.1f, 500.0f);
	}


}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
	GLFWwindow* window; // window desciptor/handle

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		//        exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

	if (!window) {
		glfwTerminate();
		//        exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );

	/* --- register callbacks with GLFW --- */

	/* Register function to handle window resizes */
	/* With Retina display on Mac OS X GLFW's FramebufferSize
	   is different from WindowSize */
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);

	/* Register function to handle window close */
	glfwSetWindowCloseCallback(window, quit);

	/* Register function to handle keyboard input */
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

	/* Register function to handle mouse click */
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
	glfwSetCursorPosCallback(window, cursor_position);
	glfwSetScrollCallback(window, scroll_callback);



	return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
	/* Objects should be created before any other gl function and shaders */
	// Create the models
	createTriangle (-5.0,0, -5.0,0, 5.0,0, 0.6,0.6,0.6, 0); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createRectangle( 0,-0.1, 0.35,-0.1, 0.35,0.1, 0,0.1, 0, 3, 0);
	createRectangle(-0.35,0.5, -0.35,-0.5, 0.35,-0.5, 0.35,0.5, 1, 1, 0);
	createRectangle(-0.35,0.5, -0.35,-0.5, 0.35,-0.5, 0.35,0.5, 2, 2, 0);
	createCircle(0.35,0);
	createCircle(0.35,1);
	createSemicircle(0.35);
	createRectangle(-0.6,0.05, -0.6,-0.05, 0.6,-0.05, 0.6,0.05, 3, 0, 1);
	createRectangle(-0.6,0.05, -0.6,-0.05, 0.6,-0.05, 0.6,0.05, 4, 0, 2);
	createRectangle(-0.6,0.05, -0.6,-0.05, 0.6,-0.05, 0.6,0.05, 5, 0, 3);
	createRectangle(-0.2,0.25, -0.2,-0.25, 0.1,-0.25, 0.1,0.25, 6, 4, 0);
	createRectangle(-0.2,0.25, -0.2,-0.25, 0.1,-0.25, 0.1,0.25, 7, 4, 0);
	createRectangle(-0.2,0.25, -0.2,-0.25, 0.1,-0.25, 0.1,0.25, 8, 4, 0);
	createRectangle(-0.2,0.25, -0.2,-0.25, 0.1,-0.25, 0.1,0.25, 9, 4, 0);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

	// Background color of the scene
	glClearColor (214.0/255.0, 234.0/255.0, 248.0/255.0, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}



int main (int argc, char** argv)
{
	int width = 1400;//1400
	int height = 800;//800

	GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

	double last_update_time = glfwGetTime(), current_time;

	/* Draw in loop */
	while (!glfwWindowShouldClose(window)) {

		// OpenGL Draw commands
		draw();

		// Swap Frame Buffer in double buffering
		glfwSwapBuffers(window);

		// Poll for Keyboard and mouse events
		glfwPollEvents();

		// Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
		current_time = glfwGetTime(); // Time in seconds
		if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
			// do something every 0.5 seconds ..
			last_update_time = current_time;
		}
	}

	glfwTerminate();
	//    exit(EXIT_SUCCESS);
}
