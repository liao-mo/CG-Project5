#pragma once
#include <iostream>
#include <vector>
#include <string>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>



#define MaxParticles 9999

using namespace std;

// CPU representation of a particle
struct Particle {
	glm::vec3 pos, speed;
	unsigned char r, g, b, a; // Color
	float size, angle, weight;
	float life; // Remaining life of the particle. if <0 : dead and unused.
	float cameradistance; // *Squared* distance to the camera. if dead : -1.0f

	bool operator<(const Particle& that) const {
		// Sort in reverse order : far particles drawn first.
		return this->cameradistance > that.cameradistance;
	}
};

class Particle_system {
public:
	Particle_system();

	Particle ParticlesContainer[MaxParticles];
	int LastUsedParticle = 0;

	// Finds a Particle in ParticlesContainer which isn't used yet.
	// (i.e. life < 0);
	int FindUnusedParticle();

	void SortParticles();

	GLuint VertexArrayID;
	Shader* shader = nullptr;
	GLuint programID;
	GLuint CameraRight_worldspace_ID;
	GLuint CameraUp_worldspace_ID;
	GLuint ViewProjMatrixID;
	GLuint TextureID;

	GLfloat* g_particule_position_size_data;
	GLubyte* g_particule_color_data;

	GLuint Texture;

	glm::mat4 ProjectionMatrix;
	glm::mat4 ViewMatrix;

	// The VBO containing the 4 vertices of the particles.
	// Thanks to instancing, they will be shared by all particles.
	const GLfloat g_vertex_buffer_data[12] = {
		 -0.5f, -0.5f, 0.0f,
		  0.5f, -0.5f, 0.0f,
		 -0.5f,  0.5f, 0.0f,
		  0.5f,  0.5f, 0.0f,
	};
	GLuint billboard_vertex_buffer;

	// The VBO containing the positions and sizes of the particles
	GLuint particles_position_buffer;

	// The VBO containing the colors of the particles
	GLuint particles_color_buffer;

	double lastTime = 0;

	double delta = 0;

	void setMatrix(glm::mat4 projectionM, glm::mat4 viewM);

	void update();

	glm::vec3 particle_position;
	double particle_life = 5;
	double particle_size = 1;
	double particle_gravity = -9.8;
	float particle_spread = 1.5;
	glm::vec3 particle_maindir = glm::vec3(0, 10, 0);
};