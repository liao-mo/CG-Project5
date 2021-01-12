#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>

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

// Constructor to set up the GL window
TrainView::
TrainView(int x, int y, int w, int h, const char* l) :
	Fl_Gl_Window(x, y, w, h, l)
//========================================================================
{
	cout << "Initializing..." << endl;
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
	camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
	camera.MovementSpeed = 1000.0f;
	camera.Position = glm::vec3(50.0, 100.0, 0.0);
	old_t = glutGet(GLUT_ELAPSED_TIME);
	k_pressed = false;
	initLightSource();

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
			update_arcLengh();
			damage(1);
		}
		// Compute the new tree position
		if ((last_push == FL_LEFT_MOUSE) && (selectedTree >= 0)) {
			Base_Object* alter_tree = &my_trees[selectedTree];

			double r1x, r1y, r1z, r2x, r2y, r2z;
			getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

			double rx, ry, rz;
			mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z,
				static_cast<double>(alter_tree->pos.x),
				static_cast<double>(alter_tree->pos.y),
				static_cast<double>(alter_tree->pos.z),
				rx, ry, rz,
				(Fl::event_state() & FL_CTRL) != 0);

			alter_tree->pos.x = (float)rx;
			alter_tree->pos.y = (float)ry;
			alter_tree->pos.z = (float)rz;
			my_trees[selectedTree].update_modelMatrix();
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

			if (tw->fpv->value() || tw->trainCam->value()) {
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
			else if (selectedTree >= 0) {
				cout << "Tree " << selectedTree << " x, y, z = " << my_trees[selectedTree].pos.x << ", " << my_trees[selectedTree].pos.y << ", " << my_trees[selectedTree].pos.z << endl;
			}
			else
				printf("Nothing Selected\n");

			return 1;
		};
		if (k == 'f') {
			launchCannon();
		}
		if (k == 'm') {
			cout << "camera position: " << camera.Position.x << ", " << camera.Position.y << ", " << camera.Position.z << endl;
		}
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
		loadShaders();
		initParticleSystem();
		loadWaterMesh();
		loadflagMesh();
		initTitans();
		initCannons();
		initTextRender();
		loadModels();
		loadSkyBox();
		initFBOs();
		initVAOs();
		//original functions
		initUBO();
		initPlane();
		loadTextures();
		initTrackData();
		initSound();
		initRun();
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	//glViewport(0,0,w(),h());
	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,0,0);
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
	projectionMatrix = glm::mat4(1.0);
	setProjection();		// put the code to set up matrices here

	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	// set linstener position 
	updateListenerPos();	
	// now draw the ground plane
	glUseProgram(0);
	setupObjects();
	drawStuff();

	//use shader===========================================================================================================
	setUBO();
	glBindBufferRange(GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);
	//update current light_shader
	update_light_shaders();
	drawMainFBO();

	//draw main FBO to the whole screen
	drawMainScreen();

	glBindVertexArray(0);
	glUseProgram(0);

	//other stuff
	checkScoreAndTime();
}

void TrainView::
setProjection()
{
	glUseProgram(0);
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());
	projectionMatrix = projectionMatrix * glm::perspective(glm::radians(camera.Zoom), (float)w() / (float)h(), (float)NEAR, (float)FAR);

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

	// put code for train view projection here!	
	else {
		size_t i;
		if (tw->arcLength->value() == 0) {
			i = trainU_index();
		}
		else {
			i = C_length_index();
		}
		float t0 = t_param[i];

		glm::vec3 qt0_v = all_qt[i];
		glm::vec3 qt1_v;
		if (i == t_param.size() - 1) qt1_v = all_qt[0];
		else qt1_v = all_qt[i + 1];

		glm::vec3 orient_t0_v = all_orient[i];
		glm::vec3 forward = all_forward[i];


		float FPV_up_value = 20.0f;
		float TPV_up_value = 15.0f;
		float TPV_backward_value = 70.0f;

		glm::vec4 eye(qt0_v.x, qt0_v.y, qt0_v.z, 1);
		glm::vec4 center(qt0_v.x + forward.x, qt0_v.y + forward.y, qt0_v.z + forward.z, 1);
		glm::vec3 offset0(orient_t0_v.x, orient_t0_v.y, orient_t0_v.z);
		glm::mat4 trans = glm::mat4(1.0f);

		if (tw->FPV->value() == 1) {
			trans = glm::translate(trans, FPV_up_value * offset0);
			eye = trans * eye;

			center = trans * center;
			glMatrixMode(GL_PROJECTION);
			glMultMatrixf(&projectionMatrix[0][0]);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(
				eye.x,
				eye.y,
				eye.z,
				center.x,
				center.y,
				center.z,
				orient_t0_v.x,
				orient_t0_v.y,
				orient_t0_v.z
			);
			viewMatrix = glm::lookAt(
				glm::vec3(eye.x,eye.y,eye.z),
				glm::vec3(center.x,center.y,center.z),
				glm::vec3(orient_t0_v.x,orient_t0_v.y,orient_t0_v.z));

			float upOffset = 5.0f;
			camera.Position = glm::vec3(eye.x, eye.y + upOffset, eye.z);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			viewMatrix = camera.GetViewMatrix();
			glMultMatrixf(&viewMatrix[0][0]);
		}
		else if (tw->TPV->value() == 1) {
			trans = glm::translate(trans, TPV_up_value * offset0);
			trans = glm::translate(trans, TPV_backward_value * -forward);
			eye = trans * eye;

			center = trans * center;
			glMatrixMode(GL_PROJECTION);
			glMultMatrixf(&projectionMatrix[0][0]);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(
				eye.x,
				eye.y,
				eye.z,
				center.x,
				center.y,
				center.z,
				orient_t0_v.x,
				orient_t0_v.y,
				orient_t0_v.z
			);
			viewMatrix = glm::lookAt(
				glm::vec3(eye.x, eye.y, eye.z),
				glm::vec3(center.x, center.y, center.z),
				glm::vec3(orient_t0_v.x, orient_t0_v.y, orient_t0_v.z));
		}
	}
}

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
	glLoadIdentity();
	projectionMatrix = glm::mat4(1.0);
	gluPickMatrix((double)mx, (double)(viewport[3] - my),
		5, 5, viewport);
	float projection_temp[16];
	glGetFloatv(GL_PROJECTION_MATRIX, projection_temp);
	projectionMatrix = glm::make_mat4(projection_temp);


	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100, buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
		glLoadName((GLuint)(i + 1));
		m_pTrack->points[i].draw();
	}

	if (tw->pickObjects->value()) {
		standard_shader->use();
		for (int i = 0; i < my_trees.size(); ++i) {
			glLoadName((GLuint)(m_pTrack->points.size() + i + 1));
			standard_shader->setMat4("model", my_trees[i].model_matrix);
			my_trees[i].model->Draw(*standard_shader);
		}
		glUseProgram(0);
	}


	// go back to drawing mode, and see how picking did
	selectedCube = -1;
	selectedTree = -1;
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedObject = buf[3] - 1;
	}
	else // nothing hit, nothing selected
		selectedObject = -1;

	if (selectedObject < m_pTrack->points.size()) { // hit control points
		selectedCube = selectedObject;
		printf("Selected Cube %d\n", selectedCube);
	}
	else if (selectedObject < m_pTrack->points.size() + my_trees.size()) { // hit trees
		selectedTree = selectedObject - m_pTrack->points.size();
		cout << "you hit tree " << selectedTree << endl;
	}
	else {
		cout << "you hit nothing!" << endl;
	}


	

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
	if (tw->runButton->value()) {
		now_t = glutGet(GLUT_ELAPSED_TIME);
		delta_t = (now_t - old_t) / 1000.0;
		old_t = now_t;
	}
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
	standard_shader->use();
	standard_shader->setInt("material.diffuse", 0);
	standard_shader->setInt("material.specular", 1);
	standard_shader->setVec3("viewPos", camera.Position);
	standard_shader->setFloat("material.shininess", 32.0f);

	for (int i = 0; i < NR_LIGHT; ++i) {
		string dir_number = to_string(i);
		string point_number = to_string(i - 4);
		if (light_sources[i].type == DIRECTIONAL_LIGHT) {
			standard_shader->setVec3("dirLight[" + dir_number + "].direction", light_sources[i].direction);
			standard_shader->setVec3("dirLight[" + dir_number + "].ambient", light_sources[i].ambient);
			standard_shader->setVec3("dirLight[" + dir_number + "].diffuse", light_sources[i].diffuse);
			standard_shader->setVec3("dirLight[" + dir_number + "].specular", light_sources[i].specular);
		}
		else if (light_sources[i].type == POINT_LIGHT) {
			standard_shader->setVec3("pointLights[" + point_number + "].position", light_sources[i].position);
			standard_shader->setVec3("pointLights[" + point_number + "].ambient", light_sources[i].ambient);
			standard_shader->setVec3("pointLights[" + point_number + "].diffuse", light_sources[i].diffuse);
			standard_shader->setVec3("pointLights[" + point_number + "].specular", light_sources[i].specular);
			standard_shader->setFloat("pointLights[" + point_number + "].constant", light_sources[i].constant);
			standard_shader->setFloat("pointLights[" + point_number + "].linear", light_sources[i].linear);
			standard_shader->setFloat("pointLights[" + point_number + "].quadratic", light_sources[i].quadratic);
		}
		else if (light_sources[i].type == SPOT_LIGHT) {
			standard_shader->setVec3("spotLight.position", light_sources[i].position);
			standard_shader->setVec3("spotLight.direction", light_sources[i].direction);
			standard_shader->setVec3("spotLight.ambient", light_sources[i].ambient);
			standard_shader->setVec3("spotLight.diffuse", light_sources[i].diffuse);
			standard_shader->setVec3("spotLight.specular", light_sources[i].specular);
			standard_shader->setFloat("spotLight.constant", light_sources[i].constant);
			standard_shader->setFloat("spotLight.linear", light_sources[i].linear);
			standard_shader->setFloat("spotLight.quadratic", light_sources[i].quadratic);
			standard_shader->setFloat("spotLight.cutOff", light_sources[i].cutOff);
			standard_shader->setFloat("spotLight.outerCutOff", light_sources[i].outerCutOff);
		}
	}
	glUseProgram(0);
}

