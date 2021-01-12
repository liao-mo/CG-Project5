#include "WaterMesh.h"
#include <string>

using namespace std;

WaterMesh::WaterMesh(int type) :
	waveCounter(0),
	previousTime(0),
	currentTime(0),
	amplitude_coefficient(1.0),
	type(type)
{
	if (type == 0) {
		grid = new Model("../resources/objects/grid/grid.obj");
	}
	else if (type == 1) {
		grid = new Model("../resources/objects/grid/low_grid.obj");
	}
	//
	//Debug: low polygons for fast loading
	//grid = new Model("../resources/objects/grid/low_grid.obj");
	sinWave_shader = new Shader("../src/shaders/water_surface.vert", "../src/shaders/water_surface.frag");
	heightMap_shader = new Shader("../src/shaders/water_heightMap.vert", "../src/shaders/water_heightMap.frag");
	color_uv_shader = new Shader("../src/shaders/color_uv.vert", "../src/shaders/color_uv.frag");
	flagTexture = new Texture2D("../resources/textures/freedomWings.jpg");
	initWaves();
	loadHeightMaps();
}

void WaterMesh::initWaves()
{
	for (int i = 0; i < MAX_WAVE; i++)
	{
		this->waves.waveLength[i] = 0;
		this->waves.amplitude[i] = 0;
		this->waves.speed[i] = 0;
		this->waves.direction[i] = glm::vec2(1, 0);
	}
	if (type == 0) {
		addSineWave(10, 0.3, 5, glm::vec2(1, 1));
		addSineWave(20, 0.4, 3, glm::vec2(1, -1));
		addSineWave(30, 0.5, 4, glm::vec2(2, 1));
	}
	else if (type == 1) {
		addSineWave(500, 10, 50, glm::vec2(1, 1));
		//addSineWave(250, 12, 15, glm::vec2(1, -1));
	}
}


void WaterMesh::addSineWave(float waveLength, float amplitude, float speed, glm::vec2 direction) {
	this->waves.waveLength[waveCounter] = waveLength;
	this->waves.amplitude[waveCounter] = amplitude;
	this->waves.speed[waveCounter] = speed;
	this->waves.direction[waveCounter] = direction;
	waveCounter++;
	if (waveCounter == MAX_WAVE)
	{
		waveCounter = 0;
		cerr << "too many wave" << endl;
	}
}

void WaterMesh::setEyePos(glm::vec3 eye_pos) {
	eyePos = eye_pos;
}

void WaterMesh::setMVP(glm::mat4 m, glm::mat4 v, glm::mat4 p) {
	modelMatrix = m;
	viewMatrix = v;
	projectionMatrix = p;
}

void WaterMesh::addTime(float delta_t) {
	currentTime += delta_t;
}

void WaterMesh::draw(int mode) {
	if (mode == 1) {
		drawSineWave();
	}
	else if (mode == 2) {
		drawHeightMap();
	}
}

void WaterMesh::drawSineWave() {
	sinWave_shader->use();
	sinWave_shader->setMat4("model", modelMatrix);
	sinWave_shader->setFloat("time", currentTime);
	sinWave_shader->setInt("numWaves", waveCounter);

	GLfloat amplitude[MAX_WAVE];
	GLfloat waveLength[MAX_WAVE];
	GLfloat speed[MAX_WAVE];
	for (int i = 0; i < MAX_WAVE; ++i) {
		amplitude[i] = waves.amplitude[i] * amplitude_coefficient;
		waveLength[i] = waves.waveLength[i] * waveLength_coefficient / 5.0;
		speed[i] = waves.speed[i] * speed_coefficient / 5.0;
	}

	glUniform1fv(glGetUniformLocation(sinWave_shader->ID, "amplitude"), MAX_WAVE, amplitude);
	glUniform1fv(glGetUniformLocation(sinWave_shader->ID, "wavelength"), MAX_WAVE, waveLength);
	glUniform1fv(glGetUniformLocation(sinWave_shader->ID, "speed"), MAX_WAVE, speed);
	for (int i = 0; i != MAX_WAVE; i++) {
		string name = "direction[";
		name += to_string(i);
		name += "]";
		GLint originsLoc = glGetUniformLocation(sinWave_shader->ID, name.c_str());
		glUniform2f(originsLoc, waves.direction[i].x, waves.direction[i].y);
	}

	sinWave_shader->setVec3("EyePos", eyePos);
	sinWave_shader->setVec3("light.direction", lightDir);
	sinWave_shader->setVec3("viewPos", eyePos);
	// light properties
	sinWave_shader->setVec3("light.ambient", 0.1f, 0.1f, 0.1f);
	sinWave_shader->setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
	sinWave_shader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);
	// material properties
	sinWave_shader->setFloat("material.shininess", 32.0f);

	sinWave_shader->setInt("type", type);
	sinWave_shader->setInt("material.diffuse", 2);
	flagTexture->bind(2);
	grid->Draw(*sinWave_shader);
	glUseProgram(0);
}

void WaterMesh::drawHeightMap() {
	heightMap_shader->use();
	heightMap_shader->setMat4("model", modelMatrix);
	heightMap_shader->setInt("heightMap", 1);
	heightMap_shader->setInt("interactive", 2);
	heightMap_shader->setFloat("amplitude", amplitude_coefficient);
	heightMap_shader->setBool("doInteractive", false);

	//check if we shoud change the heightMap after a period of time
	if (currentTime - previousTime >= 16) {
		if (heightMap_counter == HEIGHTMAP_NUM - 1) {
			heightMap_counter = 0;
		}
		else {
			++heightMap_counter;
		}
	};

	heightMap_textures[heightMap_counter]->bind(1);
	heightMap_shader->setVec3("EyePos", eyePos);
	heightMap_shader->setVec3("light.direction", -1.0f, -1.0f, -0.0f);
	heightMap_shader->setVec3("viewPos", eyePos);
	// light properties
	heightMap_shader->setVec3("light.ambient", 0.1f, 0.1f, 0.1f);
	heightMap_shader->setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
	heightMap_shader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);
	// material properties
	heightMap_shader->setFloat("material.shininess", 32.0f);

	grid->Draw(*heightMap_shader);
	heightMap_textures[heightMap_counter]->unbind(1);
	heightMap_textures[heightMap_counter]->unbind(2);
	glUseProgram(0);
}

void WaterMesh::loadHeightMaps() {
	heightMap_textures.resize(HEIGHTMAP_NUM);
	for (int i = 0; i < HEIGHTMAP_NUM; ++i) {
		string path = "../Images/heightMaps/";
		string number;
		if (i / 10 == 0) {
			number = "00" + to_string(i);
		}
		else if (i / 100 == 0) {
			number = "0" + to_string(i);
		}
		else {
			number = to_string(i);
		}
		path = path + number + ".png";
		heightMap_textures[i] = new Texture2D(path.c_str());
	}
}