#include "Camera.h"
#include "stdafx.h"
#include <glm\gtc\matrix_transform.hpp>
#include <chrono>
#include <algorithm>


Camera::Camera()
{
   this->position = glm::vec3(0.0f, 0.0f, -2.0f);
   this->viewDirection = glm::vec3(0.0f, 0.0f, 1.0f);
   this->upDirection = glm::vec3(0.0f, 1.0f, 0.0f);
}

Camera::Camera(glm::vec3 position, glm::vec3 viewDirection, glm::vec3 upDirection)
{
   this->viewDirection = viewDirection;
   this->position      = position;
   this->upDirection   = upDirection;
}

void Camera::updateMatrices()
{
   //TODO: check if anything has changed.


   matrixBufferObject.viewMatrix = glm::lookAt(position, position + viewDirection, upDirection);

   matrixBufferObject.projectionMatrix = glm::perspective(glm::radians(45.0f), windowSize.x / (float)windowSize.y, 0.1f, 10.0f);
   matrixBufferObject.projectionMatrix[1][1] *= -1;
}

void Camera::setWindowSize(int width, int height)
{
   windowSize.x = (float)width;
   windowSize.y = (float)height;
}

Camera::~Camera()
{}

Camera::MatrixBufferObject Camera::getCameraData()
{
   return matrixBufferObject;
}