void TrainView::drawGround() {
	//bind ground texture
	ground_texture->bind(0);
	// world transformation
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(500.0, 1.0, 500.0));
	standard_shader->setMat4("model", model);

	//bind VAO and draw plane
	glBindVertexArray(this->plane->vao);
	glDrawElements(GL_TRIANGLES, this->plane->element_amount, GL_UNSIGNED_INT, 0);
	ground_texture->unbind(0);
}

void TrainView::draw_track() {
	standard_shader->use();

	//draw track with respect to my t_param and qt vectors data
	for (int i = 0; i < t_param.size(); ++i) {
		glm::vec3 qt0_v = all_qt[i];
		glm::vec3 qt1_v;
		if (i == t_param.size() - 1) qt1_v = all_qt[0];
		else qt1_v = all_qt[i + 1];

		glm::vec3 orient_t0_v = all_orient[i];

		glLineWidth(5);
		float scale_value = 2.0f;

		mat4 rotate = glm::inverse(glm::lookAt(
			qt0_v,
			qt1_v,
			orient_t0_v));

		float up_offset = -0.3f;
		//glm::vec3 side_offset_v = glm::cross(forward, orient_t0_v);
		//side_offset_v = glm::normalize(side_offset_v);
		//side_offset_v *= 1.5f;

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(up_offset * orient_t0_v.x, up_offset * orient_t0_v.y, up_offset * orient_t0_v.z));
		//model = glm::translate(model, glm::vec3(side_offset_v.x, side_offset_v.y, side_offset_v.z));
		model = rotate * model;
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
		model = glm::scale(model, glm::vec3(scale_value, scale_value, scale_value));

		standard_shader->setMat4("model", model);
		my_track->Draw(*standard_shader);
		glLineWidth(1);
	}
	glUseProgram(0);
}

void TrainView::draw_sleeper() {
	standard_shader->use();

	float current_length = 0;
	while (current_length < accumulate_length.back()) {
		size_t i = length_to_index(current_length);
		glm::vec3 qt0_v = all_qt[i];
		glm::vec3 qt1_v;
		if (i == t_param.size() - 1) qt1_v = all_qt[0];
		else qt1_v = all_qt[i + 1];

		glm::vec3 orient_t0_v = all_orient[i];

		float scale_value = 8.0f;
		mat4 rotate = glm::inverse(glm::lookAt(
			qt0_v,
			qt1_v,
			orient_t0_v));
		float up_offset = -1.5f;

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(up_offset * orient_t0_v.x, up_offset * orient_t0_v.y, up_offset * orient_t0_v.z));
		//model = glm::translate(model, glm::vec3(side_offset_v.x, side_offset_v.y, side_offset_v.z));
		model = rotate * model;
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
		model = glm::scale(model, glm::vec3(scale_value, scale_value, scale_value));

		standard_shader->setMat4("model", model);
		my_sleeper->Draw(*standard_shader);
		current_length += 20;
	}
	glUseProgram(0);
}

