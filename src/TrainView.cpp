#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>

#include <GL/glu.h>
#include <GL/glut.h>

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"

#include "Utilities/Matrices.h"
#include "Utilities/objloader.hpp"
#include "My_functions.H"
#include <vector> 



// Constructor to set up the GL window
TrainView::
TrainView(int x, int y, int w, int h, const char* l) :
	Fl_Gl_Window(x, y, w, h, l)
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
	camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
	camera.MovementSpeed = 200.0f;
	camera.Position = glm::vec3(50.0, 100.0, 0.0);
	old_t = glutGet(GLUT_ELAPSED_TIME);
	k_pressed = false;
	
}

// * Reset the camera to look at the world
void TrainView::
resetArcball()
{
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

// * FlTk Event handler for the window
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value()) {
		if (tw->arcball->value()) {
			if (arcball.handle(event))
				return 1;
		}
		
	}


	// remember what button was used
	static int last_push;
	

	switch (event) {
		// Mouse button being pushed event
	case FL_PUSH:
		last_push = Fl::event_button();
		// if the left button be pushed is left mouse button
		if (last_push == FL_LEFT_MOUSE) {
			doPick();
			damage(1);
			return 1;
		}
		else if (last_push == FL_RIGHT_MOUSE) {
			int xpos = Fl::event_x();
			int ypos = Fl::event_y();
			lastX = xpos;
			lastY = ypos;
			damage(1);
			return 1;
		}
		break;

		// Mouse button release event
	case FL_RELEASE: // button release
		damage(1);
		last_push = 0;
		return 1;

		// Mouse button drag event
	case FL_DRAG:

		// Compute the new control point position
		if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
			ControlPoint* cp = &m_pTrack->points[selectedCube];

			double r1x, r1y, r1z, r2x, r2y, r2z;
			getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

			double rx, ry, rz;
			mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z,
				static_cast<double>(cp->pos.x),
				static_cast<double>(cp->pos.y),
				static_cast<double>(cp->pos.z),
				rx, ry, rz,
				(Fl::event_state() & FL_CTRL) != 0);

			cp->pos.x = (float)rx;
			cp->pos.y = (float)ry;
			cp->pos.z = (float)rz;
			damage(1);
		}
		else if (last_push == FL_RIGHT_MOUSE) {
			// where is the mouse?
			int xpos = Fl::event_x();
			int ypos = Fl::event_y();
			if (firstMouse)
			{
				lastX = xpos;
				lastY = ypos;
				firstMouse = false;
			}

			float xoffset = xpos - lastX;
			float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

			lastX = xpos;
			lastY = ypos;

			if (tw->fpv->value()) {
				camera.ProcessMouseMovement(xoffset, yoffset);
			}

			damage(1);
		}
		break;

		// in order to get keyboard events, we need to accept focus
	case FL_FOCUS:
		return 1;

		// every time the mouse enters this window, aggressively take focus
	case FL_ENTER:
		focus(this);
		break;

	case FL_KEYBOARD:
		if (k_pressed == false) {
			k_pressed = true;
			damage(1);
		}

		k = Fl::event_key();
		ks = Fl::event_state();
		if (k == 'p') {
			// Print out the selected control point information
			if (selectedCube >= 0)
				printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
					selectedCube,
					m_pTrack->points[selectedCube].pos.x,
					m_pTrack->points[selectedCube].pos.y,
					m_pTrack->points[selectedCube].pos.z,
					m_pTrack->points[selectedCube].orient.x,
					m_pTrack->points[selectedCube].orient.y,
					m_pTrack->points[selectedCube].orient.z);
			else
				printf("Nothing Selected\n");

			return 1;
		};
		break;

	case 9:
		k_pressed = false;
		//cout << "key up" << endl;
		break;
	}


	return Fl_Gl_Window::handle(event);
}

// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
void TrainView::draw()
{
	//calculate delta time
	updateTimer();

	// * Set up basic opengl informaiton
	//initialized glad
	if (gladLoadGL())
	{
		//initiailize VAO, VBO, Shader...
		
		//load shaders
		loadShaders();

		//load models
		loadModels();

		//load water object
		loadWaterMesh();

		//load skyBox object
		loadSkyBox();

		//initialize FBOs
		initFBOs();

		//initialize VAOs
		initVAOs();
		
		//original stuff
		if (!this->commom_matrices)
			this->commom_matrices = new UBO();
			this->commom_matrices->size = 2 * sizeof(glm::mat4);
			glGenBuffers(1, &this->commom_matrices->ubo);
			glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
			glBufferData(GL_UNIFORM_BUFFER, this->commom_matrices->size, NULL, GL_STATIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

		if (!this->plane) {
			GLfloat  vertices[] = {
				-0.5f ,0.0f , -0.5f,
				-0.5f ,0.0f , 0.5f ,
				0.5f ,0.0f ,0.5f ,
				0.5f ,0.0f ,-0.5f };
			GLfloat  normal[] = {
				0.0f, 1.0f, 0.0f,
				0.0f, 1.0f, 0.0f,
				0.0f, 1.0f, 0.0f,
				0.0f, 1.0f, 0.0f };
			GLfloat  texture_coordinate[] = {
				0.0f, 0.0f,
				1.0f, 0.0f,
				1.0f, 1.0f,
				0.0f, 1.0f };
			GLuint element[] = {
				0, 1, 2,
				0, 2, 3, };

			this->plane = new VAO;
			this->plane->element_amount = sizeof(element) / sizeof(GLuint);
			glGenVertexArrays(1, &this->plane->vao);
			glGenBuffers(3, this->plane->vbo);
			glGenBuffers(1, &this->plane->ebo);

			glBindVertexArray(this->plane->vao);

			// Position attribute
			glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);

			// Normal attribute
			glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(1);

			// Texture Coordinate attribute
			glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(2);

			//Element attribute
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);

			// Unbind VAO
			glBindVertexArray(0);
		}

		loadTextures();
		
		if (!this->device){
			//Tutorial: https://ffainelli.github.io/openal-example/
			this->device = alcOpenDevice(NULL);
			if (!this->device) {
				//puts("ERROR::NO_AUDIO_DEVICE");
			}
				

			ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
			if (enumeration == AL_FALSE) {
				//puts("Enumeration not supported");
			}
			else {
				//puts("Enumeration supported");
			}

			this->context = alcCreateContext(this->device, NULL);
			if (!alcMakeContextCurrent(context))
				puts("Failed to make context current");

			this->source_pos = glm::vec3(0.0f, 5.0f, 0.0f);

			ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
			alListener3f(AL_POSITION, source_pos.x, source_pos.y, source_pos.z);
			alListener3f(AL_VELOCITY, 0, 0, 0);
			alListenerfv(AL_ORIENTATION, listenerOri);

			alGenSources((ALuint)1, &this->source);
			alSourcef(this->source, AL_PITCH, 1);
			alSourcef(this->source, AL_GAIN, 1.0f);
			alSource3f(this->source, AL_POSITION, source_pos.x, source_pos.y, source_pos.z);
			alSource3f(this->source, AL_VELOCITY, 0, 0, 0);
			alSourcei(this->source, AL_LOOPING, AL_TRUE);

			alGenBuffers((ALuint)1, &this->buffer);

			ALsizei size, freq;
			ALenum format;
			ALvoid* data;
			ALboolean loop = AL_TRUE;

			//Material from: ThinMatrix
			alutLoadWAVFile((ALbyte*)"../resources/audio/YOASOBI0.wav", &format, &data, &size, &freq, &loop);
			alBufferData(this->buffer, format, data, size, freq);
			alSourcei(this->source, AL_BUFFER, this->buffer);

			if (format == AL_FORMAT_STEREO16 || format == AL_FORMAT_STEREO8)
				puts("TYPE::STEREO");
			else if (format == AL_FORMAT_MONO16 || format == AL_FORMAT_MONO8)
				puts("TYPE::MONO");

			alSourcef(source, AL_GAIN, 0.1);
			alSourcePlay(this->source);

			// cleanup context
			//alDeleteSources(1, &source);
			//alDeleteBuffers(1, &buffer);
			//device = alcGetContextsDevice(context);
			//alcMakeContextCurrent(NULL);
			//alcDestroyContext(context);
			//alcCloseDevice(device);
		}
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0,0,w(),h());
	

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	setProjection();		// put the code to set up matrices here

	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);

	// set linstener position 
	if(selectedCube >= 0)
		alListener3f(AL_POSITION, 
			m_pTrack->points[selectedCube].pos.x,
			m_pTrack->points[selectedCube].pos.y,
			m_pTrack->points[selectedCube].pos.z);
	else
		alListener3f(AL_POSITION, 
			this->source_pos.x, 
			this->source_pos.y,
			this->source_pos.z);


	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	glDisable(GL_LIGHTING);
	//drawFloor(200,10);

	//glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();

	// this time drawing is for shadows (except for top view)
	//if (!tw->topCam->value()) {
	//	setupShadows();
	//	//drawStuff(true);
	//	unsetupShadows();
	//}

	//use shader===========================================================================================================

	setUBO();
	glBindBufferRange(GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);

	//update current light_shader
	update_light_shaders();

	//drawGround();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glViewport(0, 0, 1920,1080);

	//drawMainFBO();

	//drawSubScreenFBO();

	//drawTrain();

	//drawTeapot();
	
	drawWater(tw->waveTypeBrowser->value());

	drawSkyBox();

	//draw main FBO to the whole screen
	//drawMainScreen();

	//drawSubScreen();

	//unbind VAO
	glBindVertexArray(0);

	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);
}

// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
void TrainView::
setProjection()
{
	glUseProgram(0);
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());
	projectionMatrix = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, (float)NEAR, (float)FAR);

	// Check whether we use the world camp
	if (tw->worldCam->value()) {
		if (tw->arcball->value()) {
			arcball.setProjection(false);
			float view_temp[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, view_temp);
			viewMatrix = glm::make_mat4(view_temp);
		}

		if (tw->fpv->value()) {
			updata_camera();
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();

			
			glMultMatrixf(&projectionMatrix[0][0]);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			viewMatrix = camera.GetViewMatrix();
			glMultMatrixf(&viewMatrix[0][0]);
		}
		
	}
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		} 
		else {
			he = 110;
			wi = he * aspect;
		}
		
		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} 
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {

	}
}

//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################


	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################

}

void TrainView::
doPick()
//========================================================================
{
	glUseProgram(0);
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();

	// where is the mouse?
	int mx = Fl::event_x();
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);

	//drawColorUVFBO();
	//colorUVFBO->bind();
	//glReadBuffer(GL_COLOR_ATTACHMENT0);
	//glm::vec3 uv;
	//glReadPixels(Fl::event_x(), h() - Fl::event_y(), 1, 1, GL_RGB, GL_FLOAT, &uv[0]);

	//glReadBuffer(GL_NONE);
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	//colorUVFBO->unbind();
	//if (uv.b != 1.0) {
	//	cout << "uv.r = " << uv.r << " uv.g = " << uv.g << endl;
	//	updateInteractiveHeightMapFBO(1, glm::vec2(uv.r, uv.g));
	//}
}

