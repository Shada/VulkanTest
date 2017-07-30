#include "WorldObjectToMeshMapper.h"

void WorldObjectToMeshMapper::addWorldObject(int meshId, int objectId)
{
   meshObjectMap[meshId].push_back(objectId);
}