void TrainView::draw_train() {
	standard_shader->use();

	size_t i;
	if (tw->arcLength->value() == 0) {
		i = int(m_pTrack->trainU / 1) * DIVIDE_LINE;
		i = i + (m_pTrack->trainU - int(m_pTrack->trainU / 1)) * DIVIDE_LINE - 0.1;
		if (i < 0) {
			i = (float)m_pTrack->points.size() - i;
		}
	}
	else {
		i = C_length_index();
	}

	glm::vec3 qt0_v = all_qt[i];
	glm::vec3 qt1_v;
	if (i == t_param.size() - 1) qt1_v = all_qt[0];
	else qt1_v = all_qt[i + 1];

	glm::vec3 orient_t0_v = all_orient[i];
	glm::vec3 forward = all_forward[i];

	float up_offset = 0.0f;
	float scale_value = 2.0f;

	mat4 rotate = glm::inverse(glm::lookAt(qt0_v, qt1_v, orient_t0_v));

	// world transformation
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(up_offset * orient_t0_v.x, up_offset * orient_t0_v.y, up_offset * orient_t0_v.z));
	model = rotate * model;
	//model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(scale_value, scale_value, scale_value));

	standard_shader->setMat4("model", model);
	sci_fi_train->Draw(*standard_shader);

	//draw its wings
	float wing_up = 3.0f;
	float wing_forward = 2.0f;
	model = glm::translate(model, glm::vec3(wing_up * orient_t0_v.x, wing_up * orient_t0_v.y, wing_up * orient_t0_v.z));
	wingRotateAngle += dAngle * 0.016;
	if (wingRotateAngle >= 45) {
		dAngle *= -1;
	}
	else if (wingRotateAngle <= -45) {
		dAngle *= -1;
	}

	glm::mat4 leftWingModel = model;
	leftWingModel = glm::rotate(leftWingModel, glm::radians(wingRotateAngle), glm::vec3(0, 0, 1));
	standard_shader->setMat4("model", leftWingModel);
	my_leftWing->Draw(*standard_shader);

	glm::mat4 rightWingModel = model;
	rightWingModel = glm::rotate(rightWingModel, glm::radians(-wingRotateAngle), glm::vec3(0, 0, 1));
	standard_shader->setMat4("model", rightWingModel);
	my_rightWing->Draw(*standard_shader);

	glUseProgram(0);


	//draw smode particles
	glm::vec3 particle_pos(qt0_v);
	float particle_up = 10.0;
	float particle_forward = 10.0;
	particle_pos = particle_pos + glm::vec3(particle_up * orient_t0_v.x, particle_up * orient_t0_v.y, particle_up * orient_t0_v.z);
	particle_pos = particle_pos + particle_forward * forward;
	glm::vec3 particle_dir(0, 20, 0);
	drawParticles(particle_pos, 5, 0.5, -4, 3, particle_dir);
}

void TrainView::draw_cars() {
	standard_shader->use();

	float first_offset = 20.0f;
	for (int i = 0; i < num_of_car; ++i) {
		float backward_offset = first_offset + 15.0 * i;

		if (!(tw->trainCam->value() == 1 && tw->FPV->value() == 1)) {
			size_t i;
			i = length_to_index(m_pTrack->C_length - backward_offset);
			float t0 = t_param[i];

			glm::vec3 qt0_v = all_qt[i];
			glm::vec3 qt1_v;
			if (i == t_param.size() - 1) qt1_v = all_qt[0];
			else qt1_v = all_qt[i + 1];

			glm::vec3 orient_t0_v = all_orient[i];
			glm::vec3 forward = all_forward[i];

			float scale_value = 1.0;
			float up_offset = 5.0f;

			mat4 rotate = glm::inverse(glm::lookAt(qt0_v, qt1_v, orient_t0_v));

			// world transformation
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(up_offset * orient_t0_v.x, up_offset * orient_t0_v.y, up_offset * orient_t0_v.z));
			model = rotate * model;
			model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
			model = glm::scale(model, glm::vec3(scale_value, scale_value, scale_value));

			standard_shader->setMat4("model", model);
			my_car->Draw(*standard_shader);
		}
	}
	glUseProgram(0);
}

void TrainView::draw_terrain() {
	standard_shader->use();

	// world transformation
	float scale_value = 10.0;
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0,-50,0));
	model = glm::scale(model, glm::vec3(scale_value, scale_value, scale_value));

	standard_shader->setMat4("model", model);
	my_terrain->Draw(*standard_shader);
	glUseProgram(0);
}

void TrainView::drawWater(int mode) {
	glm::mat4 model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(0, 0, 100));
	model = glm::scale(model, glm::vec3(12,1,14));

	waterMesh->setEyePos(camera.Position);
	waterMesh->setMVP(model, viewMatrix, projectionMatrix);
	waterMesh->addTime(delta_t);
	waterMesh->lightDir = -light_sources[4].position;

	waterMesh->amplitude_coefficient = tw->waterAmplitude->value();
	waterMesh->waveLength_coefficient = tw->waterWaveLength->value();
	waterMesh->speed_coefficient = tw->waterSpeed->value();

	waterMesh->draw(mode);
}

void TrainView::drawflag() {
	glm::mat4 model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(-2600, 1500, -2500));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
	model = glm::scale(model, glm::vec3(4, 4, 4));

	flagMesh->setEyePos(camera.Position);
	flagMesh->setMVP(model, viewMatrix, projectionMatrix);
	flagMesh->addTime(delta_t);
	flagMesh->lightDir = -light_sources[4].position;

	flagMesh->amplitude_coefficient = 1;
	flagMesh->waveLength_coefficient = 1;
	flagMesh->speed_coefficient = 1;

	flagMesh->draw(1);
}

void TrainView::drawTeapot() {
	standard_shader->use();
	float scale_value = 10.0;
	// world transformation
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-500, 0, 800));
	model = glm::scale(model, glm::vec3(scale_value));
	standard_shader->setMat4("model", model);

	teapot->Draw(*standard_shader);
	glUseProgram(0);
}

void TrainView::drawTrees() {
	standard_shader->use();
	for (int i = 0; i < my_trees.size(); ++i) {
		standard_shader->setMat4("model", my_trees[i].model_matrix);
		my_trees[i].model->Draw(*standard_shader);
	}
	glUseProgram(0);
}

void TrainView::drawTitans() {
	standard_shader->use();
	for (int i = 0; i < my_titans.size(); ++i) {
		standard_shader->setMat4("model", my_titans[i].model_matrix);
		my_titans[i].model->Draw(*standard_shader);
		my_titans[i].delta_t = delta_t;
		my_titans[i].move();
	}

	for (auto it1 = my_titans.begin(); it1 != my_titans.end(); ++it1) {
		for (auto it2 = my_cannons.begin(); it2 != my_cannons.end(); ++it2) {
			if (checkCollision(it1->pos, it1->radius, it2->pos,it2->radius)) {
				//delete the titan and the cannon
				cout << "hit!" << endl;
				my_titans.erase(it1--);
				my_cannons.erase(it2--);
				break;
			}
		}
	}
	glUseProgram(0);
}

void TrainView::drawCannons() {
	standard_shader->use();
	for (auto it = my_cannons.begin(); it != my_cannons.end(); ++it) {
		standard_shader->setMat4("model", it->model_matrix);
		it->model->Draw(*standard_shader);
		it->delta_t = delta_t;
		it->move();
		if (it->pos.y < -100) {
			//delete this cannon
			my_cannons.erase(it--);
		}
	}
	glUseProgram(0);
}