void TrainView::setUBO()
{
	float wdt = this->pixel_w();
	float hgt = this->pixel_h();

	glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projectionMatrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &viewMatrix[0][0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void TrainView::updateTimer() {
	now_t = glutGet(GLUT_ELAPSED_TIME);
	delta_t = (now_t - old_t) / 1000.0;
	old_t = now_t;
}

void TrainView::updata_camera() {
	if (tw->fpv->value()) {
		if (k_pressed) {
			if (k == 'w') {
				camera.ProcessKeyboard(FORWARD, delta_t);
				damage(1);
			}
			if (k == 's') {
				camera.ProcessKeyboard(BACKWARD, delta_t);
				damage(1);
			}
			if (k == 'a') {
				camera.ProcessKeyboard(LEFT, delta_t);
				damage(1);
			}
			if (k == 'd') {
				camera.ProcessKeyboard(RIGHT, delta_t);
				damage(1);
			}
		}
	}
	//cout << camera.Position.x << " " << camera.Position.y << " " << camera.Position.z << endl;
}

void TrainView::update_light_shaders() {
	//set the selected lighting shader
	if (tw->lightBrowser->value() == 1) {
		current_light_shader = directional_light_shader;
	}
	else if (tw->lightBrowser->value() == 2) {
		current_light_shader = point_light_shader;
	}
	else if (tw->lightBrowser->value() == 3) {
		current_light_shader = spot_light_shader;
	}


	glm::mat4 model = glm::mat4(1.0f);

	//directional light
	if (tw->lightBrowser->value() == 1) {
		directional_light_shader->use();
		directional_light_shader->setVec3("light.direction", -1.0f, -0.1f, -0.3f);
		directional_light_shader->setVec3("viewPos", camera.Position);
		// light properties
		directional_light_shader->setVec3("light.ambient", 0.1f, 0.1f, 0.1f);
		directional_light_shader->setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
		directional_light_shader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);
		// material properties
		directional_light_shader->setFloat("material.shininess", 32.0f);
		// view/projection transformations
		// world transformation
		//model = glm::mat4(1.0f);
		//model = glm::scale(model, glm::vec3(5.0, 5.0, 5.0));
		//directional_light_shader->setMat4("model", model);
	}

	//point light
	if (tw->lightBrowser->value() == 2) {
		point_light_shader->use();
		glm::vec3 lightPos(35.0f, 100.0f, 2.0f);
		point_light_shader->setVec3("light.position", lightPos);
		point_light_shader->setVec3("viewPos", camera.Position);

		// light properties
		point_light_shader->setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
		point_light_shader->setVec3("light.diffuse", 0.9f, 0.9f, 0.9f);
		point_light_shader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);
		point_light_shader->setFloat("light.constant", 1.0f);
		point_light_shader->setFloat("light.linear", 0.001f);
		point_light_shader->setFloat("light.quadratic", 0.001f);

		// material properties
		point_light_shader->setFloat("material.shininess", 32.0f);

		// world transformation
		//model = glm::mat4(1.0f);
		//model = glm::scale(model, glm::vec3(5.0, 5.0, 5.0));
		//point_light_shader->setMat4("model", model);
	}

	//spot light
	if (tw->lightBrowser->value() == 3) {
		spot_light_shader->use();
		spot_light_shader->setVec3("light.position", camera.Position);
		spot_light_shader->setVec3("light.direction", camera.Front);
		spot_light_shader->setFloat("light.cutOff", glm::cos(glm::radians(12.5f)));
		spot_light_shader->setVec3("viewPos", camera.Position);
		// light properties
		spot_light_shader->setVec3("light.ambient", 0.1f, 0.1f, 0.1f);
		// we configure the diffuse intensity slightly higher; the right lighting conditions differ with each lighting method and environment.
		// each environment and lighting type requires some tweaking to get the best out of your environment.
		spot_light_shader->setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
		spot_light_shader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);
		spot_light_shader->setFloat("light.constant", 1.0f);
		spot_light_shader->setFloat("light.linear", 0.05f);
		spot_light_shader->setFloat("light.quadratic", 0.01f);
		// material properties
		spot_light_shader->setFloat("material.shininess", 32.0f);

		// world transformation
		//model = glm::mat4(1.0f);
		//model = glm::scale(model, glm::vec3(5.0, 5.0, 5.0));
		//spot_light_shader->setMat4("model", model);
	}
}

void TrainView::drawGround() {
	//bind ground texture
	ground_texture->bind(0);
	// world transformation
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(500.0, 1.0, 500.0));
	current_light_shader->setMat4("model", model);

	//bind VAO and draw plane
	glBindVertexArray(this->plane->vao);
	glDrawElements(GL_TRIANGLES, this->plane->element_amount, GL_UNSIGNED_INT, 0);
	ground_texture->unbind(0);
}

void TrainView::drawTeapot() {

	// world transformation
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(100, 100, 0));
	model = glm::scale(model, glm::vec3(10, 10, 10));
	current_light_shader->setMat4("model", model);

	teapot->Draw(*current_light_shader);
}

void TrainView::drawWater(int mode) {
	glm::mat4 model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(0, 10, 0));
	model = glm::scale(model, glm::vec3(10,1,10));

	waterMesh->setEyePos(camera.Position);
	waterMesh->setMVP(model, viewMatrix, projectionMatrix);
	waterMesh->addTime(delta_t);

	waterMesh->amplitude_coefficient = tw->waterAmplitude->value();
	waterMesh->waveLength_coefficient = tw->waterWaveLength->value();
	waterMesh->speed_coefficient = tw->waterSpeed->value();

	if (mode == 3) {
		if (firstDraw) {
			updateInteractiveHeightMapFBO(0);
			updateInteractiveHeightMapFBO(0);
			firstDraw = false;
		}
		else {
			updateInteractiveHeightMapFBO(2);
		}
		
		
		if (currentFBO == 0) {
			waterMesh->interactiveTexId = interactiveHeightMapFBO0->getColorId();
		}
		else {
			waterMesh->interactiveTexId = interactiveHeightMapFBO1->getColorId();
		}
		mainFBO->bind();
	}

	waterMesh->draw(mode);
	glUseProgram(0);
}

