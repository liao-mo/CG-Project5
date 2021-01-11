#pragma once

#include <iostream>
#include <glad/glad.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>

#include <vector>
#include <string>

#include "Utilities/3DUtils.H"
#include "Utilities/Pnt3f.H"
#include "Utilities/Matrices.h"
#include "Utilities/objloader.hpp"
#include "Utilities/ArcBallCam.H"
#include "RenderUtilities/Texture.h"

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

using namespace std;

class Base_Object {
public:
	Base_Object(string file_name) {
		model = new Model(file_name);
		pos = glm::vec3(0, 0, 0);
		scaleVal = glm::vec3(1, 1, 1);
		rotation_axis = glm::vec3(1, 0, 0);
		rotation_angle = 0.0f;
		update_modelMatrix();
	}

	Model* model = nullptr;
	glm::vec3 pos;
	glm::vec3 scaleVal;
	glm::vec3 rotation_axis;
	float rotation_angle;
	glm::mat4 model_matrix;

	void update_modelMatrix() {
		model_matrix = glm::mat4(1.0);
		model_matrix = glm::translate(model_matrix, pos);
		if(rotation_angle > 0.1) model_matrix = glm::rotate(model_matrix, glm::radians(rotation_angle), rotation_axis);
		model_matrix = glm::scale(model_matrix, glm::vec3(scaleVal));
	}
};

class Light_object : public Base_Object{
public:
	Light_object(string file_name) : Base_Object(file_name){

	}
};

class Titan :public Base_Object {
public:
	Titan(string file_name) : Base_Object(file_name) {

	}
	float radius;
	float distance;
	glm::vec3 centralPoint;
	float speed = 1.0f;
	float delta_t = 0;
	float angle = 0;
	void move() {
		angle += delta_t * speed;
		float x = distance * cos(angle);
		float z = distance * sin(angle);
		pos.x = x;
		pos.z = z;

		update_modelMatrix();
	}
};

class Cannon :public Base_Object {
public:
	Cannon(string file_name) : Base_Object(file_name) {

	}
	float gravity;
	glm::vec3 speed;
};

struct Light_Source {
	int type = DIRECTIONAL_LIGHT;

	glm::vec3 direction;
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float constant;
	float linear;
	float quadratic;
	float cutOff;
	float outerCutOff;
};