void TrainView::drawText() {
	if (playing) {
		textRender->setScreenSize(w(), h());
		string text = "There are " + to_string(my_titans.size()) + " titans still standing, let them sit down!";
		textRender->RenderText(text, 100.0f, 700.0, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
		text = "Press F to launch anti titan cannon!";
		textRender->RenderText(text, 100.0f, 600.0, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
		text = "Time left: " + to_string(game_time);
		textRender->RenderText(text, 100.0f, 500.0, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
	}
	if (win) {
		string text = "Congratulation! Now it's your time to ruin other countries...";
		textRender->RenderText(text, 100.0f, 700.0, 1.0f, glm::vec3(0.5, 0.0f, 0.0f));
	}
	if(lose) {
		string text = "You lose! Titans will eat your mom...";
		textRender->RenderText(text, 100.0f, 700.0, 1.0f, glm::vec3(0.5, 0.0f, 0.0f));
	}
}

void TrainView::drawLightObjects() {
	light_source_shader->use();
	for (int i = 0; i < my_light_objects.size(); ++i) {
		light_source_shader->setMat4("model", my_light_objects[i].model_matrix);
		my_light_objects[i].model->Draw(*light_source_shader);
	}
	glUseProgram(0);
}

void TrainView::drawCyborg() {
	normalMapping_shader->use();
	//standard_shader->use();
	float scale_value = 10.0;
	// world transformation
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, 0, 0));
	model = glm::scale(model, glm::vec3(scale_value));
	//standard_shader->setMat4("model", model);

	normalMapping_shader->setMat4("model", model);
	normalMapping_shader->setVec3("viewPos", camera.Position);
	normalMapping_shader->setVec3("lightPos", light_sources[4].position);
	normalMapping_shader->setInt("diffuseMap", 0);
	normalMapping_shader->setInt("normalMap", 2);
	my_cyborg->Draw(*normalMapping_shader);
	glUseProgram(0);
}

void TrainView::drawBrickWall() {
	// shader configuration
	// --------------------
	normalMapping_shader->use();
	normalMapping_shader->setInt("diffuseMap", 0);
	normalMapping_shader->setInt("normalMap", 1);
	// lighting info
	// -------------
	

	// render normal-mapped quad
	glm::mat4 model = glm::mat4(1.0f);
	//model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))); // rotate the quad to show normal mapping from multiple directions
	model = glm::translate(model, glm::vec3(0, 1000, 3000));
	model = glm::scale(model, glm::vec3(3300, 1000, 1000));
	normalMapping_shader->setMat4("model", model);
	normalMapping_shader->setVec3("viewPos", camera.Position);
	normalMapping_shader->setVec3("lightPos", light_sources[4].position);
	//normalMapping_shader->setVec3("lightPos", glm::vec3(0,0,0));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, brick_diffuseMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, brick_normalMap);
	renderQuad();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, 1000, -3000));
	model = glm::scale(model, glm::vec3(3300, 1000, 1000));
	normalMapping_shader->setMat4("model", model);
	renderQuad();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(3000, 1000, 0));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(3300, 1000, 1000));
	normalMapping_shader->setMat4("model", model);
	renderQuad();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-3000, 1000, 0));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(3300, 1000, 1000));
	normalMapping_shader->setMat4("model", model);
	renderQuad();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void TrainView::drawBrickWall2() {
	// shader configuration
// --------------------
	parallaxMapping_shader->use();
	parallaxMapping_shader->setInt("diffuseMap", 0);
	parallaxMapping_shader->setInt("normalMap", 1);
	parallaxMapping_shader->setInt("depthMap", 2);

	glm::vec3 lightPos = light_sources[0].direction;
	lightPos = glm::vec3(-390, 30, 0) + 100.0f * glm::normalize(lightPos);
	float heightScale = 0.1;

	parallaxMapping_shader->use();
	parallaxMapping_shader->setMat4("projection", projectionMatrix);
	parallaxMapping_shader->setMat4("view", viewMatrix);
	// render parallax-mapped quad
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-390, 30, 0));
	model = glm::scale(model, glm::vec3(100,100,100));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
	model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0, 1, 0));
	parallaxMapping_shader->setMat4("model", model);
	parallaxMapping_shader->setVec3("viewPos", camera.Position);
	parallaxMapping_shader->setVec3("lightPos", lightPos);
	parallaxMapping_shader->setFloat("heightScale", heightScale);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, brick2_diffuseMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, brick2_normalMap);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, brick2_heightMap);
	renderQuad();

	lightPos = glm::vec3(-390, 30, 200) + 100.0f * glm::normalize(lightPos);
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-390, 30, 200));
	model = glm::scale(model, glm::vec3(100, 100, 100));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
	model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0, 1, 0));
	parallaxMapping_shader->setMat4("model", model);
	parallaxMapping_shader->setVec3("viewPos", camera.Position);
	parallaxMapping_shader->setVec3("lightPos", lightPos);
	parallaxMapping_shader->setFloat("heightScale", heightScale);
	renderQuad();

	lightPos = glm::vec3(-390, 30, 400) + 100.0f * glm::normalize(lightPos);
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-390, 30, 400));
	model = glm::scale(model, glm::vec3(100, 100, 100));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
	model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0, 1, 0));
	parallaxMapping_shader->setMat4("model", model);
	parallaxMapping_shader->setVec3("viewPos", camera.Position);
	parallaxMapping_shader->setVec3("lightPos", lightPos);
	parallaxMapping_shader->setFloat("heightScale", heightScale);
	renderQuad();

	lightPos = glm::vec3(-390, 30, 600) + 100.0f * glm::normalize(lightPos);
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-390, 30, 600));
	model = glm::scale(model, glm::vec3(100, 100, 100));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
	model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0, 1, 0));
	parallaxMapping_shader->setMat4("model", model);
	parallaxMapping_shader->setVec3("viewPos", camera.Position);
	parallaxMapping_shader->setVec3("lightPos", lightPos);
	parallaxMapping_shader->setFloat("heightScale", heightScale);
	renderQuad();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void TrainView::drawParticles(glm::vec3 pos, double life, double size, double gravity, double spread, glm::vec3 dir) {
	particle_system->delta = delta_t;
	particle_system->setMatrix(projectionMatrix, viewMatrix);
	particle_system->update();

	particle_system->particle_position = pos;
	particle_system->particle_life = life;
	particle_system->particle_size = size;
	particle_system->particle_gravity = gravity;
	particle_system->particle_spread = spread;
	particle_system->particle_maindir = dir;
}

void TrainView::drawSkyBox() {
	glm::mat4 model = glm::mat4(1.0);

	skyBox->setMVP(model, viewMatrix, projectionMatrix);
	skyBox->draw();

	glUseProgram(0);
}