void TrainView::drawSkyBox() {
	glm::mat4 model = glm::mat4(1.0);

	skyBox->setMVP(model, viewMatrix, projectionMatrix);
	skyBox->draw();

	glUseProgram(0);
}

void TrainView::loadShaders() {
	if (!directional_light_shader) {
		directional_light_shader = new Shader("../src/shaders/directional_light.vert", "../src/shaders/directional_light.frag");
	}

	if (!point_light_shader) {
		point_light_shader = new Shader("../src/shaders/point_light.vert", "../src/shaders/point_light.frag");
	}

	if (!spot_light_shader) {
		spot_light_shader = new Shader("../src/shaders/spot_light.vert", "../src/shaders/spot_light.frag");
	}

	if (!light_source_shader) {
		light_source_shader = new Shader("../src/shaders/light_cube.vert", "../src/shaders/light_cube.frag");
	}

	if (!mainScreen_shader) {
		mainScreen_shader = new Shader("../src/shaders/main_screen.vert", "../src/shaders/main_screen.frag");
	}

	if (!subScreen_shader) {
		subScreen_shader = new Shader("../src/shaders/sub_screen.vert", "../src/shaders/sub_screen.frag");
	}

	if (!interactiveHeightMap_shader) {
		interactiveHeightMap_shader = new Shader("../src/shaders/interactive_heightmap.vert", "../src/shaders/interactive_heightmap.frag");
	}
	
}

void TrainView::loadModels() {
	if (!sci_fi_train) {
		sci_fi_train = new Model(FileSystem::getPath("resources/objects/Sci_fi_Train/Sci_fi_Train.obj"));
	}
	if (!teapot) {
		teapot = new Model(FileSystem::getPath("resources/objects/teapot/teapot.obj"));
	}
}

void TrainView::loadTextures() {
	if (!ground_texture)
		ground_texture = new Texture2D("../Images/black_white_board.png");
	if (!water_texture)
		water_texture = new Texture2D("../Images/blue.png");
}

void TrainView::loadWaterMesh() {
	if (!waterMesh) {
		waterMesh = new WaterMesh(glm::vec3(0.0, 20.0, 0.0));
	}
}

void TrainView::loadSkyBox() {
	if (!skyBox) {
		skyBox = new SkyBox();
	}
}

