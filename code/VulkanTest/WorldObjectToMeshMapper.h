#pragma once

#include <map>
#include <vector>

// have the dynamic buffer here
// wont actually fill it, just storage.

class WorldObjectToMeshMapper
{
public:

   void addWorldObject(int meshId, int objectId);

   std::vector<int> getWorldObjectIdsForMesh(int meshId)
   {
      return meshObjectMap[meshId];
   }

private:

   std::map<int, std::vector<int>> meshObjectMap;
  // std::vector<std::vector<int>> meshObjectVector;
};

