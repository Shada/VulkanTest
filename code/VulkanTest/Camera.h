#pragma once
#include <glm\glm.hpp>
#include <vector>
#include <iostream>
#include <array>


class Camera
{
  
public:
   struct MatrixBufferObject
   {
      glm::mat4 viewMatrix;
      glm::mat4 projectionMatrix;
   } matrixBufferObject;

   Camera();
   Camera(glm::vec3 position, glm::vec3 viewDirection, glm::vec3 upDirection);
   ~Camera();

   MatrixBufferObject getCameraData();

   // TODO:: make something more sensible out of this. 
   // have to think about howthe camera is supposed to move..
   // how to change speed, etc. etc.
   void move(glm::vec3 direction, float distance);
   void rotate(glm::vec3 yawPichRollAngles);
   void rotate();
   void setWindowSize(int width, int height);
private:

   glm::vec3 position;
   glm::vec3 viewDirection;
   glm::vec3 upDirection;
   glm::vec2 windowSize;
   // TODO: should use quaternions 
};