void TrainView::initVAOs() {
	if (!this->mainScreenVAO) {
		float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates. NOTE that this plane is now much smaller and at the top left of the screen
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f,  -1.0f,  0.0f, 0.0f,
			 1.0f,  -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f,  -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};

		mainScreenVAO = new VAO;
		mainScreenVAO->count = 6;
		glGenVertexArrays(1, &mainScreenVAO->vao);
		glGenBuffers(1, &mainScreenVAO->vbo[0]);
		glBindVertexArray(mainScreenVAO->vao);
		glBindBuffer(GL_ARRAY_BUFFER, mainScreenVAO->vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		// Unbind VAO
		glBindVertexArray(0);
	}

	if (!this->subScreenVAO) {
		float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates. NOTE that this plane is now much smaller and at the top left of the screen
			// positions   // texCoords
			-0.5f,  1.0f,  0.0f, 1.0f,
			-0.5f,  0.5f,  0.0f, 0.0f,
			 0.0f,  0.5f,  1.0f, 0.0f,

			-0.5f,  1.0f,  0.0f, 1.0f,
			 0.0f,  0.5f,  1.0f, 0.0f,
			 0.0f,  1.0f,  1.0f, 1.0f
		};

		subScreenVAO = new VAO;
		subScreenVAO->count = 6;
		glGenVertexArrays(1, &subScreenVAO->vao);
		glGenBuffers(1, &subScreenVAO->vbo[0]);
		glBindVertexArray(subScreenVAO->vao);
		glBindBuffer(GL_ARRAY_BUFFER, subScreenVAO->vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		// Unbind VAO
		glBindVertexArray(0);
	}

	if (!this->interactiveHeightMapVAO) {
		float quadVertices[] = {
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f,  -1.0f,  0.0f, 0.0f,
			 1.0f,  -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f,  -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};

		interactiveHeightMapVAO = new VAO;
		interactiveHeightMapVAO->count = 6;
		glGenVertexArrays(1, &interactiveHeightMapVAO->vao);
		glGenBuffers(1, &interactiveHeightMapVAO->vbo[0]);
		glBindVertexArray(interactiveHeightMapVAO->vao);
		glBindBuffer(GL_ARRAY_BUFFER, interactiveHeightMapVAO->vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		// Unbind VAO
		glBindVertexArray(0);
	}
}

void TrainView::initFBOs() {
	if (!mainFBO) {
		mainFBO = new FrameBuffer();
		mainFBO->init(TEXTURE_WIDTH, TEXTURE_HEIGHT);
	}

	if (!subScreenFBO) {
		subScreenFBO = new FrameBuffer();
		subScreenFBO->init(TEXTURE_WIDTH, TEXTURE_HEIGHT);
	}

	if (!colorUVFBO) {
		colorUVFBO = new FrameBuffer();
		colorUVFBO->init(TEXTURE_WIDTH, TEXTURE_HEIGHT);
	}

	if (!interactiveHeightMapFBO0) {
		interactiveHeightMapFBO0 = new FrameBuffer();
		interactiveHeightMapFBO0->init(TEXTURE_WIDTH, TEXTURE_HEIGHT);
	}

	if (!interactiveHeightMapFBO1) {
		interactiveHeightMapFBO1 = new FrameBuffer();
		interactiveHeightMapFBO1->init(TEXTURE_WIDTH, TEXTURE_HEIGHT);
	}
}

void TrainView::drawMainFBO() {
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
	// set the rendering destination to FBO
	mainFBO->bind();
	// clear buffer
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawTeapot();

	int waterType = tw->waveTypeBrowser->value();
	drawWater(waterType);

	glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
	drawSkyBox();
	glDepthFunc(GL_LESS); // set depth function back to default

	glBindVertexArray(0);

	// if MSAA is on, explicitly copy multi-sample color/depth buffers to single-sample
	// it also generates mipmaps of color texture object
	mainFBO->update();

	// back to normal window-system-provided framebuffer
	mainFBO->unbind();
}

void TrainView::drawSubScreenFBO() {
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
	// set the rendering destination to FBO
	subScreenFBO->bind();
	// clear buffer
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawTeapot();

	int waterType = tw->waveTypeBrowser->value();
	drawWater(waterType);

	drawSkyBox();
	glBindVertexArray(0);

	// if MSAA is on, explicitly copy multi-sample color/depth buffers to single-sample
	// it also generates mipmaps of color texture object
	subScreenFBO->update();

	// back to normal window-system-provided framebuffer
	subScreenFBO->unbind();
}

void TrainView::drawColorUVFBO() {
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
	// set the rendering destination to FBO
	colorUVFBO->bind();
	// clear buffer
	glClearColor(0, 0, 1.0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawWater(4);

	glBindVertexArray(0);

	// if MSAA is on, explicitly copy multi-sample color/depth buffers to single-sample
	// it also generates mipmaps of color texture object
	colorUVFBO->update();

	// back to normal window-system-provided framebuffer
	colorUVFBO->unbind();
}

void TrainView::updateInteractiveHeightMapFBO(int mode, glm::vec2 u_center) {
	//mode 0: initialization, mode 1: drop, mode 2: update
	// set the rendering destination to FBO
	FrameBuffer* lastfbo;
	FrameBuffer* currentfbo;
	if (currentFBO == 0) {
		lastfbo = interactiveHeightMapFBO1;
		currentfbo = interactiveHeightMapFBO0;
		currentFBO = 1;
	}
	else {
		lastfbo = interactiveHeightMapFBO0;
		currentfbo = interactiveHeightMapFBO1;
		currentFBO = 0;
	}

	currentfbo->bind();

	glDisable(GL_DEPTH_TEST);
	// clear buffer
	glClearColor(0.0, 0.9, 0.0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	interactiveHeightMap_shader->use();
	if (mode == 0) { // initialization
		interactiveHeightMap_shader->setInt("mode", 0);
		interactiveHeightMap_shader->setInt("u_water", 1);
		interactiveHeightMap_shader->setVec2("u_center", u_center);
		interactiveHeightMap_shader->setFloat("u_radius", 0.01f);
		interactiveHeightMap_shader->setFloat("u_strength", 1.0f);
	}
	else if (mode == 1) { // drop
		interactiveHeightMap_shader->setInt("mode", 1);
		interactiveHeightMap_shader->setInt("u_water", 1);
		interactiveHeightMap_shader->setVec2("u_center", u_center);
		interactiveHeightMap_shader->setFloat("u_radius", 0.01f);
		interactiveHeightMap_shader->setFloat("u_strength", 1.0f);

	}
	else if (mode == 2) { // update
		interactiveHeightMap_shader->setInt("mode", 2);
		interactiveHeightMap_shader->setInt("u_water", 1);
		interactiveHeightMap_shader->setVec2("u_center", u_center);
		interactiveHeightMap_shader->setFloat("u_radius", 0.01f);
		interactiveHeightMap_shader->setFloat("u_strength", 1.0f);
	}
	glBindVertexArray(interactiveHeightMapVAO->vao);
	glActiveTexture(GL_TEXTURE0+1);
	glBindTexture(GL_TEXTURE_2D, lastfbo->getColorId());
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	currentfbo->update();
	currentfbo->unbind();
	glEnable(GL_DEPTH_TEST);
}

void TrainView::drawMainScreen() {
	glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.

	mainScreen_shader->use();
	mainScreen_shader->setFloat("vx_offset", 0.5);
	mainScreen_shader->setFloat("rt_w", w());
	mainScreen_shader->setFloat("rt_h", h());
	mainScreen_shader->setFloat("pixel_w", 10.0);
	mainScreen_shader->setFloat("pixel_h", 10.0);
	//mainScreen_shader->setBool("doPixelation", tw->pixelation->value());
	//mainScreen_shader->setBool("doOffset", tw->offset->value());
	//mainScreen_shader->setBool("doGrayscale", tw->grayscale->value());
	

	glBindVertexArray(mainScreenVAO->vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mainFBO->getColorId());
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void TrainView::drawSubScreen() {
	glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.

	subScreen_shader->use();
	glBindVertexArray(subScreenVAO->vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, interactiveHeightMapFBO1->getColorId());
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

//find 4 control point around current point
std::vector<Pnt3f> TrainView::find_Cpoints(int currentPoint) {
#ifdef DEBUG

#endif // DEBUG

	Pnt3f cp_pos_p0;
	if (currentPoint == 0) {
		cp_pos_p0 = m_pTrack->points[(m_pTrack->points.size() - 1)].pos;
		//cout << "cp0: " << m_pTrack->points.size() - 1;
	}
	else {
		cp_pos_p0 = m_pTrack->points[(currentPoint - 1)].pos;
		//cout << "cp0: " << currentPoint - 1;
	}
	Pnt3f cp_pos_p1 = m_pTrack->points[currentPoint].pos;
	//cout << " cp1: " << currentPoint;
	Pnt3f cp_pos_p2 = m_pTrack->points[(currentPoint + 1) % m_pTrack->points.size()].pos;
	//cout << " cp2: " << (currentPoint + 1) % m_pTrack->points.size();
	Pnt3f cp_pos_p3 = m_pTrack->points[(currentPoint + 2) % m_pTrack->points.size()].pos;
	//cout << " cp3: " << (currentPoint + 2) % m_pTrack->points.size();
	//cout << endl;



	std::vector<Pnt3f> output = { cp_pos_p0, cp_pos_p1, cp_pos_p2, cp_pos_p3 };
	return output;

}

//find 4 orient vector around current point
std::vector<Pnt3f> TrainView::find_orient_vectors(int currentPoint) {
#ifdef DEBUG

#endif // DEBUG

	Pnt3f orient0;
	if (currentPoint == 0) {
		orient0 = m_pTrack->points[(m_pTrack->points.size() - 1)].orient;
	}
	else {
		orient0 = m_pTrack->points[(currentPoint - 1)].orient;
	}
	Pnt3f orient1 = m_pTrack->points[currentPoint].orient;
	Pnt3f orient2 = m_pTrack->points[(currentPoint + 1) % m_pTrack->points.size()].orient;
	Pnt3f orient3 = m_pTrack->points[(currentPoint + 2) % m_pTrack->points.size()].orient;

	std::vector<Pnt3f> output = { orient0, orient1, orient2, orient3 };
	return output;

}

//update track data 
void TrainView::update_arcLengh() {
	size_t size = DIVIDE_LINE * m_pTrack->points.size();

	glm::vec3 default_vec3(0, 0, 0);
	t_param.assign(size, 0);
	arc_length.assign(size, 0);
	accumulate_length.assign(size, 0);
	speeds.assign(size, 0);
	all_qt.assign(size, default_vec3);
	all_orient.assign(size, default_vec3);
	all_forward.assign(size, default_vec3);

	//initialize t_param
	for (int i = 0; i < size; ++i) {
		int fraction = i % 100;
		t_param[i] = float(fraction) / float(DIVIDE_LINE);
	}

	//setup arcLength and all qt, orient and forward
	for (int i = 0; i < m_pTrack->points.size(); ++i) {
		Pnt3f cp_orient_p0 = m_pTrack->points[i].orient;
		Pnt3f cp_orient_p1 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].orient;

		vector<Pnt3f> points = find_Cpoints(i);

		for (size_t j = 0; j < DIVIDE_LINE; j++) {
			size_t current_index = i * DIVIDE_LINE + j;
			float t0 = t_param[current_index];

			vector<Pnt3f> qts = find_two_qt(tw->splineBrowser->value(), points, t0);

			glm::vec3 qt0_v(qts[0].x, qts[0].y, qts[0].z);
			glm::vec3 qt1_v(qts[1].x, qts[1].y, qts[1].z);
			glm::vec3 forward = qt1_v - qt0_v;

			//forward 可以改一下，遇到邊界無法跨度兩個線段
			forward = glm::normalize(forward);

			Pnt3f orient_t0 = find_orient(cp_orient_p0, cp_orient_p1, t0);
			glm::vec3 orient_t0_v(orient_t0.x, orient_t0.y, orient_t0.z);
			orient_t0_v = glm::normalize(orient_t0_v);

			float length = glm::distance(qt0_v, qt1_v);
			arc_length[current_index] = length;
			if (current_index == 0) accumulate_length[current_index] = length;
			else  accumulate_length[current_index] = accumulate_length[current_index - 1] + length;
			all_qt[current_index] = qt0_v;
			all_orient[current_index] = orient_t0_v;
			all_forward[current_index] = forward;
		}
	}

	//setup speeds
	float total_energy = 2000.0f;
	float kinetic_energy = 2000.0f;
	float potential_energy = 0.0f;
	float h_coefficient = 20.0f;
	float current_h = all_qt[0].y;
	speeds[0] = 0.05 * sqrt(kinetic_energy);
	for (int i = 1; i < size; ++i) {
		float h_diff = all_qt[i].y - all_qt[i - 1].y;
		potential_energy = potential_energy + h_coefficient * h_diff;
		kinetic_energy = total_energy - potential_energy;
		if (kinetic_energy <= 50) {
			total_energy += 50.0f;
			kinetic_energy = 50.0f;
		}
		//cout << "p: " << potential_energy << endl;
		float current_speed = 0.05 * sqrt(kinetic_energy);
		speeds[i] = current_speed;
	}

}

//use current length to find parameter t
float TrainView::length_to_t(float length) {
	for (size_t i = 0; i < accumulate_length.size() - 1; ++i) {
		if (length < accumulate_length[i]) {
			return t_param[i];
		}
	}
	return t_param.back();
}

//use given length to find current index
size_t TrainView::length_to_index(float length) {

	if (length < 0) {
		length = accumulate_length.back() + length;
	}
	for (size_t i = 0; i < accumulate_length.size() - 1; ++i) {
		if (length < accumulate_length[i]) {
			return i;
		}
	}
	return accumulate_length.size() - 1;
}

//find current parameter t by C_length
size_t TrainView::C_length_index() {
	int index = -1;
	for (size_t i = 0; i < accumulate_length.size() - 1; ++i) {
		if (m_pTrack->C_length < accumulate_length[i]) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		index = accumulate_length.size() - 1;
	}
	return index;
}

//find current index by trainU
size_t TrainView::trainU_index() {
	int index = int(m_pTrack->trainU / 1) * DIVIDE_LINE;
	index = index + (m_pTrack->trainU - int(m_pTrack->trainU / 1)) * DIVIDE_LINE;
	if (index == t_param.size()) --index;
	return index;
}

//match current length and trainU
void TrainView::match_length() {
	int index = C_length_index();

	m_pTrack->trainU = (index / DIVIDE_LINE) + length_to_t(m_pTrack->C_length);
	//cout << "index: " << index << endl;
	//cout << "length_to_t(m_pTrack->C_length): " << length_to_t(m_pTrack->C_length) << endl;
	//cout << "C_length: " << m_pTrack->C_length << endl;
	//cout << " trainU: " << m_pTrack->trainU << endl;

}

//match current trainU to length 
void TrainView::match_trainU() {
	int index = trainU_index();
	m_pTrack->C_length = accumulate_length[index];
}