#include "Camera.h"



Camera::Camera()
{
   this->position = glm::vec3(0.0f, 0.0f, 0.0f);
   this->viewDirection = glm::vec3(0.0f, 0.0f, 1.0f);
   this->upDirection = glm::vec3(0.0f, 1.0f, 0.0f);
}

Camera::Camera(glm::vec3 position, glm::vec3 viewDirection, glm::vec3 upDirection)
{
   this->viewDirection = viewDirection;
   this->position      = position;
   this->upDirection   = upDirection;
}

#include <glm\gtc\matrix_transform.hpp>
#include <chrono>
#include <algorithm>


static auto startTime = std::chrono::high_resolution_clock::now();

void Camera::rotate()
{
   auto currentTime = std::chrono::high_resolution_clock::now();
   float time       = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count() / 1e6f;

   matrixBufferObject.viewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

   matrixBufferObject.projectionMatrix = glm::perspective(glm::radians(45.0f), windowSize.x / (float)windowSize.y, 0.1f, 10.0f);
   matrixBufferObject.projectionMatrix[1][1] *= -1;
}

void Camera::setWindowSize(int width, int height)
{
   windowSize.x = width;
   windowSize.y = height;
}

Camera::~Camera()
{}

Camera::MatrixBufferObject Camera::getCameraData()
{
   return matrixBufferObject;
}
