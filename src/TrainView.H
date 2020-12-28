#pragma once

#include "RenderUtilities/BufferObject.h"
//#include "RenderUtilities/Shader.h"
#include "RenderUtilities/Texture.h"

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include "WaterMesh.h"
#include "SkyBox.h"
#include "FrameBuffer.h"


#define SCR_WIDTH 800
#define SCR_HEIGHT 600
#define NEAR 0.01
#define FAR 5000.0
const int TEXTURE_WIDTH = 1920;  // NOTE: texture size cannot be larger than
const int TEXTURE_HEIGHT = 1080;  // the rendering window size in non-FBO mode

// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;

// The TrainView also needs to handle its user interface events.
// since this is an FLTk Window, we need to include FlTk Headers
// the pragmas turn off the warnings from FlTk - our code should have no warnings,
// but their code doesn't meet that standard
#pragma warning(push)
#pragma warning(disable:4312)
#pragma warning(disable:4311)
#include <Fl/Fl_Gl_Window.h>
#pragma warning(pop)

#include <AL/alut.h>
//#include <AL/al.h>
//#include <AL/alc.h>

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"

class TrainView : public Fl_Gl_Window
{
	public:
		// note that we keep the "standard widget" constructor arguments
		TrainView(int x, int y, int w, int h, const char* l = 0);

		// overrides of important window things
		virtual int handle(int);
		virtual void draw();

		// all of the actual drawing happens in this routine
		// it has to be encapsulated, since we draw differently if
		// we're drawing shadows (no colors, for example)
		void drawStuff(bool doingShadows=false);

		// setup the projection - assuming that the projection stack has been
		// cleared for you
		void setProjection();

		// Reset the Arc ball control
		void resetArcball();

		// pick a point (for when the mouse goes down)
		void doPick();

		//set ubo
		void setUBO();

		//draw object functions
		//draw ground
		void drawGround();

		//draw train
		void drawTrain();
		
	public:
		ArcBallCam		arcball;			// keep an ArcBall for the UI
		int				selectedCube;  // simple - just remember which cube is selected

		TrainWindow*	tw;				// The parent of this display window
		CTrack*			m_pTrack;		// The track of the entire scene

		//shader objects
		Shader* current_light_shader = nullptr;
		Shader* directional_light_shader = nullptr;
		Shader* point_light_shader = nullptr;
		Shader* spot_light_shader = nullptr;
		Shader* light_source_shader = nullptr;
		Shader* mainScreen_shader = nullptr;
		Shader* subScreen_shader = nullptr;
		Shader* interactiveHeightMap_shader = nullptr;
		void loadShaders();
		void update_light_shaders();

		//FBO
		FrameBuffer* mainFBO = nullptr;
		FrameBuffer* subScreenFBO = nullptr;
		FrameBuffer* colorUVFBO = nullptr;
		FrameBuffer* interactiveHeightMapFBO0 = nullptr;
		FrameBuffer* interactiveHeightMapFBO1 = nullptr;
		void initFBOs();
		void drawMainFBO();
		void drawSubScreenFBO();
		void drawColorUVFBO();
		void updateInteractiveHeightMapFBO(int mode, glm::vec2 u_center = glm::vec2(0, 0));

		//screens
		VAO* mainScreenVAO = nullptr;
		VAO* subScreenVAO = nullptr;
		void initVAOs();
		void drawMainScreen();
		void drawSubScreen();

		//textures
		Texture2D* ground_texture	= nullptr;
		Texture2D* water_texture = nullptr;

		//plane
		VAO* plane			= nullptr;
		UBO* commom_matrices= nullptr;
		void loadTextures();

		//models
		Model* sci_fi_train = nullptr;
		Model* teapot = nullptr;
		void loadModels();
		void drawTeapot();

		//water
		WaterMesh* waterMesh = nullptr;
		VAO* interactiveHeightMapVAO = nullptr;
		int currentFBO = 0;
		void loadWaterMesh();
		void drawWater(int mode);

		//skyBox
		SkyBox* skyBox = nullptr;
		void loadSkyBox();
		void drawSkyBox();

		// camera
		Camera camera;
		float lastX = SCR_WIDTH / 2.0f;
		float lastY = SCR_HEIGHT / 2.0f;
		bool firstMouse = true;
		void updata_camera();

		//OpenAL
		glm::vec3 source_pos;
		glm::vec3 listener_pos;
		ALCdevice* device	= nullptr;
		ALCcontext* context = nullptr;
		ALuint source;
		ALuint buffer;

		//timer
		int old_t;
		int now_t;
		float delta_t;
		void updateTimer();

		//events
		int k;
		int ks;
		bool k_pressed;
		bool firstDraw = true;

		//view projection matrices
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;
};