void TrainView::loadShaders() {
	if (!standard_shader) {
		standard_shader = new Shader("../src/shaders/standard_shader.vert", "../src/shaders/standard_shader.frag");
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
	if (!normalMapping_shader) {
		normalMapping_shader = new Shader("../src/shaders/normal_mapping.vert", "../src/shaders/normal_mapping.frag");
	}
	if (!parallaxMapping_shader) {
		parallaxMapping_shader = new Shader("../src/shaders/parallax_mapping.vert", "../src/shaders/parallax_mapping.frag");
	}
}

void TrainView::loadModels() {
	if (!my_track) {
		cout << "loading track..." << endl;
		my_track = new Model("../resources/objects/track/track.obj");
	}
	if (!my_sleeper) {
		cout << "loading sleeper..." << endl;
		my_sleeper = new Model("../resources/objects/sleeper/sleeper.obj");
	}
	if (!sci_fi_train) {
		cout << "loading train..." << endl;
		sci_fi_train = new Model("../resources/objects/Sci_fi_Train/Sci_fi_Train.obj");
	}
	if (!my_car) {
		cout << "loading train car..." << endl;
		//my_car = new Model("../resources/objects/train_car/train_car.obj"));
		my_car = new Model("../resources/objects/planet/planet.obj");
	}
	if (!teapot) {
		cout << "loading teapot..." << endl;
		teapot = new Model("../resources/objects/teapot/teapot.obj");
	}
	if (!my_terrain) {
		cout << "loading terrain..." << endl;
		my_terrain = new Model("../resources/objects/terrain/terrain.obj");
	}
	if (!my_cyborg) {
		cout << "loading cyborg..." << endl;
		my_cyborg = new Model("../resources/objects/cyborg/cyborg.obj");
	}
	if (initTree) {
		initTree = false;
		loadTrees();
	}
	if (!my_leftWing) {
		cout << "loading wings..." << endl;
		my_leftWing = new Model("../resources/objects/wings/leftWing.obj");
	}
	if (!my_rightWing) {
		my_rightWing = new Model("../resources/objects/wings/rightWing.obj");
	}
	if (initLightObject) {
		cout << "loading light objects..." << endl;
		initLightObject = false;
		Light_object temp_obj("../resources/objects/sun/sun.obj");
		for (int i = 0; i < NR_LIGHT; ++i) {
			if (light_sources[i].type == POINT_LIGHT) {
				my_light_objects.push_back(temp_obj);
				my_light_objects.back().pos = light_sources[i].position;
				my_light_objects.back().scaleVal = powf(light_sources[i].diffuse.x, 3) * glm::vec3(15.0);
				my_light_objects.back().update_modelMatrix();
			}
		}
		cout << "All models are loaded." << endl;
	}
}

void TrainView::loadTrees() {
	fstream tree_file("../TrackFiles/tree_pos.txt");
	if (!tree_file) cout << "tree file error" << endl;
	cout << "loading tree1..." << endl;
	Base_Object temp_tree1("../resources/objects/tree1/JASMIM+MANGA.obj");
	cout << "loading tree2..." << endl;
	Base_Object temp_tree2("../resources/objects/tree2/tree2.obj");

	string line;
	while (getline(tree_file, line)) {
		if (line[0] == '#') continue;
		stringstream ss(line);

		int tree_type;
		ss >> tree_type;
		if (tree_type == 1) {
			my_trees.push_back(temp_tree1);
		}
		else if (tree_type == 2) {
			my_trees.push_back(temp_tree2);
		}

		ss >> my_trees.back().pos.x >> my_trees.back().pos.y >> my_trees.back().pos.z;
		float scale;
		ss >> scale;
		my_trees.back().scaleVal = glm::vec3(scale);
		ss >> my_trees.back().rotation_angle >> my_trees.back().rotation_axis.x >> my_trees.back().rotation_axis.y >> my_trees.back().rotation_axis.z;
		
		my_trees.back().update_modelMatrix();
	}

	my_trees.push_back(temp_tree1);
	my_trees.back().pos = vec3(0, 0, 0);
	my_trees.back().scaleVal = glm::vec3(10); 
	my_trees.back().update_modelMatrix();
	cout << my_trees.size() << endl;
}

void TrainView::loadTextures() {
	if (!ground_texture)
		ground_texture = new Texture2D("../Images/black_white_board.png");
	if (!water_texture)
		water_texture = new Texture2D("../Images/blue.png");

	if (initBrickTexture) {
		initBrickTexture = false;
		brick_diffuseMap = loadTexture("../resources/textures/brickwall.jpg");
		brick_normalMap = loadTexture("../resources/textures/brickwall_normal.jpg");

		brick2_diffuseMap = loadTexture("../resources/textures/bricks2.jpg");
		brick2_normalMap = loadTexture("../resources/textures/bricks2_normal.jpg");
		brick2_heightMap = loadTexture("../resources/textures/bricks2_disp.jpg");
	}
}

void TrainView::loadWaterMesh() {
	if (!waterMesh) {
		cout << "loading water mesh..." << endl;
		waterMesh = new WaterMesh(0);
	}
}

void TrainView::loadflagMesh() {
	if (!flagMesh) {
		cout << "loading flag mesh..." << endl;
		flagMesh = new WaterMesh(1);
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
	// set the rendering destination to FBO
	mainFBO->bind();
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
	// clear buffer
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawTeapot();
	drawTrees();
	drawTitans();
	drawCannons();
	draw_track();
	draw_sleeper();
	draw_train();
	draw_cars();
	updateLightSource();
	drawLightObjects();
	drawBrickWall();
	drawBrickWall2();
	draw_terrain();
	drawWater(tw->waveTypeBrowser->value());
	drawflag();
	drawSkyBox();
	drawText();

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
	// clear all relevant buffers
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
	glClear(GL_COLOR_BUFFER_BIT);

	mainScreen_shader->use();
	mainScreen_shader->setFloat("vx_offset", 1.0);
	mainScreen_shader->setFloat("rt_w", w());
	mainScreen_shader->setFloat("rt_h", h());
	mainScreen_shader->setFloat("pixel_w", 10.0);
	mainScreen_shader->setFloat("pixel_h", 10.0);

	mainScreen_shader->setBool("doPixelation", 0);
	mainScreen_shader->setBool("doOffset", 0);
	mainScreen_shader->setBool("doGrayscale", 0);
	mainScreen_shader->setBool("doCartoon", 0);
	mainScreen_shader->setBool("edgeFilter", 0);

	if (tw->NPRBrowser->value() == 1) {
		mainScreen_shader->setBool("noEffect", 1);
	}
	else if (tw->NPRBrowser->value() == 2) {
		mainScreen_shader->setBool("doPixelation", 1);
	}
	else if (tw->NPRBrowser->value() == 3) {
		mainScreen_shader->setBool("doOffset", 1);
	}
	else if (tw->NPRBrowser->value() == 4) {
		mainScreen_shader->setBool("doGrayscale", 1);
	}
	else if (tw->NPRBrowser->value() == 5) {
		mainScreen_shader->setBool("doCartoon", 1);
	}
	else if (tw->NPRBrowser->value() == 6) {
		float offset = 1.0f / 300.0f;
		float offsets[9][2] = {
			{ -offset,  offset  },  // top-left
			{  0.0f,    offset  },  // top-center
			{  offset,  offset  },  // top-right
			{ -offset,  0.0f    },  // center-left
			{  0.0f,    0.0f    },  // center-center
			{  offset,  0.0f    },  // center - right
			{ -offset, -offset  },  // bottom-left
			{  0.0f,   -offset  },  // bottom-center
			{  offset, -offset  }   // bottom-right    
		};
		int edge_kernel[9] = {
		-1, -1, -1,
		-1,  8, -1,
		-1, -1, -1
		};
		glUniform2fv(glGetUniformLocation(mainScreen_shader->ID, "offsets"), 9, (float*)offsets);
		glUniform1iv(glGetUniformLocation(mainScreen_shader->ID, "edge_kernel"), 9, edge_kernel);
		mainScreen_shader->setBool("edgeFilter", 1);
	}

	if (shakeEffect) {
		if (shakeTime > 0) {
			shakeTime -= delta_t;
		}
		else {
			shakeEffect = false;
		}
		mainScreen_shader->setBool("shake", 1);
		mainScreen_shader->setFloat("time", shakeTime);
	}
	

	glBindVertexArray(mainScreenVAO->vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mainFBO->getColorId());
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glUseProgram(0);
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
	cout << "update track data\n";
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

//initialize openAL stuff
void TrainView::initSound() {
	if (initOpenAL) {
		initOpenAL = false;
		this->device = alcOpenDevice(NULL);
		if (!this->device) {
			//puts("ERROR::NO_AUDIO_DEVICE");
		}
		ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
		this->context = alcCreateContext(this->device, NULL);
		if (!alcMakeContextCurrent(context))
			puts("Failed to make context current");
		this->source_pos = glm::vec3(0.0f, 0.0f, 0.0f);
		ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
		alListener3f(AL_POSITION, source_pos.x, source_pos.y, source_pos.z);
		alListener3f(AL_VELOCITY, 0, 0, 0);
		alListenerfv(AL_ORIENTATION, listenerOri);

		alGenSources((ALuint)1, &this->BGMSoundSource);
		alSourcef(this->BGMSoundSource, AL_PITCH, 1);
		alSourcef(this->BGMSoundSource, AL_GAIN, 1.0f);
		alSource3f(this->BGMSoundSource, AL_POSITION, source_pos.x, source_pos.y, source_pos.z);
		alSource3f(this->BGMSoundSource, AL_VELOCITY, 0, 0, 0);
		alSourcei(this->BGMSoundSource, AL_LOOPING, AL_TRUE);
		alGenBuffers((ALuint)1, &this->BGMSoundBuffer);
		ALsizei size, freq;
		ALenum format;
		ALvoid* data;
		ALboolean loop = AL_TRUE;
		//Material from: ThinMatrix
		cout << "loading bgm..." << endl;
		alutLoadWAVFile((ALbyte*)"../resources/audio/BGM2.wav", &format, &data, &size, &freq, &loop);
		alBufferData(this->BGMSoundBuffer, format, data, size, freq);
		alSourcei(this->BGMSoundSource, AL_BUFFER, this->BGMSoundBuffer);
		//if (format == AL_FORMAT_STEREO16 || format == AL_FORMAT_STEREO8)
		//	puts("TYPE::STEREO");
		//else if (format == AL_FORMAT_MONO16 || format == AL_FORMAT_MONO8)
		//	puts("TYPE::MONO");
		alSourcef(BGMSoundSource, AL_GAIN, 1.0);


		alGenSources((ALuint)1, &this->cannonSoundSource);
		alSourcef(this->cannonSoundSource, AL_PITCH, 1);
		alSourcef(this->cannonSoundSource, AL_GAIN, 1.0f);
		alSource3f(this->cannonSoundSource, AL_POSITION, source_pos.x, source_pos.y, source_pos.z);
		alSource3f(this->cannonSoundSource, AL_VELOCITY, 0, 0, 0);
		alSourcei(this->cannonSoundSource, AL_LOOPING, AL_FALSE);
		alGenBuffers((ALuint)1, &this->cannonSoundBuffer);

		loop = AL_FALSE;
		//Material from: ThinMatrix
		cout << "loading other sound effect..." << endl;
		alutLoadWAVFile((ALbyte*)"../resources/audio/cannon.wav", &format, &data, &size, &freq, &loop);
		alBufferData(this->cannonSoundBuffer, format, data, size, freq);
		alSourcei(this->cannonSoundSource, AL_BUFFER, this->cannonSoundBuffer);
		//if (format == AL_FORMAT_STEREO16 || format == AL_FORMAT_STEREO8)
		//	puts("TYPE::STEREO");
		//else if (format == AL_FORMAT_MONO16 || format == AL_FORMAT_MONO8)
		//	puts("TYPE::MONO");
		alSourcef(cannonSoundSource, AL_GAIN, 0.25);
	}
}

//initialize UBO
void TrainView::initUBO() {
	if (!this->commom_matrices) {
		this->commom_matrices = new UBO();
		this->commom_matrices->size = 2 * sizeof(glm::mat4);
		glGenBuffers(1, &this->commom_matrices->ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
		glBufferData(GL_UNIFORM_BUFFER, this->commom_matrices->size, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

//intialize plane vao
void TrainView::initPlane() {
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
}

void TrainView::updateListenerPos() {
	if (selectedCube >= 0)
		alListener3f(AL_POSITION,
			m_pTrack->points[selectedCube].pos.x,
			m_pTrack->points[selectedCube].pos.y,
			m_pTrack->points[selectedCube].pos.z);
	else
		alListener3f(AL_POSITION,
			this->source_pos.x,
			this->source_pos.y,
			this->source_pos.z);
}

void TrainView::deleteSelectedObject() {
	if (selectedTree >= 0 && !my_trees.empty()) {
		my_trees.erase(my_trees.begin() + selectedTree);
	}
	//... other stuff
}

void TrainView::addTree() {
	Base_Object temp_tree("../resources/objects/tree2/tree2.obj");
	my_trees.push_back(temp_tree);
	my_trees.back().pos = glm::vec3(0, 200, 0);
	my_trees.back().scaleVal = glm::vec3(tw->treeSize->value(), tw->treeSize->value(), tw->treeSize->value());
	my_trees.back().update_modelMatrix();
}

void TrainView::initLightSource() {
	light_sources[0].type = DIRECTIONAL_LIGHT;
	light_sources[0].direction = glm::vec3(0, -1, 0);
	light_sources[0].ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	light_sources[0].diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
	light_sources[0].specular = glm::vec3(0.5f, 0.5f, 0.5f);

	light_sources[1].type = DIRECTIONAL_LIGHT;
	light_sources[1].direction = glm::vec3(0.1, -1, 0);
	light_sources[1].ambient = glm::vec3(0.0f, 0.0f, 0.0f);
	light_sources[1].diffuse = glm::vec3(0.0f, 0.0f, 0.0f);
	light_sources[1].specular = glm::vec3(0.0f, 0.0f, 0.0f);

	light_sources[2].type = DIRECTIONAL_LIGHT;
	light_sources[2].direction = glm::vec3(-0.1, -1, 0);
	light_sources[2].ambient = glm::vec3(0.0f, 0.0f, 0.0f);
	light_sources[2].diffuse = glm::vec3(0.0f, 0.0f, 0.0f);
	light_sources[2].specular = glm::vec3(0.0f, 0.0f, 0.0f);

	light_sources[3].type = DIRECTIONAL_LIGHT;
	light_sources[3].direction = glm::vec3(0, -1, -0.1);
	light_sources[2].ambient = glm::vec3(0.0f, 0.0f, 0.0f);
	light_sources[2].diffuse = glm::vec3(0.0f, 0.0f, 0.0f);
	light_sources[2].specular = glm::vec3(0.0f, 0.0f, 0.0f);

	light_sources[4].type = POINT_LIGHT;
	light_sources[4].position = glm::vec3(0.0f, 5000.0, 0.0f);
	light_sources[4].ambient = glm::vec3(0.05f, 0.05f, 0.05f);
	light_sources[4].diffuse = glm::vec3(5.0f, 4.0f, 4.0f);
	light_sources[4].specular = glm::vec3(1.0f, 1.0f, 1.0f);
	light_sources[4].constant = 0.05f;
	light_sources[4].linear = 0.001;
	light_sources[4].quadratic = 0.0;

	light_sources[5].type = POINT_LIGHT;
	light_sources[5].position = glm::vec3(200.0f, 10.0f, 0.0f);
	light_sources[5].ambient = glm::vec3(0.05f, 0.05f, 0.05f);
	light_sources[5].diffuse = glm::vec3(0.7f, 0.7f, 0.6);
	light_sources[5].specular = glm::vec3(0.5f, 0.5f, 0.5f);
	light_sources[5].constant = 0.1f;
	light_sources[5].linear = 0.01;
	light_sources[5].quadratic = 0.0;

	light_sources[6].type = POINT_LIGHT;
	light_sources[6].position = glm::vec3(-200.0f, 0.0f, 0.0f);
	light_sources[6].ambient = glm::vec3(0.05f, 0.05f, 0.05f);
	light_sources[6].diffuse = glm::vec3(0.6f, 0.6f, 0.55f);
	light_sources[6].specular = glm::vec3(0.5f, 0.5f, 0.5f);
	light_sources[6].constant = 0.1f;
	light_sources[6].linear = 0.01;
	light_sources[6].quadratic = 0.0;

	light_sources[7].type = POINT_LIGHT;
	light_sources[7].position = glm::vec3(0.0f, 30.0f, 200.0f);
	light_sources[7].ambient = glm::vec3(0.05f, 0.05f, 0.05f);
	light_sources[7].diffuse = glm::vec3(0.9f, 0.9f, 0.9f);
	light_sources[7].specular = glm::vec3(0.5f, 0.5f, 0.5f);
	light_sources[7].constant = 0.1f;
	light_sources[7].linear = 0.01;
	light_sources[7].quadratic = 0.0;

	light_sources[8].type = SPOT_LIGHT;
	light_sources[8].position = glm::vec3(0.0f, 100.0f, -100.0f);
	light_sources[8].direction = glm::vec3(0, -1, 0);
	light_sources[8].ambient = glm::vec3(0.05f, 0.05f, 0.05f);
	light_sources[8].diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
	light_sources[8].specular = glm::vec3(0.5f, 0.5f, 0.5f);
	light_sources[8].constant = 1.0f;
	light_sources[8].linear = 0.09;
	light_sources[8].quadratic = 0.032;
	light_sources[8].cutOff = glm::cos(glm::radians(12.5f));
	light_sources[8].outerCutOff = glm::cos(glm::radians(15.0f));
}

void TrainView::updateLightSource() {
	float radius = 20000.0;
	float speed = 0.01;
	float x = radius * cos((now_t / 1000.0) * speed);
	float y = radius * sin((now_t / 1000.0) * speed);



	light_sources[4].position = glm::vec3(x, y, 0.0f);
	my_light_objects[0].pos = light_sources[4].position;
	my_light_objects[0].update_modelMatrix();
	if (y >= 0) {
		/*light_sources[4].constant = 0.05f;
		light_sources[4].linear = 0.001;
		light_sources[4].quadratic = 0.0;*/
		light_sources[4].constant = 1.0f;
		light_sources[4].linear = 1.0;
		light_sources[4].quadratic = 1.0;
	}
	else {
		light_sources[4].constant = 1.0f;
		light_sources[4].linear = 1.0;
		light_sources[4].quadratic = 1.0;
	}

	light_sources[4].direction = -light_sources[4].position;

}

void TrainView::initParticleSystem() {
	if (!particle_system) {
		particle_system = new Particle_system;
	}
}

void TrainView::initRun() {
	if (firstRun) {
		firstRun = false;
		tw->runButton->set();
		
		now_t = now_t = glutGet(GLUT_ELAPSED_TIME);
		old_t = now_t;
	}
}

void TrainView::renderQuad() {
	if (quadVAO == 0)
	{
		// positions
		glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
		glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
		glm::vec3 pos3(1.0f, -1.0f, 0.0f);
		glm::vec3 pos4(1.0f, 1.0f, 0.0f);
		// texture coordinates
		glm::vec2 uv1(0.0f, 1.0f);
		glm::vec2 uv2(0.0f, 0.0f);
		glm::vec2 uv3(1.0f, 0.0f);
		glm::vec2 uv4(1.0f, 1.0f);
		// normal vector
		glm::vec3 nm(0.0f, 0.0f, 1.0f);

		// calculate tangent/bitangent vectors of both triangles
		glm::vec3 tangent1, bitangent1;
		glm::vec3 tangent2, bitangent2;
		// triangle 1
		// ----------
		glm::vec3 edge1 = pos2 - pos1;
		glm::vec3 edge2 = pos3 - pos1;
		glm::vec2 deltaUV1 = uv2 - uv1;
		glm::vec2 deltaUV2 = uv3 - uv1;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

		bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

		// triangle 2
		// ----------
		edge1 = pos3 - pos1;
		edge2 = pos4 - pos1;
		deltaUV1 = uv3 - uv1;
		deltaUV2 = uv4 - uv1;

		f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


		bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


		float quadVertices[] = {
			// positions            // normal         // texcoords  // tangent                          // bitangent
			pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
			pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
			pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

			pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
			pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
			pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
		};
		// configure plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void TrainView::initTrackData() {
	if (initTrack) {
		initTrack = false;
		tw->m_Track.readPoints("../TrackFiles/map.txt");
		update_arcLengh();
		
	}
}

void TrainView::initTitans() {
	if (initTitan) {
		initTitan = false;
		cout << "loading titan1..." << endl;
		all_titans.push_back(Titan("../resources/objects/titan/chou/chou.obj"));
		cout << "loading titan2..." << endl;
		all_titans.push_back(Titan("../resources/objects/titan/aga/textured_output.obj"));
		cout << "loading titan3..." << endl;
		all_titans.push_back(Titan("../resources/objects/titan/tsai/tsai.obj"));


		//all_titans.push_back(Titan("../resources/objects/titan/fake/fake.obj"));
		//all_titans.push_back(Titan("../resources/objects/titan/fake/fake.obj"));
		//all_titans.push_back(Titan("../resources/objects/titan/fake/fake.obj"));

		my_titans.clear();
		for (int i = 0; i < INIT_NR_TITAN; ++i) {
			int randnum = rand() % 3;
			//randnum = 0; //debug
			if (randnum == 0) {
				my_titans.push_back(all_titans[0]);
				float distance = 7000 + rand() % 20000;
				float angle = glm::radians(float(rand() % 360));
				float x = distance * cos(angle);
				float z = distance * sin(angle);

				float speed = 1.0 + (rand() % 100) / 500.0;

				float scale = 1000 + rand() % 1000;
				my_titans.back().distance = distance;
				my_titans.back().pos = glm::vec3(x, scale / 2.0f, z);
				my_titans.back().centralPoint = my_titans.back().pos;
				my_titans.back().scaleVal = glm::vec3(scale, scale, scale);
				my_titans.back().radius = 0.6 * scale;
				my_titans.back().speed = speed;
				my_titans.back().angle = angle;
				my_titans.back().delta_t = 0;
				my_titans.back().rotation_angle = rand() % 360;
				my_titans.back().rotateSpeed = 0.1 + (rand()%100) / 1000;
				my_titans.back().rotation_axis = glm::vec3(0, 1, 0);
				my_titans.back().update_modelMatrix();
			}
			else if (randnum == 1) {
				my_titans.push_back(all_titans[1]);
				float distance = 7000 + rand() % 20000;
				float angle = glm::radians(float(rand() % 360));
				float x = distance * cos(angle);
				float z = distance * sin(angle);

				float speed = 1.5 + (rand() % 100) / 100.0;

				float scale = 1000 + rand() % 1000;
				my_titans.back().distance = distance;
				my_titans.back().pos = glm::vec3(x, scale / 2.0f, z);
				my_titans.back().centralPoint = my_titans.back().pos;
				my_titans.back().scaleVal = glm::vec3(scale, scale, scale);
				my_titans.back().radius = 0.6 * scale;
				my_titans.back().speed = speed;
				my_titans.back().angle = angle;
				my_titans.back().delta_t = 0;
				my_titans.back().rotation_angle = rand() % 360;
				my_titans.back().rotateSpeed = 0.3 + (rand() % 100) / 700;
				my_titans.back().rotation_axis = glm::vec3(0, 1, 0);
				my_titans.back().update_modelMatrix();
			}
			else if (randnum == 2) {
				my_titans.push_back(all_titans[2]);
				float distance = 6000 + rand() % 5000;
				float angle = glm::radians(float(rand() % 360));
				float x = distance * cos(angle);
				float z = distance * sin(angle);

				float speed = 1.0 + (rand() % 100) / 300.0;

				float scale = 800 + rand() % 800;
				my_titans.back().distance = distance;
				my_titans.back().pos = glm::vec3(x, scale / 1.5f, z);
				my_titans.back().centralPoint = my_titans.back().pos;
				my_titans.back().scaleVal = glm::vec3(scale, scale, scale);
				my_titans.back().radius = 0.6 * scale;
				my_titans.back().speed = speed;
				my_titans.back().angle = angle;
				my_titans.back().delta_t = 0;
				my_titans.back().rotation_angle = rand() % 360;
				my_titans.back().rotateSpeed = 0.05 + (rand() % 100) / 1000;
				my_titans.back().rotation_axis = glm::vec3(0, 1, 0);
				my_titans.back().update_modelMatrix();
			}
		}
	}
}

void TrainView::reloadTitans() {
	my_titans.clear();
	for (int i = 0; i < INIT_NR_TITAN; ++i) {
		int randnum = rand() % 3;
		//randnum = 0; //debug
		if (randnum == 0) {
			my_titans.push_back(all_titans[0]);
			float distance = 7000 + rand() % 20000;
			float angle = glm::radians(float(rand() % 360));
			float x = distance * cos(angle);
			float z = distance * sin(angle);

			float speed = 1.0 + (rand() % 100) / 500.0;

			float scale = 1000 + rand() % 1000;
			my_titans.back().distance = distance;
			my_titans.back().pos = glm::vec3(x, scale / 2.0f, z);
			my_titans.back().centralPoint = my_titans.back().pos;
			my_titans.back().scaleVal = glm::vec3(scale, scale, scale);
			my_titans.back().radius = 0.6 * scale;
			my_titans.back().speed = speed;
			my_titans.back().angle = angle;
			my_titans.back().delta_t = 0;
			my_titans.back().rotation_angle = rand() % 360;
			my_titans.back().rotateSpeed = 0.1 + (rand() % 100) / 1000;
			my_titans.back().rotation_axis = glm::vec3(0, 1, 0);
			my_titans.back().update_modelMatrix();
		}
		else if (randnum == 1) {
			my_titans.push_back(all_titans[1]);
			float distance = 7000 + rand() % 20000;
			float angle = glm::radians(float(rand() % 360));
			float x = distance * cos(angle);
			float z = distance * sin(angle);

			float speed = 1.5 + (rand() % 100) / 100.0;

			float scale = 1000 + rand() % 1000;
			my_titans.back().distance = distance;
			my_titans.back().pos = glm::vec3(x, scale / 2.0f, z);
			my_titans.back().centralPoint = my_titans.back().pos;
			my_titans.back().scaleVal = glm::vec3(scale, scale, scale);
			my_titans.back().radius = 0.6 * scale;
			my_titans.back().speed = speed;
			my_titans.back().angle = angle;
			my_titans.back().delta_t = 0;
			my_titans.back().rotation_angle = rand() % 360;
			my_titans.back().rotateSpeed = 0.3 + (rand() % 100) / 700;
			my_titans.back().rotation_axis = glm::vec3(0, 1, 0);
			my_titans.back().update_modelMatrix();
		}
		else if (randnum == 2) {
			my_titans.push_back(all_titans[2]);
			float distance = 6000 + rand() % 5000;
			float angle = glm::radians(float(rand() % 360));
			float x = distance * cos(angle);
			float z = distance * sin(angle);

			float speed = 1.0 + (rand() % 100) / 300.0;

			float scale = 800 + rand() % 800;
			my_titans.back().distance = distance;
			my_titans.back().pos = glm::vec3(x, scale / 1.5f, z);
			my_titans.back().centralPoint = my_titans.back().pos;
			my_titans.back().scaleVal = glm::vec3(scale, scale, scale);
			my_titans.back().radius = 0.6 * scale;
			my_titans.back().speed = speed;
			my_titans.back().angle = angle;
			my_titans.back().delta_t = 0;
			my_titans.back().rotation_angle = rand() % 360;
			my_titans.back().rotateSpeed = 0.05 + (rand() % 100) / 1000;
			my_titans.back().rotation_axis = glm::vec3(0, 1, 0);
			my_titans.back().update_modelMatrix();
		}
	}
}

void TrainView::initCannons() {
	if (initCannon) {
		initCannon = false;
		all_cannons.push_back(Cannon("../resources/objects/cannon/cannon.obj"));
	}
}

void TrainView::launchCannon() {
	if (!(my_cannons.size() <= MAX_NR_CANNON)) {
		return;
	}
	alSourcePlay(this->cannonSoundSource);
	my_cannons.push_back(all_cannons[0]);
	size_t i;
	if (tw->arcLength->value() == 0) {
		i = int(m_pTrack->trainU / 1) * DIVIDE_LINE;
		i = i + (m_pTrack->trainU - int(m_pTrack->trainU / 1)) * DIVIDE_LINE - 0.1;
		if (i < 0) {
			i = (float)m_pTrack->points.size() - i;
		}
	}
	else {
		i = C_length_index();
	}
	glm::vec3 qt0_v = all_qt[i];
	my_cannons.back().pos = qt0_v;
	float scale = 25.0f;
	my_cannons.back().scaleVal = glm::vec3(scale, scale, scale);
	glm::vec3 speed = 5000.0f * camera.Front;
	if (playing && game_time < 30) {
		speed *= 3;
	}
	my_cannons.back().speed = speed;
	my_cannons.back().gravity = 500.0f;
	my_cannons.back().radius = scale;
	my_cannons.back().update_modelMatrix();

	shakeEffect = true;
	shakeTime = 0.5;
}

void TrainView::initTextRender() {
	if (!textRender) {
		textRender = new TextRender();
	}
}

void TrainView::startGame() {
	game_time = 180;
	playing = true;
	win = false;
	lose = false;
	reloadTitans();
	tw->worldCam->clear();
	tw->trainCam->set();
	tw->speed->value(30);
	alSourcePlay(this->BGMSoundSource);
}

void TrainView::checkScoreAndTime() {
	if (!playing)return;
	game_time -= delta_t;
	if (game_time <= 0) {
		game_time = 0;
		playing = false;
		if (my_titans.size() > 0) {
			lose = true;
		}
		else {
			lose = true;
		}
	}
}