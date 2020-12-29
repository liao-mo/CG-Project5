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

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"
#include "Utilities/Pnt3f.H"
#include <vector>
#include <chrono>


#define SCR_WIDTH 800
#define SCR_HEIGHT 600
#define NEAR 0.01
#define FAR 5000.0
#define DIVIDE_LINE 100

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

		//my function that draws the train
		void draw_train(bool doingShadows);

		//my function that draws the car
		void draw_car(bool doingShadows, float backward_offset);

		//my function that draws the track
		void draw_track(bool doingShadows);

		//my function that draws the sleeper
		void draw_sleeper(bool doingShadows);

		//my function that draws my ground
		void draw_ground();

		//my function that draws my env models
		void draw_env_model(bool doingShadows);

		// setup the projection - assuming that the projection stack has been
		// cleared for you
		void setProjection();

		// Reset the Arc ball control
		void resetArcball();

		// pick a point (for when the mouse goes down)
		void doPick();

		//set ubo
		void setUBO();

		//setupt train's head light
		void set_train_light();

		//update track data 
		void update_arcLengh();

		//use current length to find parameter t
		float length_to_t(float length);

		//use current length to find current index
		size_t length_to_index(float length);

		//find current index by C_length
		size_t C_length_index();

		//find current index by trainU
		size_t trainU_index();

		//match current length and trainU
		void match_length();

		//match current trainU to length 
		void match_trainU();

		//draw object functions
		//draw ground
		void drawGround();

		
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


		Model* my_train = nullptr;			//my train model
		Model* my_sleeper = nullptr;			//my train model
		Model* my_track = nullptr;			//my train model
		Model* my_car = nullptr;				//my car model
		Model* my_ground = nullptr;				//my car model
		std::vector<Model*> scene_models; //all other static models
		unsigned int num_of_car;

		//find 4 control point around current point
		std::vector<Pnt3f> find_Cpoints(int currentPoint);
		//find 4 orient vector around current point
		std::vector<Pnt3f> find_orient_vectors(int currentPoint);

		std::vector<float> t_param;
		std::vector<float> arc_length;
		std::vector<float> accumulate_length;
		std::vector<float> speeds;
		std::vector<glm::vec3> all_qt;
		std::vector<glm::vec3> all_orient;
		std::vector<glm::vec3> all_forward;
};