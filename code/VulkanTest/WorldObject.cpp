#include "WorldObject.h"


//TODO: add collision box and collision detections. Object vs object, and object vs ray to start


WorldObject::WorldObject(WorldObjectToMeshMapper *worldObjectToMeshMapper)
{
   this->worldObjectToMeshMapper = worldObjectToMeshMapper;
   numberOfObjects = 0;
}

void WorldObject::update(float dt)
{
   for(uint32_t i = 0; i < position.size(); i++)
   {
      if(movingSpeed[i] != 0.f)
      {
         position[i] += dt * movingSpeed[i] * movingDirection[i];
         invalidateModelMatrix(i);
      }
   }
   for(uint32_t i = 0; i < rotation.size(); i++)
   {
      if(rotationSpeed[i] != glm::vec3(0.f))
      {
         rotation[i] += dt * rotationSpeed[i];
         invalidateModelMatrix(i);
      }
   }

   updateModelMatrix();
}

void WorldObject::updateModelMatrix()
{
   for(size_t i = 0; i < modelMatrix.size(); i++)
   {
      if(isModelMatrixInvalid[i])
      {
         modelMatrix[i] = glm::translate(glm::mat4(), position[i]);

         modelMatrix[i] = glm::rotate(modelMatrix[i], glm::radians(rotation[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
         modelMatrix[i] = glm::rotate(modelMatrix[i], glm::radians(rotation[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
         modelMatrix[i] = glm::rotate(modelMatrix[i], glm::radians(rotation[i].x), glm::vec3(1.0f, 0.0f, 0.0f));

         modelMatrix[i] = glm::scale(modelMatrix[i], scale[i]);
      }
   }
}

uint32_t WorldObject::addInstance(uint32_t meshId)
{
   this->meshId.push_back(meshId);
   position.push_back(glm::vec3(0.f));
   rotation.push_back(glm::vec3(0.f));
   scale.push_back(glm::vec3(1.f));
   rotationSpeed.push_back(glm::vec3(0.f, 0.f, 10.f));
   movingSpeed.push_back(0.f);
   isModelMatrixInvalid.push_back(true);
   isChangingPosition.push_back(false);
   isChangingRotation.push_back(false);
   isChangingScale.push_back(false);
   modelMatrix.push_back(glm::mat4());
   movingDirection.push_back(glm::vec3(0.f));
   targetPosition.push_back(glm::vec3(0.f));
   targetRotation.push_back(glm::vec3(0.f));
   targetScale.push_back(glm::vec3(0.f));

   worldObjectToMeshMapper->addWorldObject(meshId, numberOfObjects);

   return static_cast<uint32_t>(numberOfObjects++);
}

uint32_t WorldObject::addInstance(uint32_t meshId, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
   this->meshId.push_back(meshId);
   this->position.push_back(position);
   this->rotation.push_back(rotation);
   this->scale.push_back(scale);
   rotationSpeed.push_back(glm::vec3(0.f, 0.f, 10.f));
   movingSpeed.push_back(0.f);
   isModelMatrixInvalid.push_back(true);
   isChangingPosition.push_back(false);
   isChangingRotation.push_back(false);
   isChangingScale.push_back(false);
   modelMatrix.push_back(glm::mat4());
   movingDirection.push_back(glm::vec3(0.f));
   targetPosition.push_back(glm::vec3(0.f));
   targetRotation.push_back(glm::vec3(0.f));
   targetScale.push_back(glm::vec3(0.f));

   worldObjectToMeshMapper->addWorldObject(meshId, numberOfObjects);

   return static_cast<uint32_t>(numberOfObjects++);
}


void WorldObject::setPosition(uint32_t index, glm::vec3 position)
{
   targetPosition[index] = position;
   isChangingPosition[index] = true;
}

void WorldObject::setRotation(uint32_t index, glm::vec3 rotation)
{
   targetRotation[index] = rotation;
   isChangingRotation[index] = true;
}

void WorldObject::setScale(uint32_t index, glm::vec3 scale)
{
   targetScale[index] = scale;
   isChangingScale[index] = true;
}

void WorldObject::setRotationSpeed(uint32_t index, float yaw, float pitch, float roll)
{
   rotationSpeed[index].x = yaw;
   rotationSpeed[index].y = pitch;
   rotationSpeed[index].z = roll;
}

void WorldObject::setMovingSpeed(uint32_t index, float movingSpeed)
{
   this->movingSpeed[index] = movingSpeed;
}

void WorldObject::setMovingDirection(uint32_t index, glm::vec3 movingDirection)
{
   this->movingDirection[index] = movingDirection;
}

void WorldObject::invalidateModelMatrix(uint32_t index)
{
   isModelMatrixInvalid[index] = true;
}
