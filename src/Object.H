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