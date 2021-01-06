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

using namespace std;

class Tree {
public:
	Tree(string file_name) {
		model = new Model(FileSystem::getPath("resources/objects/tree1/JASMIM+MANGA.obj"));
		pos = glm::vec3(0, 0, 0);
		scaleVal = glm::vec3(1, 1, 1);
		update_modelMatrix();
	}

	Model* model = nullptr;
	glm::vec3 pos;
	glm::vec3 scaleVal;
	glm::mat4 model_matrix;

	void update_modelMatrix() {
		model_matrix = glm::mat4(1.0);
		model_matrix = glm::translate(model_matrix, pos);
		model_matrix = glm::scale(model_matrix, glm::vec3(scaleVal));
	}
};